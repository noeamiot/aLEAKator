/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2019-2020  whitequark <whitequark@whitequark.org>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

// This file is included by the designs generated with `write_cxxrtl`. It is not used in Yosys itself.
//
// The CXXRTL support library implements compile time specialized arbitrary width arithmetics, as well as provides
// composite lvalues made out of bit slices and concatenations of lvalues. This allows the `write_cxxrtl` pass
// to perform a straightforward translation of RTLIL structures to readable C++, relying on the C++ compiler
// to unwrap the abstraction and generate efficient code.

#ifndef CXXRTL_H
#define CXXRTL_H

#include <ostream>
#include <signal.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <limits>
#include <type_traits>
#include <tuple>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <functional>
#include <sstream>
#include <fstream>
#include <iostream>

// `cxxrtl::debug_item` has to inherit from `cxxrtl_object` to satisfy strict aliasing requirements.
#include <cxxrtl/capi/cxxrtl_capi.h>

// Use lss and verif librairies
#include "verif_msi_pp.hpp"
#include "lss.h"

// Global instance of the simulation logger, must be defined by each simulator once
extern std::ofstream simulation_logger;

#ifndef __has_attribute
#	define __has_attribute(x) 0
#endif

// CXXRTL essentially uses the C++ compiler as a hygienic macro engine that feeds an instruction selector.
// It generates a lot of specialized template functions with relatively large bodies that, when inlined
// into the caller and (for those with loops) unrolled, often expose many new optimization opportunities.
// Because of this, most of the CXXRTL runtime must be always inlined for best performance.
#if __has_attribute(always_inline)
#define CXXRTL_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#define CXXRTL_ALWAYS_INLINE inline
#endif
// Conversely, some functions in the generated code are extremely large yet very cold, with both of these
// properties being extreme enough to confuse C++ compilers into spending pathological amounts of time
// on a futile (the code becomes worse) attempt to optimize the least important parts of code.
#if __has_attribute(optnone)
#define CXXRTL_EXTREMELY_COLD __attribute__((__optnone__))
#elif __has_attribute(optimize)
#define CXXRTL_EXTREMELY_COLD __attribute__((__optimize__(0)))
#else
#define CXXRTL_EXTREMELY_COLD
#endif

// CXXRTL uses assert() to check for C++ contract violations (which may result in e.g. undefined behavior
// of the simulation code itself), and CXXRTL_ASSERT to check for RTL contract violations (which may at
// most result in undefined simulation results).
//
// Though by default, CXXRTL_ASSERT() expands to assert(), it may be overridden e.g. when integrating
// the simulation into another process that should survive violating RTL contracts.
#ifndef CXXRTL_ASSERT
#ifndef CXXRTL_NDEBUG
#define CXXRTL_ASSERT(x) assert(x)
#else
#define CXXRTL_ASSERT(x)
#endif
#endif

namespace cxxrtl {

// All arbitrary-width values in CXXRTL are backed by arrays of unsigned integers called chunks. The chunk size
// is the same regardless of the value width to simplify manipulating values via FFI interfaces, e.g. driving
// and introspecting the simulation in Python.
//
// It is practical to use chunk sizes between 32 bits and platform register size because when arithmetics on
// narrower integer types is legalized by the C++ compiler, it inserts code to clear the high bits of the register.
// However, (a) most of our operations do not change those bits in the first place because of invariants that are
// invisible to the compiler, (b) we often operate on non-power-of-2 values and have to clear the high bits anyway.
// Therefore, using relatively wide chunks and clearing the high bits explicitly and only when we know they may be
// clobbered results in simpler generated code.
typedef uint32_t chunk_t;
typedef uint64_t wide_chunk_t;

template<typename T>
struct chunk_traits {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
	              "chunk type must be an unsigned integral type");
	using type = T;
	static constexpr size_t bits = std::numeric_limits<T>::digits;
	static constexpr T mask = std::numeric_limits<T>::max();
};

struct leakable {
	// Previous and curr values of each written cells
	virtual std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>> leak_mem() const = 0;
	virtual std::pair<Node*, leaks::LeakSet*> leak_single() const = 0;
};

template<class T>
struct expr_base;

template<size_t Bits>
struct value : public expr_base<value<Bits>>, leakable {
	static constexpr size_t bits = Bits;
	Node* node = nullptr;
	leaks::LeakSet* ls = nullptr;

	std::pair<Node*, leaks::LeakSet*> leak_single() const {
		return {node, ls};
	}
	std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>> leak_mem() const {
		assert(false && "Not a memory.");
		return std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>>();
	}

	using chunk = chunk_traits<chunk_t>;
	static constexpr chunk::type msb_mask = (Bits % chunk::bits == 0) ? chunk::mask
		: chunk::mask >> (chunk::bits - (Bits % chunk::bits));

	static constexpr size_t chunks = (Bits + chunk::bits - 1) / chunk::bits;
	chunk::type data[chunks] = {};
	chunk::type stability[chunks] = {};

	// Warning: considered unstable by default
	value() : node(&constant(0, Bits)) { }

	template<typename... Init>
	explicit constexpr value(Init ...init) : data{init...} {
		auto values = {init...};
		node = &constant(*values.begin(), (chunks>1) ? chunk::bits : Bits);
		size_t remainingBits = (chunks>1) ? (Bits - chunk::bits) : 0;

		// First arg is the msb in data[]
		for (auto it = values.begin() + 1; it != values.end(); ++it) {
			node = &Concat(constant(*it, (remainingBits>chunk::bits) ? chunk::bits : remainingBits), *node);
			remainingBits -= chunk::bits;
		}

		// Warning: All values initialized with this method are considered stable
		// This is because constants are initalized this way
		for (unsigned int n = 0; n < chunks-1; ++n)
			stability[n] = chunk::mask;
		stability[chunks - 1] = msb_mask;
		assert(Bits == node->width);
		this->debug_assert();
	}

	value(const value<Bits> &other) = default;
	value<Bits> &operator=(const value<Bits> &) = default;

	value(value<Bits> &&) = default;
	value<Bits> &operator=(value<Bits> &&) = default;

	void symb_keep(bool toBeKept) {
		this->debug_assert();
		if (toBeKept) {
			leaks::keep(ls);
		} else {
			#ifdef NO_CLEAN_STATE
			leaks::keep(ls);
			#else
			ls = nullptr;
			node = &constant(0, Bits);
			for (size_t n = 0; n < chunks; ++n) {
				data[n] = 0;
				stability[n] = 0;
			}
			#endif
		}
	}

	// A (no-op) helper that forces the cast to value<>.
	CXXRTL_ALWAYS_INLINE
	const value<Bits> &val() const {
		return *this;
	}

	std::string str() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

	// Conversion operations.
	//
	// These functions ensure that a conversion is never out of range, and should be always used, if at all
	// possible, instead of direct manipulation of the `data` member. For very large types, .slice() and
	// .concat() can be used to split them into more manageable parts.
	template<class IntegerT, typename std::enable_if<!std::is_signed<IntegerT>::value, int>::type = 0>
	CXXRTL_ALWAYS_INLINE
	IntegerT get() const {
		static_assert(std::numeric_limits<IntegerT>::is_integer && !std::numeric_limits<IntegerT>::is_signed,
		              "get<T>() requires T to be an unsigned integral type");
		static_assert(std::numeric_limits<IntegerT>::digits >= Bits,
		              "get<T>() requires T to be at least as wide as the value is");
		IntegerT result = 0;
		for (size_t n = 0; n < chunks; n++)
			result |= IntegerT(data[n]) << (n * chunk::bits);
		return result;
	}

	template<class IntegerT, typename std::enable_if<std::is_signed<IntegerT>::value, int>::type = 0>
	CXXRTL_ALWAYS_INLINE
	IntegerT get() const {
		auto unsigned_result = get<typename std::make_unsigned<IntegerT>::type>();
		IntegerT result;
		memcpy(&result, &unsigned_result, sizeof(IntegerT));
		return result;
	}

	// Everything is considered unstable
	// TODO: If it really is considered unstable, we should clear the stability shouldn't we ?
	template<class IntegerT, bool setConstNode = false, typename std::enable_if<!std::is_signed<IntegerT>::value, int>::type = 0>
	CXXRTL_ALWAYS_INLINE
	void set(IntegerT value) {
		static_assert(std::numeric_limits<IntegerT>::is_integer && !std::numeric_limits<IntegerT>::is_signed,
		              "set<T>() requires T to be an unsigned integral type");
		static_assert(std::numeric_limits<IntegerT>::digits >= Bits,
		              "set<T>() requires the value to be at least as wide as T is");
		for (size_t n = 0; n < chunks; n++) {
			data[n] = (value >> (n * chunk::bits)) & chunk::mask;
		}

		if constexpr (setConstNode)
			node = &constant(value, Bits);
	}

	template<class IntegerT, bool setConstNode = false, typename std::enable_if<std::is_signed<IntegerT>::value, int>::type = 0>
	CXXRTL_ALWAYS_INLINE
	void set(IntegerT value) {
		typename std::make_unsigned<IntegerT>::type unsigned_value;
		memcpy(&unsigned_value, &value, sizeof(IntegerT));
		set(unsigned_value);

		assert(false && "Unimplemented");
	}

	CXXRTL_ALWAYS_INLINE
	Node* getNode() const {
		return node;
	}

	CXXRTL_ALWAYS_INLINE
	void setNode(Node* newNode) {
		// TODO: Should be done in config so that no recompilation for something this trivial is needed
		#ifdef MUST_BIT_DECOMPOSE
		node = &getBitDecomposition(*newNode);
		#else
		node = newNode;
		#endif
		ls = leaks::reg_stabilize(node);
	}

	void debug_assert() const {
		#ifdef DEBUG_STATE_CONSISTANCE
		if(this->node->nature == CONST) {
			// Find fully const node that are not consistant with conc state
			assert(this->cmpConc());
		} else {
			// Find partially const node that are not consistant with conc state
			for (size_t i = 0; i < Bits; ++i) {
				Node* curr = &simplify(Extract(i, i, *(this->node)));
				assert(curr->nature != CONST || (((this->data[i/32]) >> (i%32)) & 1) == curr->cst[0]);
			}
		}

		// Find looping node creations
		//if (this->node->hasSaturated == true)
		//	raise(SIGTRAP);

		// Find leaksets that aren't real if any
		if (this->ls != nullptr and not leaks::is_ls_real(this->ls))
			raise(SIGTRAP);

		// Find stability overlapping the number of bits
		assert((stability[chunks-1] & ~msb_mask) == 0x0u);
		#endif
	}

	// Operations with compile-time parameters.
	//
	// These operations are used to implement slicing, concatenation, and blitting.
	// The trunc, zext and sext operations add or remove most significant bits (i.e. on the left);
	// the rtrunc and rzext operations add or remove least significant bits (i.e. on the right).
	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> trunc() const {
		static_assert(NewBits <= Bits, "trunc() may not increase width");
		value<NewBits> result;
		// Set stability on all newly added bits
		for (size_t n = 0; n < result.chunks; n++) {
			result.data[n] = data[n];
			result.stability[n] = stability[n];
		}
		result.data[result.chunks - 1] &= result.msb_mask;
		result.stability[result.chunks - 1] &= result.msb_mask;

		if constexpr (NewBits != Bits) {
			result.node = &simplify(Extract(NewBits - 1, 0, *node));
			// No partial stab, ls cannot change
			result.ls = leaks::extract(ls, 0, NewBits - 1);
		} else {
			result.node = node;
			result.ls = ls;
		}

		result.debug_assert();
		return result;
	}

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> zext() const {
		static_assert(NewBits >= Bits, "zext() may not decrease width");
		value<NewBits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = data[n];
			result.stability[n] = stability[n];
		}
		// Set stability on all newly added bits
		result.stability[chunks - 1] |= ~msb_mask;
		for (size_t n = chunks; n < result.chunks; n++)
			result.stability[n] = chunk::mask;
		result.stability[result.chunks - 1] &= result.msb_mask;

		if constexpr (NewBits != Bits) {
			result.node = &simplify(ZeroExt(NewBits - Bits, *node));
			result.ls = leaks::extend(ls, NewBits);
		} else {
			result.node = node;
			result.ls = ls;
		}

		result.debug_assert();
		return result;
	}

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> sext() const {
		static_assert(NewBits >= Bits, "sext() may not decrease width");

		value<NewBits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = data[n];
			result.stability[n] = stability[n];
		}
		if (Concis_neg()) { // It is valid to concretize as node and ls are handled correctly at the end
			result.data[chunks - 1] |= ~msb_mask;
			for (size_t n = chunks; n < result.chunks; n++)
				result.data[n] = chunk::mask;
			result.data[result.chunks - 1] &= result.msb_mask;
		}
		// Stability of the previous MSB is used as stability for all bits added
		if (stability[chunks - 1] & (1 << ((Bits - 1) % chunk::bits))) {
			result.stability[chunks - 1] |= ~msb_mask;
			for (size_t n = chunks; n < result.chunks; n++)
				result.stability[n] = chunk::mask;
			result.stability[result.chunks - 1] &= result.msb_mask;
		}

		if constexpr (NewBits != Bits) {
			result.node = &simplify(SignExt(NewBits - Bits, *node));
			result.ls = leaks::sextend(ls, NewBits);
		} else {
			result.node = node;
			result.ls = ls;
		}

		result.debug_assert();
		return result;
	}

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> rtrunc() const {
		static_assert(NewBits <= Bits, "rtrunc() may not increase width");
		value<NewBits> result;
		constexpr size_t shift_chunks = (Bits - NewBits) / chunk::bits;
		constexpr size_t shift_bits   = (Bits - NewBits) % chunk::bits;
		chunk::type carry = 0;
		chunk::type carryStab = 0;
		if (shift_chunks + result.chunks < chunks) {
			carry = (shift_bits == 0) ? 0
				: data[shift_chunks + result.chunks] << (chunk::bits - shift_bits);
			carryStab = (shift_bits == 0) ? 0
				: stability[shift_chunks + result.chunks] << (chunk::bits - shift_bits);
		}
		for (size_t n = result.chunks; n > 0; n--) {
			result.data[n - 1] = carry | (data[shift_chunks + n - 1] >> shift_bits);
			carry = (shift_bits == 0) ? 0
				: data[shift_chunks + n - 1] << (chunk::bits - shift_bits);
			result.stability[n - 1] = carryStab | (stability[shift_chunks + n - 1] >> shift_bits);
			carryStab = (shift_bits == 0) ? 0
				: stability[shift_chunks + n - 1] << (chunk::bits - shift_bits);
		}

		if constexpr (NewBits != Bits) {
			result.node = &simplify(Extract(Bits - 1, Bits - NewBits, *node));
			// No partial stab as it cannot change the ls
			result.ls = leaks::extract(ls, Bits - NewBits, Bits - 1);
		} else {
			result.node = node;
			result.ls = ls;
		}

		result.debug_assert();
		return result;
	}

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> rzext() const {
		static_assert(NewBits >= Bits, "rzext() may not decrease width");
		value<NewBits> result;
		constexpr size_t shift_chunks = (NewBits - Bits) / chunk::bits;
		constexpr size_t shift_bits   = (NewBits - Bits) % chunk::bits;
		chunk::type carry = 0;
		chunk::type carryStab = 0;
		for (size_t n = 0; n < chunks; n++) {
			result.data[shift_chunks + n] = (data[n] << shift_bits) | carry;
			carry = (shift_bits == 0) ? 0
				: data[n] >> (chunk::bits - shift_bits);

			result.stability[shift_chunks + n] = (stability[n] << shift_bits) | carryStab;
			carryStab = (shift_bits == 0) ? 0
				: stability[n] >> (chunk::bits - shift_bits);
		}
		if (shift_chunks + chunks < result.chunks) {
			result.data[shift_chunks + chunks] = carry;
			result.stability[shift_chunks + chunks] = carryStab;
		}

		// Set stability on all newly added bits
		for (size_t n = 0; n < shift_chunks; n++)
			result.stability[n] = chunk::mask;
		if (shift_bits > 0)
			result.stability[shift_chunks] |= (chunk::mask >> (chunk::bits - shift_bits));

		if constexpr (NewBits != Bits) {
			result.node = &simplify(Concat(*node, constant(0, NewBits - Bits)));
			result.ls = leaks::rextend(ls, NewBits);
		} else {
			result.node = node;
			result.ls = ls;
		}

		result.debug_assert();
		return result;
	}

	// Bit blit operation, i.e. a partial read-modify-write.
	template<size_t Stop, size_t Start>
	CXXRTL_ALWAYS_INLINE
	value<Bits> blit(const value<Stop - Start + 1> &source) const {
		static_assert(Stop >= Start, "blit() may not reverse bit order");
		constexpr chunk::type start_mask = ~(chunk::mask << (Start % chunk::bits));
		constexpr chunk::type stop_mask = (Stop % chunk::bits + 1 == chunk::bits) ? 0
			: (chunk::mask << (Stop % chunk::bits + 1));

		value<Bits> masked = *this;
		if (Start / chunk::bits == Stop / chunk::bits) {
			masked.data[Start / chunk::bits] &= stop_mask | start_mask;
			masked.stability[Start / chunk::bits] &= stop_mask | start_mask;
		} else {
			masked.data[Start / chunk::bits] &= start_mask;
			masked.stability[Start / chunk::bits] &= start_mask;
			for (size_t n = Start / chunk::bits + 1; n < Stop / chunk::bits; n++) {
				masked.data[n] = 0;
				masked.stability[n] = 0;
			}
			masked.data[Stop / chunk::bits] &= stop_mask;
			masked.stability[Stop / chunk::bits] &= stop_mask;
		}
		// TODO: We could save some operations here by clearing the leaksets before
		// We can't use shifted stability here as it will be extended with stable bits
		// TODO: We could save some time by shifting the data as well ...
		value<Bits> shifted = source
			.template rzext<Stop + 1>()
			.template zext<Bits>();

		// Stab and val init to 0
		value<Bits> shifted_clone;

		if (Start / chunk::bits == Stop / chunk::bits) {
			shifted_clone.data[Start / chunk::bits] = shifted.data[Start / chunk::bits] & (~stop_mask & ~start_mask);
			shifted_clone.stability[Start / chunk::bits] = shifted.stability[Start / chunk::bits] & (~stop_mask & ~start_mask);
		} else {
			shifted_clone.data[Start / chunk::bits] = shifted.data[Start / chunk::bits] & ~start_mask;
			shifted_clone.stability[Start / chunk::bits] = shifted.stability[Start / chunk::bits] & ~start_mask;
			for (size_t i = (Start / chunk::bits) + 1; i < Stop / chunk::bits; i++) {
				shifted_clone.data[i] = shifted.data[i];
				shifted_clone.stability[i] = shifted.stability[i];
			}
			shifted_clone.data[Stop / chunk::bits] = shifted.data[Stop / chunk::bits] & ~stop_mask;
			shifted_clone.stability[Stop / chunk::bits] = shifted.stability[Stop / chunk::bits] & ~stop_mask;
		}

		// Normally cxxrtl would do a bit_or between the mask and the shifted val which
		// we don't do for performance reasons, we just re-implement the or.
		value<Bits> res;
		if constexpr (Start == 0) {
			res.node = &simplify(Concat(Extract(Bits - 1, Stop + 1, *node), *source.node));
		} else if constexpr (Stop == Bits - 1) {
			res.node = &simplify(Concat(*source.node, Extract(Start - 1, 0, *node)));
		} else {
			res.node = &simplify(Concat(Extract(Bits - 1, Stop + 1, *node), *source.node, Extract(Start - 1, 0, *node)));
		}


		for (size_t n = 0; n < chunks; n++) {
			res.data[n] = masked.data[n] | shifted_clone.data[n];
			res.stability[n] = masked.stability[n] | shifted_clone.stability[n];
		}

		// No need for partial stabilisation as if bits were stable, they still are and ls was stabilized before
		res.ls = leaks::blit(ls, source.ls, Start, Stop, Bits);

		res.debug_assert();
		return res;
	}

	// Helpers for selecting extending or truncating operation depending on whether the result is wider or narrower
	// than the operand. In C++17 these can be replaced with `if constexpr`.
	template<size_t NewBits, typename = void>
	struct zext_cast {
		CXXRTL_ALWAYS_INLINE
		value<NewBits> operator()(const value<Bits> &val) {
			return val.template zext<NewBits>();
		}
	};

	template<size_t NewBits>
	struct zext_cast<NewBits, typename std::enable_if<(NewBits < Bits)>::type> {
		CXXRTL_ALWAYS_INLINE
		value<NewBits> operator()(const value<Bits> &val) {
			return val.template trunc<NewBits>();
		}
	};

	template<size_t NewBits, typename = void>
	struct sext_cast {
		CXXRTL_ALWAYS_INLINE
		value<NewBits> operator()(const value<Bits> &val) {
			return val.template sext<NewBits>();
		}
	};

	template<size_t NewBits>
	struct sext_cast<NewBits, typename std::enable_if<(NewBits < Bits)>::type> {
		CXXRTL_ALWAYS_INLINE
		value<NewBits> operator()(const value<Bits> &val) {
			return val.template trunc<NewBits>();
		}
	};

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> zcast() const {
		return zext_cast<NewBits>()(*this);
	}

	template<size_t NewBits>
	CXXRTL_ALWAYS_INLINE
	value<NewBits> scast() const {
		return sext_cast<NewBits>()(*this);
	}

	// Bit replication is far more efficient than the equivalent concatenation.
	template<size_t Count>
	CXXRTL_ALWAYS_INLINE
	value<Bits * Count> repeat() const {
		static_assert(Bits == 1, "repeat() is implemented only for 1-bit values");
		// As Bits is = 1 and we don't really need to make this comparison to create the symbolic value
		// We directly compare to prevent a useless concretisation warning
		value<Bits * Count> res = this->data[0] ? value<Bits * Count>().bit_not() : value<Bits * Count>();

		// Trick to replicate stability
		value<Bits * Count> resStab = this->stability[0] ? value<Bits * Count>().bit_not() : value<Bits * Count>();
		std::copy(resStab.data, resStab.data + value<Bits * Count>::chunks, res.stability);

		std::vector<Node*> concatNodes(Bits * Count, node);
		res.node = &Concat(concatNodes);
		// No need to apply partial stabilisation as if the bit was stable, it has already been stabilised
		res.ls = leaks::replicate(ls, Bits * Count);
		res.debug_assert();
		return res;
	}

	// Operations with run-time parameters (offsets, amounts, etc).
	//
	// These operations are used for computations.
	bool bit(size_t offset) const {
		assert(false && "Unimplemented.");
		return data[offset / chunk::bits] & (1 << (offset % chunk::bits));
	}

	void set_bit(size_t offset, bool value = true) {
		assert(false && "Unimplemented.");
		size_t offset_chunks = offset / chunk::bits;
		size_t offset_bits = offset % chunk::bits;
		data[offset_chunks] &= ~(1 << offset_bits);
		data[offset_chunks] |= value ? 1 << offset_bits : 0;
	}

	explicit operator bool() const {
		return !is_zero();
	}

	bool is_zero() const {
		if (this->node->nature != CONST) {
			simulation_logger << "Comparison to 0 of a non conc signal !" << std::endl;
			std::abort();
		}

		for (size_t n = 0; n < chunks; n++)
			if (data[n] != 0)
				return false;
		return true;
	}

	void set_fully_stable() {
		for (size_t n = 0; n < chunks-1; ++n)
			stability[n] = chunk::mask;
		stability[chunks - 1] = msb_mask;
	}

	void set_fully_unstable() {
		for (size_t n = 0; n < chunks; ++n)
			stability[n] = 0x0u;
	}

	bool is_fully_stable() const {
		for (size_t n = 0; n < chunks-1; n++)
			if (stability[n] != chunk::mask)
				return false;
		return (stability[chunks-1] == msb_mask);
	}

	bool Concis_zero() const {
		#ifdef DEBUG_CONCRETIZATION
		if (node->nature != CONST)
			raise(SIGTRAP);
		#endif
		for (size_t n = 0; n < chunks; n++)
			if (data[n] != 0)
				return false;
		return true;
	}

	bool Concis_neg() const {
		#ifdef DEBUG_CONCRETIZATION
		if (node->nature != CONST)
			raise(SIGTRAP);
		#endif

		return data[chunks - 1] & (1 << ((Bits - 1) % chunk::bits));
	}

	bool is_neg() const {
		if (this->node->nature != CONST) {
			simulation_logger << "Neg evaluating a non conc signal !" << std::endl;
			std::abort();
		}

		return data[chunks - 1] & (1 << ((Bits - 1) % chunk::bits));
	}

	bool operator ==(const value<Bits> &other) const {
		if (this->node->nature != CONST) {
			simulation_logger << "Conc comparing a non conc signal !" << std::endl;
			std::abort();
		}

		for (size_t n = 0; n < chunks; n++)
			if (data[n] != other.data[n])
				return false;
		return true;
	}

	// For explicitely conc state ignoring operations
	bool concEq(const value<Bits> &other) const {
		#ifdef DEBUG_CONCRETIZATION
		if (node->nature != CONST)
			raise(SIGTRAP);
		#endif
		for (size_t n = 0; n < chunks; n++)
			if (data[n] != other.data[n])
				return false;
		return true;
	}

	bool ConcUcmp(const value<Bits> &other) const {
		#ifdef DEBUG_CONCRETIZATION
		if (node->nature != CONST)
			raise(SIGTRAP);
		#endif
		bool carry;
		std::tie(std::ignore, carry) = alu</*Invert=*/true, /*CarryIn=*/true>(other);
		return !carry; // a.ucmp(b) ≡ a u< b
	}

	bool ConcScmp(const value<Bits> &other) const {
		#ifdef DEBUG_CONCRETIZATION
		if (node->nature != CONST)
			raise(SIGTRAP);
		#endif
		value<Bits> result;
		bool carry;
		std::tie(result, carry) = alu</*Invert=*/true, /*CarryIn=*/true>(other);
		bool overflow = (Concis_neg() == !other.Concis_neg()) && (Concis_neg() != result.Concis_neg());
		return result.Concis_neg() ^ overflow; // a.scmp(b) ≡ a s< b
	}

	// Only for conc state debug
	bool cmpConc() const {
		// We can't say for non constant nodes
		if (node->nature != CONST)
			return true;

		if (node->width != Bits)
			return false;

		// Ugly but compares the 32 bit segmented and 64 bits segmented arrays
		for (int i = 0; i < node->nlimbs; ++i) {
			if ((uint32_t)node->cst[i] != data[2*i])
				return false;
			if (static_cast<size_t>(2*i+1) >= chunks) {
				if ((uint32_t)(node->cst[i] >> 32) != 0)
					return false;
			} else {
				if ((uint32_t)(node->cst[i] >> 32) != data[2*i+1])
					return false;
			}
		}
		return true;
	}

	bool operator !=(const value<Bits> &other) const {
		return !(*this == other);
	}

	value<Bits> bit_not() const {
		value<Bits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = ~data[n];
			result.stability[n] = stability[n];
		}
		result.data[chunks - 1] &= msb_mask;
		result.stability[chunks - 1] &= msb_mask;

		result.node = &simplify(~*node);
		result.ls = leaks::partial_stabilize(ls, result.node, result.stability);
		result.debug_assert();
		return result;
	}

	value<Bits> bit_and(const value<Bits> &other) const {
		value<Bits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = data[n] & other.data[n];
			result.stability[n] = (stability[n] & other.stability[n]);
			// Absorbing elements for fully const
			if (node->nature == CONST)
				result.stability[n] |= (stability[n] & ~data[n]);
			if (other.node->nature == CONST)
				result.stability[n] |= (other.stability[n] & ~other.data[n]);
		}

		// Absorbing elements for partially const values
		if (node->nature != CONST or other.node->nature != CONST) {
			for (size_t n = 0; n < chunks; n++) {
				chunk_t stab = 0;
				for (size_t i = 0; i < value<Bits>::chunk::bits; i++) {
					size_t bit = (n * value<Bits>::chunk::bits) + i;
					if (bit >= Bits)
						break;
					// If there is stability for this bit, split the node and check if it is const and
					// aborbant
					if (this->stability[n] & (0x1 << i)) {
						Node* na = &simplify(Extract(bit, bit, *this->node));
						stab |= static_cast<chunk_t>(na->nature == CONST and na->cst[0] == 0x0u) << i;
					}
					if (other.stability[n] & (0x1 << i)) {
						Node* nb = &simplify(Extract(bit, bit, *other.node));
						stab |= static_cast<chunk_t>(nb->nature == CONST and nb->cst[0] == 0x0u) << i;
					}
				}
				result.stability[n] |= stab;
			}
		}
		result.stability[chunks - 1] &= msb_mask;

		result.node = &simplify(*node & *other.node);
		result.ls = leaks::partial_stabilize(leaks::merge(ls, other.ls), result.node, result.stability);
		result.debug_assert();
		return result;
	}

	value<Bits> bit_or(const value<Bits> &other) const {
		value<Bits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = data[n] | other.data[n];
			result.stability[n] = (stability[n] & other.stability[n]);
			// Absorbing elements for fully const
			if (node->nature == CONST)
				result.stability[n] |= (stability[n] & data[n]);
			if (other.node->nature == CONST)
				result.stability[n] |= (other.stability[n] & other.data[n]);
		}

		// Absorbing elements for partially const values
		if (node->nature != CONST or other.node->nature != CONST) {
			for (size_t n = 0; n < chunks; n++) {
				chunk_t stab = 0;
				for (size_t i = 0; i < value<Bits>::chunk::bits; i++) {
					size_t bit = (n * value<Bits>::chunk::bits) + i;
					if (bit >= Bits)
						break;
					// If there is stability for this bit, split the node and check if it is const and
					// aborbant
					if (this->stability[n] & (0x1 << i)) {
						Node* na = &simplify(Extract(bit, bit, *this->node));
						stab |= static_cast<chunk_t>(na->nature == CONST and na->cst[0] == 0x1u) << i;
					}
					if (other.stability[n] & (0x1 << i)) {
						Node* nb = &simplify(Extract(bit, bit, *other.node));
						stab |= static_cast<chunk_t>(nb->nature == CONST and nb->cst[0] == 0x1u) << i;
					}
				}
				result.stability[n] |= stab;
			}
		}
		result.stability[chunks - 1] &= msb_mask;

		result.node = &simplify(*node | *other.node);
		result.ls = leaks::partial_stabilize(leaks::merge(ls, other.ls), result.node, result.stability);
		result.debug_assert();
		return result;
	}

	value<Bits> bit_xor(const value<Bits> &other) const {
		value<Bits> result;
		for (size_t n = 0; n < chunks; n++) {
			result.data[n] = data[n] ^ other.data[n];
			result.stability[n] = (stability[n] & other.stability[n]);
		}

		result.node = &simplify(*node ^ *other.node);
		result.ls = leaks::partial_stabilize(leaks::merge(ls, other.ls), result.node, result.stability);
		result.debug_assert();
		return result;
	}

	// Here stability and leaksets are not considered as this function is only called
	// for memories which handle it thenmselves
	value<Bits> update(const value<Bits> &val, const value<Bits> &mask) const {
		return bit_and(mask.bit_not()).bit_or(val.bit_and(mask));
	}

	template<size_t AmountBits>
	value<Bits> shl(const value<AmountBits> &amount) const {
		// Ensure our early return is correct by prohibiting values larger than 4 Gbit.
		static_assert(Bits <= chunk::mask, "shl() of unreasonably large values is not supported");
		// Detect shifts definitely large than Bits early.
		for (size_t n = 1; n < amount.chunks; n++) {
			if (amount.data[n] != 0) {
				if (amount.node->nature != CONST)
					simulation_logger << "Early return in shift left, glitches may not be accurate" << std::endl;
				return {};
			}
		}
		// Past this point we can use the least significant chunk as the shift size.
		size_t shift_chunks = amount.data[0] / chunk::bits;
		size_t shift_bits   = amount.data[0] % chunk::bits;
		if (shift_chunks >= chunks) {
			if (amount.node->nature != CONST)
				simulation_logger << "Early return in shift left, glitches may not be accurate" << std::endl;
			return {};
		}

		value<Bits> result;
		chunk::type carry = 0;
		for (size_t n = 0; n < chunks - shift_chunks; n++) {
			result.data[shift_chunks + n] = (data[n] << shift_bits) | carry;
			carry = (shift_bits == 0) ? 0
				: data[n] >> (chunk::bits - shift_bits);
		}
		result.data[result.chunks - 1] &= result.msb_mask;

		// For now, output is stable if both amount and shifted value are fully stable
		// TODO: We can do slightly better if at least the amount is stable, in the else case
		// shouldn't we clear the stability of the output ?
		if (is_fully_stable() && amount.is_fully_stable())
			std::copy(stability, stability + value<Bits>::chunks, result.stability);

		result.node = &simplify(*node << *amount.node);
		result.ls = leaks::partial_stabilize(leaks::shift_left(ls, amount.ls, Bits), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	template<size_t AmountBits, bool Signed = false>
	value<Bits> shr(const value<AmountBits> &amount) const {
		// Ensure our early return is correct by prohibiting values larger than 4 Gbit.
		static_assert(Bits <= chunk::mask, "shr() of unreasonably large values is not supported");
		// Detect shifts definitely large than Bits early.
		for (size_t n = 1; n < amount.chunks; n++) {
			if (amount.data[n] != 0) {
				if (amount.node->nature != CONST)
					simulation_logger << "Early return in shift right, glitches may not be accurate" << std::endl;
				return (Signed && is_neg()) ? value<Bits>().bit_not() : value<Bits>();
			}
		}

		// Past this point we can use the least significant chunk as the shift size.
		size_t shift_chunks = amount.data[0] / chunk::bits;
		size_t shift_bits   = amount.data[0] % chunk::bits;
		if (shift_chunks >= chunks) {
			if (amount.node->nature != CONST)
				simulation_logger << "Early return in shift right, glitches may not be accurate" << std::endl;
			return (Signed && is_neg()) ? value<Bits>().bit_not() : value<Bits>();
		}

		value<Bits> result;
		chunk::type carry = 0;
		for (size_t n = 0; n < chunks - shift_chunks; n++) {
			result.data[chunks - shift_chunks - 1 - n] = carry | (data[chunks - 1 - n] >> shift_bits);
			carry = (shift_bits == 0) ? 0
				: data[chunks - 1 - n] << (chunk::bits - shift_bits);
		}

		// If it is a signed operation, we must set all significant bits to msb val and fill
		// leakset with the same process
		if (Signed && Concis_neg()) {
			size_t top_chunk_idx  = amount.data[0] > Bits ? 0 : (Bits - amount.data[0]) / chunk::bits;
			size_t top_chunk_bits = amount.data[0] > Bits ? 0 : (Bits - amount.data[0]) % chunk::bits;
			for (size_t n = top_chunk_idx + 1; n < chunks; n++)
				result.data[n] = chunk::mask;
			if (amount.data[0] != 0)
				result.data[top_chunk_idx] |= chunk::mask << top_chunk_bits;
			result.data[result.chunks - 1] &= result.msb_mask;
		}

		// For now, output is stable if both amount and shifted value are fully stable
		// TODO: We can do slightly better if at least the amount is stable, in the else case
		// shouldn't we clear the stability of the output ?
		if (is_fully_stable() && amount.is_fully_stable())
			std::copy(stability, stability + value<Bits>::chunks, result.stability);

		// Handle arithmetic vs logical shifts
		if (Signed) {
			result.node = &simplify(*node >> *amount.node);
		} else {
			result.node = &simplify(LShR(*node, *amount.node));
		}

		// LeaskSet shift implementation is that ls of MSBs are cumulatively set on LSBs so
		// arithmetic shift does not change the leakset behavior !
		result.ls = leaks::partial_stabilize(leaks::shift_right(ls, amount.ls, Bits), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	template<size_t AmountBits>
	value<Bits> sshr(const value<AmountBits> &amount) const {
		return shr<AmountBits, /*Signed=*/true>(amount);
	}

	template<size_t ResultBits, size_t SelBits>
	value<ResultBits> bmux(const value<SelBits> &sel) const {
		assert(false && "Unimplemented.");
		static_assert(ResultBits << SelBits == Bits, "invalid sizes used in bmux()");
		size_t amount = sel.data[0] * ResultBits;
		size_t shift_chunks = amount / chunk::bits;
		size_t shift_bits   = amount % chunk::bits;
		value<ResultBits> result;
		chunk::type carry = 0;
		if (ResultBits % chunk::bits + shift_bits > chunk::bits)
			carry = data[result.chunks + shift_chunks] << (chunk::bits - shift_bits);
		for (size_t n = 0; n < result.chunks; n++) {
			result.data[result.chunks - 1 - n] = carry | (data[result.chunks + shift_chunks - 1 - n] >> shift_bits);
			carry = (shift_bits == 0) ? 0
				: data[result.chunks + shift_chunks - 1 - n] << (chunk::bits - shift_bits);
		}
		result.data[result.chunks - 1] &= result.msb_mask;
		return result;
	}

	template<size_t ResultBits, size_t SelBits>
	value<ResultBits> demux(const value<SelBits> &sel) const {
		assert(false && "Unimplemented.");
		static_assert(Bits << SelBits == ResultBits, "invalid sizes used in demux()");
		size_t amount = sel.data[0] * Bits;
		size_t shift_chunks = amount / chunk::bits;
		size_t shift_bits   = amount % chunk::bits;
		value<ResultBits> result;
		chunk::type carry = 0;
		for (size_t n = 0; n < chunks; n++) {
			result.data[shift_chunks + n] = (data[n] << shift_bits) | carry;
			carry = (shift_bits == 0) ? 0
				: data[n] >> (chunk::bits - shift_bits);
		}
		if (Bits % chunk::bits + shift_bits > chunk::bits)
			result.data[shift_chunks + chunks] = carry;
		return result;
	}

	size_t ctpop() const {
		if (node->nature != CONST) {
			simulation_logger << "Ctpoping a non conc signal, not implemented so stopping" << std::endl;
			std::abort();
		}

		size_t count = 0;
		for (size_t n = 0; n < chunks; n++) {
			// This loop implements the population count idiom as recognized by LLVM and GCC.
			for (chunk::type x = data[n]; x != 0; count++)
				x = x & (x - 1);
		}
		return count;
	}

	size_t ctlz() const {
		assert(false && "Unimplemented.");
		size_t count = 0;
		for (size_t n = 0; n < chunks; n++) {
			chunk::type x = data[chunks - 1 - n];
			// First add to `count` as if the chunk is zero
			constexpr size_t msb_chunk_bits = Bits % chunk::bits != 0 ? Bits % chunk::bits : chunk::bits;
			count += (n == 0 ? msb_chunk_bits : chunk::bits);
			// If the chunk isn't zero, correct the `count` value and return
			if (x != 0) {
				for (; x != 0; count--)
					x >>= 1;
				break;
			}
		}
		return count;
	}

	template<bool Invert, bool CarryIn>
	std::pair<value<Bits>, bool /*CarryOut*/> alu(const value<Bits> &other) const {
		value<Bits> result;
		bool carry = CarryIn;
		for (size_t n = 0; n < result.chunks; n++) {
			result.data[n] = data[n] + (Invert ? ~other.data[n] : other.data[n]) + carry;
			if (result.chunks - 1 == n)
				result.data[result.chunks - 1] &= result.msb_mask;
			carry = (result.data[n] <  data[n]) ||
			        (result.data[n] == data[n] && carry);
		}
		return {result, carry};
	}

	value<Bits> add(const value<Bits> &other) const {
		value<Bits> result = alu</*Invert=*/false, /*CarryIn=*/false>(other).first;
		result.node = &simplify(*node + *other.node);

		// Replicate full stability on output only if both inputs are fully stable
		if (is_fully_stable() && other.is_fully_stable())
			std::copy(stability, stability + value<Bits>::chunks, result.stability);

		result.ls = leaks::partial_stabilize(leaks::mix(ls, other.ls), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	value<Bits> sub(const value<Bits> &other) const {
		value<Bits> result = alu</*Invert=*/true, /*CarryIn=*/true>(other).first;
		result.node = &simplify(*node - *other.node);

		// Replicate full stability on output only if both inputs are fully stable
		if (is_fully_stable() && other.is_fully_stable())
			std::copy(stability, stability + value<Bits>::chunks, result.stability);

		result.ls = leaks::partial_stabilize(leaks::mix(ls, other.ls), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	value<Bits> neg() const {
		value<Bits> result = value<Bits>().sub(*this);
		result.node = &simplify(- *node);

		// Replicate full stability on output only if input is fully stable
		// TODO: We could do better, in case node is CONST
		if (is_fully_stable())
			std::copy(stability, stability + value<Bits>::chunks, result.stability);

		result.ls = leaks::partial_stabilize(leaks::mix(ls, nullptr), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	bool ucmp(const value<Bits> &other) const {
		if (this->node->nature != CONST || other.node->nature != CONST) {
			simulation_logger << "Ucmp comparing a non conc signal !" << std::endl;
			std::abort();
		}

		bool carry;
		std::tie(std::ignore, carry) = alu</*Invert=*/true, /*CarryIn=*/true>(other);
		return !carry; // a.ucmp(b) ≡ a u< b
	}

	bool scmp(const value<Bits> &other) const {
		if (this->node->nature != CONST || other.node->nature != CONST) {
			simulation_logger << "Scmp comparing a non conc signal !" << std::endl;
			std::abort();
		}

		value<Bits> result;
		bool carry;
		std::tie(result, carry) = alu</*Invert=*/true, /*CarryIn=*/true>(other);
		bool overflow = (is_neg() == !other.is_neg()) && (is_neg() != result.is_neg());
		return result.is_neg() ^ overflow; // a.scmp(b) ≡ a s< b
	}

	template<size_t ResultBits>
	value<ResultBits> mul(const value<Bits> &other) const {
		value<ResultBits> result;
		wide_chunk_t wide_result[result.chunks + 1] = {};
		for (size_t n = 0; n < chunks; n++) {
			for (size_t m = 0; m < chunks && n + m < result.chunks; m++) {
				wide_result[n + m] += wide_chunk_t(data[n]) * wide_chunk_t(other.data[m]);
				wide_result[n + m + 1] += wide_result[n + m] >> chunk::bits;
				wide_result[n + m] &= chunk::mask;
			}
		}
		for (size_t n = 0; n < result.chunks; n++) {
			result.data[n] = wide_result[n];
		}
		result.data[result.chunks - 1] &= result.msb_mask;
		if (ResultBits > static_cast<size_t>(node->width)) {
			result.node = &simplify(ZeroExt(ResultBits - node->width, *node) * ZeroExt(ResultBits - other.node->width, *other.node));
		} else if (ResultBits < static_cast<size_t>(node->width)) {
			result.node = &Extract(ResultBits - 1, 0, simplify(*node * *other.node));
		} else {
			result.node = &simplify((*node) * (*other.node));
		}

		// Replicate full stability on output only if both inputs are fully stable
		if (is_fully_stable() && other.is_fully_stable()) {
			std::copy(stability, stability + value<Bits>::chunks, result.stability);
			result.stability[result.chunks - 1] &= result.msb_mask;
		}

		result.ls = leaks::partial_stabilize(leaks::mix(ls, other.ls), result.node, result.stability);

		result.debug_assert();
		return result;
	}

	std::pair<value<Bits>, value<Bits>> udivmod(value<Bits> divisor) const {
		assert(false && "Unimplemented.");
		value<Bits> quotient;
		value<Bits> dividend = *this;
		if (dividend.ucmp(divisor))
			return {/*quotient=*/value<Bits>{0u}, /*remainder=*/dividend};
		int64_t divisor_shift = divisor.ctlz() - dividend.ctlz();
		assert(divisor_shift >= 0);
		divisor = divisor.shl(value<Bits>{(chunk::type) divisor_shift});
		for (size_t step = 0; step <= divisor_shift; step++) {
			quotient = quotient.shl(value<Bits>{1u});
			if (!dividend.ucmp(divisor)) {
				dividend = dividend.sub(divisor);
				quotient.set_bit(0, true);
			}
			divisor = divisor.shr(value<Bits>{1u});
		}
		return {quotient, /*remainder=*/dividend};
	}

	std::pair<value<Bits>, value<Bits>> sdivmod(const value<Bits> &other) const {
		assert(false && "Unimplemented.");
		value<Bits + 1> quotient;
		value<Bits + 1> remainder;
		value<Bits + 1> dividend = sext<Bits + 1>();
		value<Bits + 1> divisor = other.template sext<Bits + 1>();
		if (is_neg()) dividend = dividend.neg();
		if (other.is_neg()) divisor = divisor.neg();
		std::tie(quotient, remainder) = dividend.udivmod(divisor);
		if (is_neg() != other.is_neg()) quotient = quotient.neg();
		if (is_neg()) remainder = remainder.neg();
		return {quotient.template trunc<Bits>(), remainder.template trunc<Bits>()};
	}
};

// Expression template for a slice, usable as lvalue or rvalue, and composable with other expression templates here.
template<class T, size_t Stop, size_t Start>
struct slice_expr : public expr_base<slice_expr<T, Stop, Start>> {
	static_assert(Stop >= Start, "slice_expr() may not reverse bit order");
	static_assert(Start < T::bits && Stop < T::bits, "slice_expr() must be within bounds");
	static constexpr size_t bits = Stop - Start + 1;

	T &expr;

	slice_expr(T &expr) : expr(expr) {}
	slice_expr(const slice_expr<T, Stop, Start> &) = delete;

	CXXRTL_ALWAYS_INLINE
	operator value<bits>() const {
		return static_cast<const value<T::bits> &>(expr)
			.template rtrunc<T::bits - Start>()
			.template trunc<bits>();
	}

	CXXRTL_ALWAYS_INLINE
	slice_expr<T, Stop, Start> &operator=(const value<bits> &rhs) {
		// Generic partial assignment implemented using a read-modify-write operation on the sliced expression.
		expr = static_cast<const value<T::bits> &>(expr)
			.template blit<Stop, Start>(rhs);
		return *this;
	}

	// A helper that forces the cast to value<>, which allows deduction to work.
	CXXRTL_ALWAYS_INLINE
	value<bits> val() const {
		return static_cast<const value<bits> &>(*this);
	}
};

// Expression template for a concatenation, usable as lvalue or rvalue, and composable with other expression templates here.
template<class T, class U>
struct concat_expr : public expr_base<concat_expr<T, U>> {
	static constexpr size_t bits = T::bits + U::bits;

	T &ms_expr;
	U &ls_expr;

	concat_expr(T &ms_expr, U &ls_expr) : ms_expr(ms_expr), ls_expr(ls_expr) {}
	concat_expr(const concat_expr<T, U> &) = delete;

	CXXRTL_ALWAYS_INLINE
	operator value<bits>() const {
		value<bits> ms_shifted = static_cast<const value<T::bits> &>(ms_expr)
			.template rzext<bits>();
		value<bits> ls_extended = static_cast<const value<U::bits> &>(ls_expr)
			.template zext<bits>();
		return ms_shifted.bit_or(ls_extended);
	}

	CXXRTL_ALWAYS_INLINE
	concat_expr<T, U> &operator=(const value<bits> &rhs) {
		ms_expr = rhs.template rtrunc<T::bits>();
		ls_expr = rhs.template trunc<U::bits>();
		return *this;
	}

	// A helper that forces the cast to value<>, which allows deduction to work.
	CXXRTL_ALWAYS_INLINE
	value<bits> val() const {
		return static_cast<const value<bits> &>(*this);
	}
};

// Base class for expression templates, providing helper methods for operations that are valid on both rvalues and lvalues.
//
// Note that expression objects (slices and concatenations) constructed in this way should NEVER be captured because
// they refer to temporaries that will, in general, only live until the end of the statement. For example, both of
// these snippets perform use-after-free:
//
//    const auto &a = val.slice<7,0>().slice<1>();
//    value<1> b = a;
//
//    auto &&c = val.slice<7,0>().slice<1>();
//    c = value<1>{1u};
//
// An easy way to write code using slices and concatenations safely is to follow two simple rules:
//   * Never explicitly name any type except `value<W>` or `const value<W> &`.
//   * Never use a `const auto &` or `auto &&` in any such expression.
// Then, any code that compiles will be well-defined.
template<class T>
struct expr_base {
	template<size_t Stop, size_t Start = Stop>
	CXXRTL_ALWAYS_INLINE
	slice_expr<const T, Stop, Start> slice() const {
		return {*static_cast<const T *>(this)};
	}

	template<size_t Stop, size_t Start = Stop>
	CXXRTL_ALWAYS_INLINE
	slice_expr<T, Stop, Start> slice() {
		return {*static_cast<T *>(this)};
	}

	template<class U>
	CXXRTL_ALWAYS_INLINE
	concat_expr<const T, typename std::remove_reference<const U>::type> concat(const U &other) const {
		return {*static_cast<const T *>(this), other};
	}

	template<class U>
	CXXRTL_ALWAYS_INLINE
	concat_expr<T, typename std::remove_reference<U>::type> concat(U &&other) {
		return {*static_cast<T *>(this), other};
	}
};

template<size_t Bits>
std::ostream &operator<<(std::ostream &os, const value<Bits> &val) {
	auto old_flags = os.flags(std::ios::right);
	auto old_width = os.width(0);
	auto old_fill  = os.fill('0');
	os << val.bits << '\'' << std::hex;
	for (size_t n = val.chunks - 1; n != (size_t)-1; n--) {
		if (n == val.chunks - 1 && Bits % value<Bits>::chunk::bits != 0)
			os.width((Bits % value<Bits>::chunk::bits + 3) / 4);
		else
			os.width((value<Bits>::chunk::bits + 3) / 4);
		os << val.data[n];
	}
	os.fill(old_fill);
	os.width(old_width);
	os.flags(old_flags);
	return os;
}

template<size_t Bits>
struct wire : public leakable {
	static constexpr size_t bits = Bits;
	// Here we provide the next leakset because verif is done before commit
	// the next leakset will be cleared and only the curr (after commit) ls wil be kept by symb_keep
	std::pair<Node*, leaks::LeakSet*> leak_single() const {
		return {next.node, next.ls};
	}
	std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>> leak_mem() const {
		assert(false && "Not a memory.");
		return std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>>();
	}

	value<Bits> curr;
	value<Bits> next;

	wire() = default;
	explicit constexpr wire(const value<Bits> &init) : curr(init), next(init) {}
	template<typename... Init>
	explicit constexpr wire(Init ...init) : curr{init...}, next{init...} {}

	// Copying and copy-assigning values is natural. If, however, a value is replaced with a wire,
	// e.g. because a module is built with a different optimization level, then existing code could
	// unintentionally copy a wire instead, which would create a subtle but serious bug. To make sure
	// this doesn't happen, prohibit copying and copy-assigning wires.
	wire(const wire<Bits> &) = delete;
	wire<Bits> &operator=(const wire<Bits> &) = delete;

	wire(wire<Bits> &&) = default;
	wire<Bits> &operator=(wire<Bits> &&) = default;

	template<class IntegerT>
	CXXRTL_ALWAYS_INLINE
	IntegerT get() const {
		return curr.template get<IntegerT>();
	}

	template<class IntegerT, bool setConstNode = false>
	CXXRTL_ALWAYS_INLINE
	void set(IntegerT other) {
		next.template set<IntegerT, setConstNode>(other);
	}

	CXXRTL_ALWAYS_INLINE
	Node* getNode() const {
		return curr.getNode();
	}

	CXXRTL_ALWAYS_INLINE
	void setNode(Node* node) {
		next.setNode(node);
	}

	// This method intentionally takes a mandatory argument (to make it more difficult to misuse in
	// black box implementations, leading to missed observer events). It is generic over its argument
	// to allow the `on_update` method to be non-virtual.
	template<class ObserverT>
	bool commit([[maybe_unused]] ObserverT &observer) {
		curr.debug_assert();
		next.debug_assert();
		assert(Bits == curr.node->width);
		assert(Bits == next.node->width);


		// Compute stability before affecting next to curr
		if (next.node->nature == CONST && curr.node->nature == CONST) {
			for (size_t i = 0; i < next.chunks; i++) {
				next.stability[i] = ~(next.data[i] ^ curr.data[i]);
			}
		} else {
			// Symbolic stability and partially symbolic stability
			// Try full comp first and otherwise resort to bit by bit comparison
			if (&simplify(*next.node) == &simplify(*curr.node)) {
				next.set_fully_stable();
			} else {
				for (size_t n = 0; n < next.chunks; n++) {
					chunk_t stab = 0;
					for (size_t i = 0; i < value<Bits>::chunk::bits; i++) {
						size_t bit = (n * value<Bits>::chunk::bits) + i;
						if (bit >= Bits)
							break;
						stab |= static_cast<chunk_t>(&simplify(Extract(bit, bit, *next.node)) == &simplify(Extract(bit, bit, *curr.node))) << i;
					}
					next.stability[n] = stab;
				}
			}
		}
		next.stability[next.chunks-1] &= next.msb_mask;

		// On_update and commit output cannot simply be used with symbolic data
		//if (!curr.concEq(next)) {
		//	observer.on_update(curr.chunks, curr.data, next.data);
		//	curr = next;
		//	return true;
		//}

		// TODO: There is a useless ls created here
		next.ls = leaks::merge(leaks::reg_stabilize(curr.node), leaks::reg_stabilize(next.node));
		curr = next;

		#ifdef MUST_BIT_DECOMPOSE
		assert((curr.ls == nullptr || curr.ls->leaks.size() == Bits) && "The leakset should always be the same size as the width.");
		assert((next.ls == nullptr || next.ls->leaks.size() == Bits) && "The leakset should always be the same size as the width.");
		#endif

		return false;
	}

	// We keep current node in any cases and next one only if it is an (input/output/constant/...)
	void symb_keep(bool toBeKept) {
		next.symb_keep(toBeKept);
		curr.symb_keep(true);
	}
};

template<size_t Bits>
std::ostream &operator<<(std::ostream &os, const wire<Bits> &val) {
	os << val.curr;
	return os;
}


template<size_t Width>
struct memory : public leakable {
	// vector of <starting index, last index, function on how to handle>
	std::vector<std::tuple<size_t, size_t, std::function<cxxrtl::value<Width>&(cxxrtl::value<Width>&)>>> funcArrays;
	const size_t depth;
	std::unique_ptr<value<Width>[]> data;
	std::pair<Node*, leaks::LeakSet*> leak_single() const {
		assert(false && "Memory should be accessed via mem.");
		return {data[0].node, data[0].ls};
	}
	std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>> leak_mem() const {
		std::set<std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>> res;
		// As we are before the commit here, we recompute it like it will be done later
		for (const write &entry : write_queue) {
			value<Width> prev = data[entry.index];
			value<Width> next = prev.update(entry.val, entry.mask);
			res.insert(std::tuple<size_t, std::pair<Node*, leaks::LeakSet*>, std::pair<Node*, leaks::LeakSet*>>(
				entry.index,
				std::pair<Node*, leaks::LeakSet*>(prev.node, prev.ls),
				std::pair<Node*, leaks::LeakSet*>(next.node, next.ls)
			));
		}
		return res;
	};

	explicit memory(size_t depth) : depth(depth), data(new value<Width>[depth]) {}

	memory(const memory<Width> &) = delete;
	memory<Width> &operator=(const memory<Width> &) = delete;

	memory(memory<Width> &&) = default;
	memory<Width> &operator=(memory<Width> &&other) {
		assert(depth == other.depth);
		data = std::move(other.data);
		write_queue = std::move(other.write_queue);
		return *this;
	}

	// An operator for direct memory reads. May be used at any time during the simulation.
	const value<Width> &operator [](size_t index) const {
		assert(index < depth);
		return data[index];
	}

	// An operator for direct memory writes. May only be used before the simulation is started. If used
	// after the simulation is started, the design may malfunction.
	value<Width> &operator [](size_t index) {
		assert(index < depth);
		return data[index];
	}

	value<Width> &operator [](value<32> index) {
		assert(index.data[0] < depth);
		for (std::tuple<size_t, size_t, std::function<cxxrtl::value<Width>&(cxxrtl::value<Width>&)>> func : funcArrays) {
			if (index.data[0] >= std::get<0>(func) && index.data[0] < std::get<1>(func)) {
				simulation_logger << "Read inside funcArray, triggering handle for read in: " << std::hex << index.data[0] << std::dec << ", index is const: " << ((index.node->nature == CONST) ? "yes" : "no") << std::endl;
				return std::get<2>(func)(index);
			}
		}
		return data[index.data[0]];
	}

	// A simple way to make a writable memory would be to use an array of wires instead of an array of values.
	// However, there are two significant downsides to this approach: first, it has large overhead (2× space
	// overhead, and O(depth) time overhead during commit); second, it does not simplify handling write port
	// priorities. Although in principle write ports could be ordered or conditionally enabled in generated
	// code based on their priorities and selected addresses, the feedback arc set problem is computationally
	// expensive, and the heuristic based algorithms are not easily modified to guarantee (rather than prefer)
	// a particular write port evaluation order.
	//
	// The approach used here instead is to queue writes into a buffer during the eval phase, then perform
	// the writes during the commit phase in the priority order. This approach has low overhead, with both space
	// and time proportional to the amount of write ports. Because virtually every memory in a practical design
	// has at most two write ports, linear search is used on every write, being the fastest and simplest approach.
	struct write {
		size_t index;
		value<Width> val;
		value<Width> mask;
		int priority;
	};
	std::vector<write> write_queue;

	// This will act as a permanent memory of which indexes have been modified
	// It is necessary if we want to keep track of leaksets that appear in the memory
	// without having to go through the whole memory
	// TODO: Would be nice to remove indexes and not add them when there is a const node or an empty ls
	std::set<size_t> written;

	void update(size_t index, const value<Width> &val, const value<Width> &mask, int priority = 0) {
		assert(index < depth);
		// Queue up the write while keeping the queue sorted by priority.
		write_queue.insert(
			std::upper_bound(write_queue.begin(), write_queue.end(), priority,
				[](const int a, const write& b) { return a < b.priority; }),
			write { index, val, mask, priority });

		// Don't add to written if it was a fully constant node
		//if (val.node->nature != CONST or val.ls != nullptr)
		written.insert(index);

		// If we do a full overwrite, remove the index as it is not anymore interesting to verify it
		// TODO: Be carefull as the manager keeps information in memory, not to invalidate pointers
		//if (val.node->nature == CONST and val.ls == nullptr and (mask.data[0] == value<Width>::chunk::mask) and written.contains(index))
		//	written.erase(index);

		for (auto& func : funcArrays) {
			if (index >= std::get<0>(func) && index < std::get<1>(func)) {
				simulation_logger << "Wrote in funcArray, this may have no effect but as it was with non cxxrtl::value index, bail out:" << std::hex << std::get<0>(func) << ", " << std::get<1>(func) << ", " << index << std::dec << std::endl;
				std::abort();
			}
		}
	}
	void update(const value<32>& index, const value<Width> &val, const value<Width> &mask, int priority = 0) {
		assert(index.data[0] < depth);
		// Queue up the write while keeping the queue sorted by priority.
		write_queue.insert(
			std::upper_bound(write_queue.begin(), write_queue.end(), priority,
				[](const int a, const write& b) { return a < b.priority; }),
			write { index.data[0], val, mask, priority });

		// Don't add to written if it was a fully constant node
		//if (val.node->nature != CONST or val.ls != nullptr)
		written.insert(index.data[0]);

		// If we do a full overwrite, remove the index as it is not anymore interesting to verify it
		// TODO: Be carefull as the manager keeps information in memory, not to invalidate pointers
		//if (val.node->nature == CONST and val.ls == nullptr and (mask.data[0] == value<Width>::chunk::mask) and written.contains(index.data[0]))
		//	written.erase(index.data[0]);

		for (auto& func : funcArrays) {
			if (index.data[0] >= std::get<0>(func) && index.data[0] < std::get<1>(func)) {
				simulation_logger << "Wrote in funcArray, this will have no effect as read value will be overriden... HERE: " << std::hex << index.data[0] << std::dec << ", index is const: " << ((index.node->nature == CONST) ? "yes" : "no") << std::endl;
			}
		}
	}

	// See the note for `wire::commit()`.
	template<class ObserverT>
	bool commit(ObserverT &observer) {
		bool changed = false;
		for (const write &entry : write_queue) {
			value<Width> elem = data[entry.index];
			value<Width> elem_up = elem.update(entry.val, entry.mask);

			// Here, we want memory to be always stable, if a value is being read in
			// the same cycle it is written it may not be the real behaviour
			elem_up.stability[0] = value<Width>::chunk::mask;
			elem_up.ls = leaks::reg_stabilize(elem_up.node);

			if (!data[entry.index].concEq(elem_up)) {
				observer.on_update(value<Width>::chunks, data[0].data, elem_up.data, entry.index);
				changed |= true;
			}
			data[entry.index] = elem_up;
			elem_up.debug_assert();
		}
		write_queue.clear();
		return changed;
	}

	// Nothing to clear here, we just keep the leaksets of all ls in memory
	void symb_keep() {
		for (auto& index : written) {
			data[index].symb_keep(true);
		}
	}
};

struct metadata {
	const enum {
		MISSING = 0,
		UINT   	= 1,
		SINT   	= 2,
		STRING 	= 3,
		DOUBLE 	= 4,
	} value_type;

	// In debug mode, using the wrong .as_*() function will assert.
	// In release mode, using the wrong .as_*() function will safely return a default value.
	const uint64_t    uint_value = 0;
	const int64_t     sint_value = 0;
	const std::string string_value = "";
	const double      double_value = 0.0;

	metadata() : value_type(MISSING) {}
	metadata(uint64_t value) : value_type(UINT), uint_value(value) {}
	metadata(int64_t value) : value_type(SINT), sint_value(value) {}
	metadata(const std::string &value) : value_type(STRING), string_value(value) {}
	metadata(const char *value) : value_type(STRING), string_value(value) {}
	metadata(double value) : value_type(DOUBLE), double_value(value) {}

	metadata(const metadata &) = default;
	metadata &operator=(const metadata &) = delete;

	uint64_t as_uint() const {
		assert(value_type == UINT);
		return uint_value;
	}

	int64_t as_sint() const {
		assert(value_type == SINT);
		return sint_value;
	}

	const std::string &as_string() const {
		assert(value_type == STRING);
		return string_value;
	}

	double as_double() const {
		assert(value_type == DOUBLE);
		return double_value;
	}

	// Internal CXXRTL use only.
	static std::map<std::string, metadata> deserialize(const char *ptr) {
		std::map<std::string, metadata> result;
		std::string name;
		// Grammar:
		// string   ::= [^\0]+ \0
		// metadata ::= [uid] .{8} | s <string>
		// map      ::= ( <string> <metadata> )* \0
		for (;;) {
			if (*ptr) {
				name += *ptr++;
			} else if (!name.empty()) {
				ptr++;
				auto get_u64 = [&]() {
					uint64_t result = 0;
					for (size_t count = 0; count < 8; count++)
						result = (result << 8) | *ptr++;
					return result;
				};
				char type = *ptr++;
				if (type == 'u') {
					uint64_t value = get_u64();
					result.emplace(name, value);
				} else if (type == 'i') {
					int64_t value = (int64_t)get_u64();
					result.emplace(name, value);
				} else if (type == 'd') {
					double dvalue;
					uint64_t uvalue = get_u64();
					static_assert(sizeof(dvalue) == sizeof(uvalue), "double must be 64 bits in size");
					memcpy(&dvalue, &uvalue, sizeof(dvalue));
					result.emplace(name, dvalue);
				} else if (type == 's') {
					std::string value;
					while (*ptr)
						value += *ptr++;
					ptr++;
					result.emplace(name, value);
				} else {
					assert(false && "Unknown type specifier");
					return result;
				}
				name.clear();
			} else {
				return result;
			}
		}
	}
};

typedef std::map<std::string, metadata> metadata_map;

struct performer;

// An object that allows formatting a string lazily.
struct lazy_fmt {
	virtual std::string operator() () const = 0;
};

// Flavor of a `$check` cell.
enum class flavor {
	// Corresponds to a `$assert` cell in other flows, and a Verilog `assert ()` statement.
	ASSERT,
	// Corresponds to a `$assume` cell in other flows, and a Verilog `assume ()` statement.
	ASSUME,
	// Corresponds to a `$live` cell in other flows, and a Verilog `assert (eventually)` statement.
	ASSERT_EVENTUALLY,
	// Corresponds to a `$fair` cell in other flows, and a Verilog `assume (eventually)` statement.
	ASSUME_EVENTUALLY,
	// Corresponds to a `$cover` cell in other flows, and a Verilog `cover ()` statement.
	COVER,
};

// An object that can be passed to a `eval()` method in order to act on side effects. The default behavior implemented
// below is the same as the behavior of `eval(nullptr)`, except that `-print-output` option of `write_cxxrtl` is not
// taken into account.
struct performer {
	// Called by generated formatting code to evaluate a Verilog `$time` expression.
	virtual int64_t vlog_time() const { return 0; }

	// Called by generated formatting code to evaluate a Verilog `$realtime` expression.
	virtual double vlog_realtime() const { return vlog_time(); }

	// Called when a `$print` cell is triggered.
	virtual void on_print(const lazy_fmt &formatter, [[maybe_unused]] const metadata_map &attributes) {
		simulation_logger << formatter();
	}

	// Called when a `$check` cell is triggered.
	virtual void on_check(flavor type, bool condition, const lazy_fmt &formatter, [[maybe_unused]] const metadata_map &attributes) {
		if (type == flavor::ASSERT || type == flavor::ASSUME) {
			if (!condition)
				simulation_logger << formatter();
			CXXRTL_ASSERT(condition && "Check failed");
		}
	}
};

// An object that can be passed to a `commit()` method in order to produce a replay log of every state change in
// the simulation. Unlike `performer`, `observer` does not use virtual calls as their overhead is unacceptable, and
// a comparatively heavyweight template-based solution is justified.
struct observer {
	// Called when the `commit()` method for a wire is about to update the `chunks` chunks at `base` with `chunks` chunks
	// at `value` that have a different bit pattern. It is guaranteed that `chunks` is equal to the wire chunk count and
	// `base` points to the first chunk.
	void on_update([[maybe_unused]] size_t chunks, [[maybe_unused]] const chunk_t *base, [[maybe_unused]] const chunk_t *value) {}

	// Called when the `commit()` method for a memory is about to update the `chunks` chunks at `&base[chunks * index]`
	// with `chunks` chunks at `value` that have a different bit pattern. It is guaranteed that `chunks` is equal to
	// the memory element chunk count and `base` points to the first chunk of the first element of the memory.
	void on_update([[maybe_unused]] size_t chunks, [[maybe_unused]] const chunk_t *base, [[maybe_unused]] const chunk_t *value, [[maybe_unused]] size_t index) {}
};

// Must be kept in sync with `struct FmtPart` in kernel/fmt.h!
// Default member initializers would make this a non-aggregate-type in C++11, so they are commented out.
struct fmt_part {
	enum {
		LITERAL   = 0,
		INTEGER   = 1,
		STRING    = 2,
		UNICHAR   = 3,
		VLOG_TIME = 4,
	} type;

	// LITERAL type
	std::string str;

	// INTEGER/STRING/UNICHAR types
	// + value<Bits> val;

	// INTEGER/STRING/VLOG_TIME types
	enum {
		RIGHT	= 0,
		LEFT	= 1,
		NUMERIC	= 2,
	} justify; // = RIGHT;
	char padding; // = '\0';
	size_t width; // = 0;

	// INTEGER type
	unsigned base; // = 10;
	bool signed_; // = false;
	enum {
		MINUS		= 0,
		PLUS_MINUS	= 1,
		SPACE_MINUS	= 2,
	} sign; // = MINUS;
	bool hex_upper; // = false;
	bool show_base; // = false;
	bool group; // = false;

	// VLOG_TIME type
	bool realtime; // = false;
	// + int64_t itime;
	// + double ftime;

	// Format the part as a string.
	//
	// The values of `vlog_time` and `vlog_realtime` are used for Verilog `$time` and `$realtime`, correspondingly.
	template<size_t Bits>
	std::string render(value<Bits> val, performer *performer = nullptr)
	{
		// We might want to replace some of these bit() calls with direct
		// chunk access if it turns out to be slow enough to matter.
		std::string buf;
		std::string prefix;
		switch (type) {
			case LITERAL:
				return str;

			case STRING: {
				buf.reserve(Bits/8);
				for (int i = 0; i < Bits; i += 8) {
					char ch = 0;
					for (int j = 0; j < 8 && i + j < int(Bits); j++)
						if (val.bit(i + j))
							ch |= 1 << j;
					if (ch != 0)
						buf.append({ch});
				}
				std::reverse(buf.begin(), buf.end());
				break;
			}

			case UNICHAR: {
				uint32_t codepoint = val.template get<uint32_t>();
				if (codepoint >= 0x10000)
					buf += (char)(0xf0 |  (codepoint >> 18));
				else if (codepoint >= 0x800)
					buf += (char)(0xe0 |  (codepoint >> 12));
				else if (codepoint >= 0x80)
					buf += (char)(0xc0 |  (codepoint >>  6));
				else
					buf += (char)codepoint;
				if (codepoint >= 0x10000)
					buf += (char)(0x80 | ((codepoint >> 12) & 0x3f));
				if (codepoint >= 0x800)
					buf += (char)(0x80 | ((codepoint >>  6) & 0x3f));
				if (codepoint >= 0x80)
					buf += (char)(0x80 | ((codepoint >>  0) & 0x3f));
				break;
			}

			case INTEGER: {
				bool negative = signed_ && val.is_neg();
				if (negative) {
					prefix = "-";
					val = val.neg();
				} else {
					switch (sign) {
						case MINUS:       break;
						case PLUS_MINUS:  prefix = "+"; break;
						case SPACE_MINUS: prefix = " "; break;
					}
				}

				size_t val_width = Bits;
				if (base != 10) {
					val_width = 1;
					for (size_t index = 0; index < Bits; index++)
						if (val.bit(index))
							val_width = index + 1;
				}

				if (base == 2) {
					if (show_base)
						prefix += "0b";
					for (size_t index = 0; index < val_width; index++) {
						if (group && index > 0 && index % 4 == 0)
							buf += '_';
						buf += (val.bit(index) ? '1' : '0');
					}
				} else if (base == 8 || base == 16) {
					if (show_base)
						prefix += (base == 16) ? (hex_upper ? "0X" : "0x") : "0o";
					size_t step = (base == 16) ? 4 : 3;
					for (size_t index = 0; index < val_width; index += step) {
						if (group && index > 0 && index % (4 * step) == 0)
							buf += '_';
						uint8_t value = val.bit(index) | (val.bit(index + 1) << 1) | (val.bit(index + 2) << 2);
						if (step == 4)
							value |= val.bit(index + 3) << 3;
						buf += (hex_upper ? "0123456789ABCDEF" : "0123456789abcdef")[value];
					}
				} else if (base == 10) {
					if (show_base)
						prefix += "0d";
					if (val.is_zero())
						buf += '0';
					value<(Bits > 4 ? Bits : 4)> xval = val.template zext<(Bits > 4 ? Bits : 4)>();
					size_t index = 0;
					while (!xval.is_zero()) {
						if (group && index > 0 && index % 3 == 0)
							buf += '_';
						value<(Bits > 4 ? Bits : 4)> quotient, remainder;
						if (Bits >= 4)
							std::tie(quotient, remainder) = xval.udivmod(value<(Bits > 4 ? Bits : 4)>{10u});
						else
							std::tie(quotient, remainder) = std::make_pair(value<(Bits > 4 ? Bits : 4)>{0u}, xval);
						buf += '0' + remainder.template trunc<4>().template get<uint8_t>();
						xval = quotient;
						index++;
					}
				} else assert(false && "Unsupported base for fmt_part");
				if (justify == NUMERIC && group && padding == '0') {
					int group_size = base == 10 ? 3 : 4;
					while (prefix.size() + buf.size() < width) {
						if (buf.size() % (group_size + 1) == group_size)
							buf += '_';
						buf += '0';
					}
				}
				std::reverse(buf.begin(), buf.end());
				break;
			}

			case VLOG_TIME: {
				if (performer) {
					buf = realtime ? std::to_string(performer->vlog_realtime()) : std::to_string(performer->vlog_time());
				} else {
					buf = realtime ? std::to_string(0.0) : std::to_string(0);
				}
				break;
			}
		}

		std::string str;
		assert(width == 0 || padding != '\0');
		if (prefix.size() + buf.size() < width) {
			size_t pad_width = width - prefix.size() - buf.size();
			switch (justify) {
				case LEFT:
					str += prefix;
					str += buf;
					str += std::string(pad_width, padding);
					break;
				case RIGHT:
					str += std::string(pad_width, padding);
					str += prefix;
					str += buf;
					break;
				case NUMERIC:
					str += prefix;
					str += std::string(pad_width, padding);
					str += buf;
					break;
				}
		} else {
			str += prefix;
			str += buf;
		}
		return str;
	}
};

// Tag class to disambiguate values/wires and their aliases.
struct debug_alias {};

// Tag declaration to disambiguate values and debug outlines.
using debug_outline = ::_cxxrtl_outline;

// This structure is intended for consumption via foreign function interfaces, like Python's ctypes.
// Because of this it uses a C-style layout that is easy to parse rather than more idiomatic C++.
//
// To avoid violating strict aliasing rules, this structure has to be a subclass of the one used
// in the C API, or it would not be possible to cast between the pointers to these.
//
// The `attrs` member cannot be owned by this structure because a `cxxrtl_object` can be created
// from external C code.
struct debug_item : ::cxxrtl_object {
	// Object types.
	enum : uint32_t {
		VALUE   = CXXRTL_VALUE,
		WIRE    = CXXRTL_WIRE,
		MEMORY  = CXXRTL_MEMORY,
		ALIAS   = CXXRTL_ALIAS,
		OUTLINE = CXXRTL_OUTLINE,
	};

	// Object flags.
	enum : uint32_t {
		INPUT  = CXXRTL_INPUT,
		OUTPUT = CXXRTL_OUTPUT,
		INOUT  = CXXRTL_INOUT,
		DRIVEN_SYNC = CXXRTL_DRIVEN_SYNC,
		DRIVEN_COMB = CXXRTL_DRIVEN_COMB,
		UNDRIVEN    = CXXRTL_UNDRIVEN,
	};

	const leakable* leakref = nullptr;

	debug_item(const ::cxxrtl_object &object) : cxxrtl_object(object) {}

	template<size_t Bits>
	debug_item(value<Bits> &item, size_t lsb_offset = 0, uint32_t flags_ = 0) {
//		static_assert(Bits == 0 || sizeof(item) == value<Bits>::chunks * sizeof(chunk_t),
//		              "value<Bits> is not compatible with C layout");
		type    = VALUE;
		flags   = flags_;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = item.data;
		next    = item.data;
		stab    = item.stability;
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Bits>
	debug_item(const value<Bits> &item, size_t lsb_offset = 0) {
//		static_assert(Bits == 0 || sizeof(item) == value<Bits>::chunks * sizeof(chunk_t),
//		              "value<Bits> is not compatible with C layout");
		type    = VALUE;
		flags   = DRIVEN_COMB;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = const_cast<chunk_t*>(item.data);
		next    = nullptr;
		stab    = const_cast<chunk_t*>(item.stability);
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Bits>
	debug_item(wire<Bits> &item, size_t lsb_offset = 0, uint32_t flags_ = 0) {
//		static_assert(Bits == 0 ||
//		              (sizeof(item.curr) == value<Bits>::chunks * sizeof(chunk_t) &&
//		               sizeof(item.next) == value<Bits>::chunks * sizeof(chunk_t)),
//		              "wire<Bits> is not compatible with C layout");
		type    = WIRE;
		flags   = flags_;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = item.curr.data;
		next    = item.next.data;
		stab    = item.next.stability; // This will be requested pre-commit thus next
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Width>
	debug_item(memory<Width> &item, size_t zero_offset = 0) {
//		static_assert(Width == 0 || sizeof(item.data[0]) == value<Width>::chunks * sizeof(chunk_t),
//		              "memory<Width> is not compatible with C layout");
		type    = MEMORY;
		flags   = 0;
		width   = Width;
		lsb_at  = 0;
		depth   = item.depth;
		zero_at = zero_offset;
		curr    = item.data ? item.data[0].data : nullptr;
		next    = nullptr;
		stab    = item.data ? item.data[0].stability : nullptr;
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Bits>
	debug_item(debug_alias, const value<Bits> &item, size_t lsb_offset = 0) {
//		static_assert(Bits == 0 || sizeof(item) == value<Bits>::chunks * sizeof(chunk_t),
//		              "value<Bits> is not compatible with C layout");
		type    = ALIAS;
		flags   = DRIVEN_COMB;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = const_cast<chunk_t*>(item.data);
		next    = nullptr;
		stab    = const_cast<chunk_t*>(item.stability);
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Bits>
	debug_item(debug_alias, const wire<Bits> &item, size_t lsb_offset = 0) {
//		static_assert(Bits == 0 ||
//		              (sizeof(item.curr) == value<Bits>::chunks * sizeof(chunk_t) &&
//		               sizeof(item.next) == value<Bits>::chunks * sizeof(chunk_t)),
//		              "wire<Bits> is not compatible with C layout");
		type    = ALIAS;
		flags   = DRIVEN_COMB;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = const_cast<chunk_t*>(item.curr.data);
		next    = nullptr;
		stab    = const_cast<chunk_t*>(item.curr.stability);
		outline = nullptr;
		attrs   = nullptr;
		leakref = &item;
	}

	template<size_t Bits>
	debug_item(debug_outline &group, const value<Bits> &item, size_t lsb_offset = 0) {
//		static_assert(Bits == 0 || sizeof(item) == value<Bits>::chunks * sizeof(chunk_t),
//		              "value<Bits> is not compatible with C layout");
		type    = OUTLINE;
		flags   = DRIVEN_COMB;
		width   = Bits;
		lsb_at  = lsb_offset;
		depth   = 1;
		zero_at = 0;
		curr    = const_cast<chunk_t*>(item.data);
		next    = nullptr;
		stab    = const_cast<chunk_t*>(item.stability);
		outline = &group;
		attrs   = nullptr;
		leakref = nullptr;
	}

	template<size_t Bits, class IntegerT>
	IntegerT get() const {
		assert(width == Bits && depth == 1);
		value<Bits> item;
		std::copy(curr, curr + value<Bits>::chunks, item.data);
		return item.template get<IntegerT>();
	}

	template<size_t Bits, class IntegerT>
	void set(IntegerT other) const {
		assert(width == Bits && depth == 1);
		value<Bits> item;
		item.template set<IntegerT>(other);
		std::copy(item.data, item.data + value<Bits>::chunks, next);
	}
};
//static_assert(std::is_standard_layout<debug_item>::value, "debug_item is not compatible with C layout");

} // namespace cxxrtl

typedef struct _cxxrtl_attr_set {
	cxxrtl::metadata_map map;
} *cxxrtl_attr_set;

namespace cxxrtl {

// Representation of an attribute set in the C++ interface.
using debug_attrs = ::_cxxrtl_attr_set;

struct debug_items {
	// Debug items may be composed of multiple parts, but the attributes are shared between all of them.
	// There are additional invariants, not all of which are not checked by this code:
	// - Memories and non-memories cannot be mixed together.
	// - Bit indices (considering `lsb_at` and `width`) must not overlap.
	// - Row indices (considering `depth` and `zero_at`) must be the same.
	// - The `INPUT` and `OUTPUT` flags must be the same for all parts.
	// Other than that, the parts can be quite different, e.g. it is OK to mix a value, a wire, an alias,
	// and an outline, in the debug information for a single name in four parts.
	std::map<std::string, std::vector<debug_item>> table;
	std::map<std::string, std::unique_ptr<debug_attrs>> attrs_table;

	void add(const std::string &path, debug_item &&item, metadata_map &&item_attrs = {}) {
		// assert((path.empty() || path[path.size() - 1] != ' ') && path.find("  ") == std::string::npos);
		assert(path.empty() || path[path.size() - 1] != ' ');
		std::string aggreg_path = path;
		size_t idx = aggreg_path.find('@');
		if (idx != std::string::npos) {
			// We should merge this item, we suppose only the items we want to merge contains '@'
			// We also suppose they are correctly formed
			[[maybe_unused]] size_t lsb_at = std::stoi(aggreg_path.substr(idx+1, aggreg_path.size()-1));
			assert(lsb_at == item.lsb_at);
			aggreg_path = aggreg_path.substr(0, idx);
		}
		std::unique_ptr<debug_attrs> &attrs = attrs_table[aggreg_path];
		if (attrs.get() == nullptr)
			attrs = std::unique_ptr<debug_attrs>(new debug_attrs);
		for (auto attr : item_attrs)
			attrs->map.insert(attr);
		item.attrs = attrs.get();
		std::vector<debug_item> &parts = table[aggreg_path];
		parts.emplace_back(item);
		std::sort(parts.begin(), parts.end(),
			[](const debug_item &a, const debug_item &b) {
				return a.lsb_at < b.lsb_at;
			});
	}

	// This overload exists to reduce excessive stack slot allocation in `CXXRTL_EXTREMELY_COLD void debug_info()`.
	template<class... T>
	void add(const std::string &base_path, const char *path, const char *serialized_item_attrs, T&&... args) {
		add(base_path + path, debug_item(std::forward<T>(args)...), metadata::deserialize(serialized_item_attrs));
	}

	size_t count(const std::string &path) const {
		if (table.count(path) == 0)
			return 0;
		return table.at(path).size();
	}

	const std::vector<debug_item> &at(const std::string &path) const {
		return table.at(path);
	}

	// Like `at()`, but operates only on single-part debug items.
	const debug_item &operator [](const std::string &path) const {
		const std::vector<debug_item> &parts = table.at(path);
		assert(parts.size() == 1);
		return parts.at(0);
	}

	bool is_memory(const std::string &path) const {
		return at(path).at(0).type == debug_item::MEMORY;
	}

	const metadata_map &attrs(const std::string &path) const {
		return attrs_table.at(path)->map;
	}
};

// Only `module` scopes are defined. The type is implicit, since Yosys does not currently support
// any other scope types.
struct debug_scope {
	std::string module_name;
	std::unique_ptr<debug_attrs> module_attrs;
	std::unique_ptr<debug_attrs> cell_attrs;
};

struct debug_scopes {
	std::map<std::string, debug_scope> table;

	void add(const std::string &path, const std::string &module_name, metadata_map &&module_attrs, metadata_map &&cell_attrs) {
		// assert((path.empty() || path[path.size() - 1] != ' ') && path.find("  ") == std::string::npos);
		assert(path.empty() || path[path.size() - 1] != ' ');
		assert(table.count(path) == 0);
		debug_scope &scope = table[path];
		scope.module_name = module_name;
		scope.module_attrs = std::unique_ptr<debug_attrs>(new debug_attrs { module_attrs });
		scope.cell_attrs = std::unique_ptr<debug_attrs>(new debug_attrs { cell_attrs });
	}

	// This overload exists to reduce excessive stack slot allocation in `CXXRTL_EXTREMELY_COLD void debug_info()`.
	void add(const std::string &base_path, const char *path, const char *module_name, const char *serialized_module_attrs, const char *serialized_cell_attrs) {
		add(base_path + path, module_name, metadata::deserialize(serialized_module_attrs), metadata::deserialize(serialized_cell_attrs));
	}

	size_t contains(const std::string &path) const {
		return table.count(path);
	}

	const debug_scope &operator [](const std::string &path) const {
		return table.at(path);
	}
};

// Tag class to disambiguate the default constructor used by the toplevel module that calls `reset()`,
// and the constructor of interior modules that should not call it.
struct interior {};

// The core API of the `module` class consists of only four virtual methods: `reset()`, `eval()`,
// `commit`, and `debug_info()`. (The virtual destructor is made necessary by C++.) Every other method
// is a convenience method, and exists solely to simplify some common pattern for C++ API consumers.
// No behavior may be added to such convenience methods that other parts of CXXRTL can rely on, since
// there is no guarantee they will be called (and, for example, other CXXRTL libraries will often call
// the `eval()` and `commit()` directly instead, as well as being exposed in the C API).
struct module {
	module() {}
	virtual ~module() {}

	// Modules with black boxes cannot be copied. Although not all designs include black boxes,
	// delete the copy constructor and copy assignment operator to make sure that any downstream
	// code that manipulates modules doesn't accidentally depend on their availability.
	module(const module &) = delete;
	module &operator=(const module &) = delete;

	module(module &&) = default;
	module &operator=(module &&) = default;

	virtual void reset() = 0;

	// The `eval()` callback object, `performer`, is included in the virtual call signature since
	// the generated code has broadly identical performance properties.
	virtual bool eval(performer *performer = nullptr) = 0;

	// The `commit()` callback object, `observer`, is not included in the virtual call signature since
	// the generated code is severely pessimized by it. To observe commit events, the non-virtual
	// `commit(observer *)` overload must be called directly on a `module` subclass.
	virtual bool commit() = 0;

	// The symb_keep method must be defined when print_symb was used to generate module
	virtual void symb_keep() = 0;

	size_t step(performer *performer = nullptr) {
		size_t deltas = 0;
		bool converged = false;
		do {
			converged = eval(performer);
			deltas++;
		} while (commit() && !converged);
		return deltas;
	}

	virtual void debug_info(debug_items *items, debug_scopes *scopes, std::string path, metadata_map &&cell_attrs = {}) {
		(void)items, (void)scopes, (void)path, (void)cell_attrs;
	}

	// Compatibility method.
#if __has_attribute(deprecated)
	__attribute__((deprecated("Use `debug_info(path, &items, /*scopes=*/nullptr);` instead. (`path` could be \"top \".)")))
#endif
	void debug_info(debug_items &items, std::string path) {
		debug_info(&items, /*scopes=*/nullptr, path);
	}
};

} // namespace cxxrtl

// Internal structures used to communicate with the implementation of the C interface.

typedef struct _cxxrtl_toplevel {
	std::unique_ptr<cxxrtl::module> module;
} *cxxrtl_toplevel;

typedef struct _cxxrtl_outline {
	std::function<void()> eval;
} *cxxrtl_outline;

// Definitions of internal Yosys cells. Other than the functions in this namespace, CXXRTL is fully generic
// and indepenent of Yosys implementation details.
//
// The `write_cxxrtl` pass translates internal cells (cells with names that start with `$`) to calls of these
// functions. All of Yosys arithmetic and logical cells perform sign or zero extension on their operands,
// whereas basic operations on arbitrary width values require operands to be of the same width. These functions
// bridge the gap by performing the necessary casts. They are named similar to `cell_A[B]`, where A and B are `u`
// if the corresponding operand is unsigned, and `s` if it is signed.
namespace cxxrtl_yosys {

using namespace cxxrtl;

// std::max isn't constexpr until C++14 for no particular reason (it's an oversight), so we define our own.
template<class T>
CXXRTL_ALWAYS_INLINE
constexpr T max(const T &a, const T &b) {
	return a > b ? a : b;
}


// Symb mux is added to enable glitch detection in muxes
template<size_t BitsY>
CXXRTL_ALWAYS_INLINE
value<BitsY> symb_mux(const value<1>& sel, const value<BitsY>& b, const value<BitsY>& c) {
	// Here the stability is also copied, we later erase it if the selector is not stable
	value<BitsY> res = (!sel.Concis_zero() ? b : c); // It is ok to use concis_zero because we handeled this concretisation
	if (sel.node->nature != CONST) {
		simulation_logger << "Fixed: Muxing on a non conc selector, preventing concretisation." << std::endl;
		value<BitsY> maskSelector = sel.repeat<BitsY>();

		res.node = &simplify((*maskSelector.node & *b.node) | ((~*maskSelector.node) & *c.node));
	}

	if (sel.ls != nullptr and sel.stability[0] == 0x1u and res.node->nature == CONST) {
		//simulation_logger << "Fixed: mux: lsconc stable sel conc node." << std::endl;
		res.ls = leaks::merge(leaks::replicate(sel.ls, BitsY), (!sel.Concis_zero() ? b.ls : c.ls));
	} else if (sel.ls != nullptr and sel.stability[0] == 0x0u and res.node->nature == CONST) {
		//simulation_logger << "Fixed: mux: lsconc unstable sel conc node." << std::endl;
		res.ls = leaks::merge(leaks::replicate(sel.ls, BitsY), leaks::merge(b.ls, c.ls)); // Not optimal
	} else if (sel.node->nature != CONST) {
		//simulation_logger << "Fixed: mux: lsconc because of unconst node." << std::endl;
		res.ls = leaks::merge(b.ls, c.ls);
	} else if (sel.stability[0] == 0x0u) {
		res.ls = leaks::merge(b.ls, c.ls);
	}
	// Otherwise: Sel is conc and stable and has no leakset

	// If selector is stable, the output leakset is only the selected one
	// Otherwise, we merge both input leaksets
	if (sel.stability[0] == 0x0u) {
		// If some bits of both sides are the same and are both stable, corresponding bits of output are stable
		if (b.node->nature == CONST && c.node->nature == CONST) {
			for (size_t i = 0; i < res.chunks; i++) {
				res.stability[i] = ~(b.data[i] ^ c.data[i]) & b.stability[i] & c.stability[i];
			}
		} else {
			for (size_t n = 0; n < res.chunks; n++) {
				chunk_t stab = 0;
				for (size_t i = 0; i < value<BitsY>::chunk::bits; i++) {
					size_t bit = (n * value<BitsY>::chunk::bits) + i;
					if (bit >= value<BitsY>::bits)
						break;
					stab |= static_cast<chunk_t>((b.stability[n] & c.stability[n] & (0x1 << i)) and &simplify(Extract(bit, bit, *b.node)) == &simplify(Extract(bit, bit, *c.node))) << i;
				}
				res.stability[n] = stab;
			}
		}
		res.stability[res.chunks-1] &= res.msb_mask;

		res.ls = leaks::partial_stabilize(res.ls, res.node, res.stability);
	}

	// If selector is stable, conc and has no leakset, mux is just a passthrough
	res.debug_assert();
	return res;
}

// Propagate ls normally
template<size_t BitsY>
value<BitsY> symb_register(const value<BitsY>& a) {
	value<BitsY> res{a};
	return res;
}

// Logic operations
template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> logic_not(const value<BitsA> &a) {
	static_assert(BitsY == 1);
	value<BitsY> tmp = value<BitsY> { a.Concis_zero() ? 1u : 0u };

	// If any bit equals to one, result is 0. So, reduce_or then not the result
	if (a.node->nature != CONST) {
		simulation_logger << "Fixed: logic_not: Will comp to 0" << std::endl;
		Node* res = &simplify(Extract(0, 0, *a.node));
		for (int i = 1; i < a.node->width; i++) {
			res = &simplify(*res | Extract(i, i, *a.node));
			// We got a conc counter-example, we can conclude now (a bit was const and equal to one so result is const 0)
			if (res->nature == CONST and res->cst[0] == 0x1u)
				break;
		}
		tmp.node = &simplify(~(*res));
	}

	// Stability part: if a is fully stable then it is stable.
	// Otherwise, if there is any bit that is const, whose value is 1 and that is stable, the result is a stable 0
	bool stable = a.is_fully_stable();
	if (a.node->nature == CONST and not stable) {
		for (size_t i = 0; i < a.chunks; i++) {
			if ((a.data[i]) & a.stability[i]) {
				stable = true;
				break;
			}
		}
	} else if (not stable) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x1u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits are needed to compute the logic node so put them all in the result part
	if (a.ls != nullptr) {
		//simulation_logger << "Fixed: logic_not: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce(a.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> logic_and(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1 and BitsA == BitsB);
	value<BitsY> tmp = value<BitsY> { (not a.Concis_zero() && not b.Concis_zero()) ? 1u : 0u };

	// If either is not a constant, compute node
	if (a.node->nature != CONST or b.node->nature != CONST) {
		simulation_logger << "Fixed: logic_and: Will comp to 0" << std::endl;

		// logic and is the bitwise and between the reduce_or of operands a and b
		Node* resa = &simplify(Extract(0, 0, *a.node));
		Node* resb = &simplify(Extract(0, 0, *b.node));
		// We asserted that they are the same size
		for (size_t i = 1; i < BitsA; i++) {
			resa = &simplify(*resa | Extract(i, i, *a.node));
			resb = &simplify(*resb | Extract(i, i, *b.node));
			// If both temporarly computed results are const and equal to one, the result will be 0 (after the not) so stop
			if (resa->nature == CONST and resa->cst[0] == 0x1u and resb->nature == CONST and resb->cst[0] == 0x1u)
				break;
		}
		tmp.node = &simplify(*resa & *resb);
	}

	// If both operands are stable result is stable
	// If we find a bit in each operant (not necessarly the same) that is const, equal to 1 and is stable, we can conclude on the stability
	bool stableA = a.is_fully_stable();
	bool stableB = b.is_fully_stable();
	if (a.node->nature == CONST and not stableA) {
		for (size_t i = 0; i < a.chunks; i++) {
			if ((a.data[i]) & a.stability[i]) {
				stableA = true;
				break;
			}
		}
	}
	if (b.node->nature == CONST and not stableB) {
		for (size_t i = 0; i < b.chunks; i++) {
			if ((b.data[i]) & b.stability[i]) {
				stableB = true;
				break;
			}
		}
	}
	if (a.node->nature != CONST and not stableA) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x1u) {
						stableA = true;
						break;
					}
				}
			}
		}
	}
	if (b.node->nature != CONST and not stableB) {
		for (size_t n = 0; n < b.chunks; n++) {
			for (size_t i = 0; i < value<BitsB>::chunk::bits; i++) {
				size_t bit = (n * value<BitsB>::chunk::bits) + i;
				if (bit >= BitsB)
					break;
				if (b.stability[n] & (0x1 << i)) {
					Node* nb = &simplify(Extract(bit, bit, *b.node));
					if (nb->nature == CONST and nb->cst[0] == 0x1u) {
						stableB = true;
						break;
					}
				}
			}
		}
	}

	if (not stableA or not stableB)
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		simulation_logger << "Fixed: logic_and: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> logic_or(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1 and BitsA == BitsB);
	value<BitsY> tmp = value<BitsY> { (not a.Concis_zero() || not b.Concis_zero()) ? 1u : 0u };

	// logic and is the bitwise and between the reduce_or of operands a and b
	if (a.node->nature != CONST or b.node->nature != CONST) {
		simulation_logger << "Fixed: logic_or: Will comp to 0" << std::endl;

		Node* resa = &simplify(Extract(0, 0, *a.node));
		Node* resb = &simplify(Extract(0, 0, *b.node));
		// Size is equal, it was asserted
		for (size_t i = 1; i < BitsA; i++) {
			resa = &simplify(*resa | Extract(i, i, *a.node));
			resb = &simplify(*resb | Extract(i, i, *b.node));
			// If either of the temporarly computed results is const and equal to one, the result will be 1 so stop
			if ((resa->nature == CONST and resa->cst[0] == 0x1u) or (resb->nature == CONST and resb->cst[0] == 0x1u))
				break;
		}
		tmp.node = &simplify(*resa | *resb);
	}

	// If both operands are stable result is stable
	// If we find a bit in either operand that is const, equal to 1 and is stable, we can conclude on the stability
	bool stable = a.is_fully_stable() and b.is_fully_stable();
	if (a.node->nature == CONST and not stable) {
		for (size_t i = 0; i < a.chunks; i++) {
			if ((a.data[i]) & a.stability[i]) {
				stable = true;
				break;
			}
		}
	}
	if (b.node->nature == CONST and not stable) {
		for (size_t i = 0; i < b.chunks; i++) {
			if ((b.data[i]) & b.stability[i]) {
				stable = true;
				break;
			}
		}
	}
	if (a.node->nature != CONST and not stable) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x1u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (b.node->nature != CONST and not stable) {
		for (size_t n = 0; n < b.chunks; n++) {
			for (size_t i = 0; i < value<BitsB>::chunk::bits; i++) {
				size_t bit = (n * value<BitsB>::chunk::bits) + i;
				if (bit >= BitsB)
					break;
				if (b.stability[n] & (0x1 << i)) {
					Node* nb = &simplify(Extract(bit, bit, *b.node));
					if (nb->nature == CONST and nb->cst[0] == 0x1u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		simulation_logger << "Fixed: logic_or: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

// Reduction operations
template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> reduce_and(const value<BitsA> &a) {
	static_assert(BitsY == 1);
	value<BitsY> tmp = value<BitsY> { a.bit_not().Concis_zero() ? 1u : 0u };

	if (a.node->nature != CONST) {
		simulation_logger << "Fixed: reduce_and: Will comp to 0" << std::endl;
		Node* res = &simplify(Extract(0, 0, *a.node));
		for (int i = 1; i < a.node->width; i++) {
			res = &simplify(*res & Extract(i, i, *a.node));
			// If we find a bit that is const and equals 0, the and will be 0 so skip
			if (res->nature == CONST and res->cst[0] == 0x0u)
				break;
		}
		tmp.node = res;
	}

	// Stability part: if a is fully stable then it is stable.
	// Otherwise, if there is any bit that is const, whose value is 0 and that is stable, the result is a stable 0
	bool stable = a.is_fully_stable();
	if (a.node->nature == CONST and not stable) {
		for (size_t i = 0; i < a.chunks; i++) {
			if ((~a.data[i]) & a.stability[i]) {
				stable = true;
				break;
			}
		}
	}
	if (a.node->nature != CONST and not stable) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x0u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits are needed to compute the reduce node so put them all in the result part
	if (a.ls != nullptr) {
		simulation_logger << "Fixed: reduce_and: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce(a.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> reduce_or(const value<BitsA> &a) {
	static_assert(BitsY == 1);
	value<BitsY> tmp = value<BitsY> { a.Concis_zero() ? 0u : 1u };

	if (a.node->nature != CONST) {
		simulation_logger << "Fixed: reduce_or: Will comp to 0" << std::endl;
		Node* res = &simplify(Extract(0, 0, *a.node));
		for (int i = 1; i < a.node->width; i++) {
			res = &simplify(*res | Extract(i, i, *a.node));
			// If we find a bit that is const and equals 1, the and will be 1 so skip
			if (res->nature == CONST and res->cst[0] == 0x1u)
				break;
		}
		tmp.node = res;
	}

	// Stability part: if a is fully stable then it is stable.
	// Otherwise, if there is any bit that is const, whose value is 1 and that is stable, the result is a stable 1
	bool stable = a.is_fully_stable();
	if (a.node->nature == CONST and not stable) {
		for (size_t i = 0; i < a.chunks; i++) {
			if (a.data[i] & a.stability[i]) {
				stable = true;
				break;
			}
		}
	}
	if (a.node->nature != CONST and not stable) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x1u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits are needed to compute the reduce node so put them all in the result part
	if (a.ls != nullptr) {
		//simulation_logger << "Fixed: reduce_or: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce(a.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> reduce_xor(const value<BitsA> &a) {
	assert(a.node->nature == CONST && "reduce_xor is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr && "reduce_xor is currently not implemented for ls concretisations.");

	// There is a ctpop concretisation
	value<BitsY> tmp = value<BitsY> { (a.ctpop() % 2) ? 1u : 0u };

	if (not a.is_fully_stable())
		tmp.stability[0] = 0x0u;

	return tmp;
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> reduce_xnor(const value<BitsA> &a) {
	assert(a.node->nature == CONST && "reduce_xnor is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr && "reduce_xnor is currently not implemented for ls concretisations.");

	value<BitsY> tmp = value<BitsY> { (a.ctpop() % 2) ? 0u : 1u };

	if (not a.is_fully_stable())
		tmp.stability[0] = 0x0u;
	return tmp;
}

// Reduce_bool is very similar to reduce_or
template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> reduce_bool(const value<BitsA> &a) {
	static_assert(BitsY == 1);
	value<BitsY> tmp = value<BitsY> { a.Concis_zero() ? 0u : 1u };

	if (a.node->nature != CONST) {
		simulation_logger << "Fixed: reduce_bool: Will comp to 0" << std::endl;
		Node* res = &simplify(Extract(0, 0, *a.node));
		for (int i = 1; i < a.node->width; i++) {
			res = &simplify(*res | Extract(i, i, *a.node));
			// If we find a bit that is const and equals 1, the and will be 1 so skip
			if (res->nature == CONST and res->cst[0] == 0x1u)
				break;
		}
		tmp.node = res;
	}

	// Stability part: if a is fully stable then it is stable.
	// Otherwise, if there is any bit that is const, whose value is 1 and that is stable, the result is a stable 1
	bool stable = a.is_fully_stable();
	if (a.node->nature == CONST and not stable) {
		for (size_t i = 0; i < a.chunks; i++) {
			if (a.data[i] & a.stability[i]) {
				stable = true;
				break;
			}
		}
	}
	if (a.node->nature != CONST and not stable) {
		for (size_t n = 0; n < a.chunks; n++) {
			for (size_t i = 0; i < value<BitsA>::chunk::bits; i++) {
				size_t bit = (n * value<BitsA>::chunk::bits) + i;
				if (bit >= BitsA)
					break;
				if (a.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *a.node));
					if (na->nature == CONST and na->cst[0] == 0x1u) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits are needed to compute the reduce node so put them all in the result part
	if (a.ls != nullptr) {
		//simulation_logger << "Fixed: reduce_bool: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce(a.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

// Bitwise operations
template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> not_u(const value<BitsA> &a) {
	return a.template zcast<BitsY>().bit_not();
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> not_s(const value<BitsA> &a) {
	return a.template scast<BitsY>().bit_not();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> and_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().bit_and(b.template zcast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> and_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().bit_and(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> or_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().bit_or(b.template zcast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> or_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().bit_or(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> xor_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().bit_xor(b.template zcast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> xor_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().bit_xor(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> xnor_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().bit_xor(b.template zcast<BitsY>()).bit_not();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> xnor_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().bit_xor(b.template scast<BitsY>()).bit_not();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shl_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().shl(b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shl_su(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().shl(b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sshl_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().shl(b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sshl_su(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().shl(b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shr_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.shr(b).template zcast<BitsY>();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shr_su(const value<BitsA> &a, const value<BitsB> &b) {
	return a.shr(b).template scast<BitsY>();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sshr_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.shr(b).template zcast<BitsY>();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sshr_su(const value<BitsA> &a, const value<BitsB> &b) {
	return a.sshr(b).template scast<BitsY>();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shift_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return shr_uu<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shift_su(const value<BitsA> &a, const value<BitsB> &b) {
	return shr_su<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shift_us(const value<BitsA> &a, const value<BitsB> &b) {
	return b.is_neg() ? shl_uu<BitsY>(a, b.template sext<BitsB + 1>().neg()) : shr_uu<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shift_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return b.is_neg() ? shl_su<BitsY>(a, b.template sext<BitsB + 1>().neg()) : shr_su<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shiftx_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return shift_uu<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shiftx_su(const value<BitsA> &a, const value<BitsB> &b) {
	return shift_su<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shiftx_us(const value<BitsA> &a, const value<BitsB> &b) {
	return shift_us<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> shiftx_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return shift_ss<BitsY>(a, b);
}

// Comparison operations
template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> eq_uu(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	constexpr size_t BitsExt = max(BitsA, BitsB);

	value<BitsExt> ea = a.template zext<BitsExt>();
	value<BitsExt> eb = b.template zext<BitsExt>();

	value<BitsY> tmp = value<BitsY>{ ea.concEq(eb) ? 1u : 0u };

	if (a.node->nature != CONST || b.node->nature != CONST) {
		simulation_logger << "Fixed: eq_uu: Will ==" << std::endl;
		Node* xorc = &simplify((*ea.node) ^ (*eb.node));
		Node* res = &simplify(Extract(0, 0, *xorc));
		for (size_t i = 1; i < BitsExt; i++) {
			res = &simplify(*res | Extract(i, i, *xorc));
			// If we find a bit that is const and equal to 1 (it means we found two bits that are const and different), skip the rest result will be 0
			if (res->nature == CONST and res->cst[0] == 0x1u)
				break;
		}
		tmp.node = &simplify(~*res);
	}

	// If both operands are stable result is stable
	// If we find two bits of the same rank in both operand that are const, not equal and stable, we can conclude on the stability
	bool stable = a.is_fully_stable() and b.is_fully_stable();
	if (a.node->nature == CONST and b.node->nature == CONST and not stable) {
		for (size_t i = 0; i < ea.chunks; i++) {
			if (ea.stability[i] & eb.stability[i] & (ea.data[i] ^ eb.data[i])) {
				stable = true;
				break;
			}
		}
	}
	if ((ea.node->nature != CONST or eb.node->nature != CONST) and not stable) {
		for (size_t n = 0; n < ea.chunks; n++) {
			for (size_t i = 0; i < value<BitsExt>::chunk::bits; i++) {
				size_t bit = (n * value<BitsExt>::chunk::bits) + i;
				if (bit >= BitsExt)
					break;
				if (a.stability[n] & b.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *ea.node));
					Node* nb = &simplify(Extract(bit, bit, *eb.node));
					if (na->nature == CONST and nb->nature == CONST and na->cst[0] != nb->cst[0]) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: eq_uu: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> eq_ss(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	assert(a.node->nature == CONST and b.node->nature == CONST && "eq_ss is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr and b.ls == nullptr && "eq_ss is currently not implemented for ls concretisations.");

	constexpr size_t BitsExt = max(BitsA, BitsB);

	value<BitsExt> ea = a.template sext<BitsExt>();
	value<BitsExt> eb = b.template sext<BitsExt>();

	value<BitsY> tmp = value<BitsY>{ ea == eb ? 1u : 0u };

	// If we can find two bits of the same rank that are stable and not equal, they drive the result
	bool stable = a.is_fully_stable() and b.is_fully_stable();
	if (a.node->nature == CONST and b.node->nature == CONST and not stable) {
		for (size_t i = 0; i < ea.chunks; i++) {
			if (ea.stability[i] & eb.stability[i] & (ea.data[i] ^ eb.data[i])) {
				stable = true;
				break;
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> ne_uu(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	constexpr size_t BitsExt = max(BitsA, BitsB);

	value<BitsExt> ea = a.template zext<BitsExt>();
	value<BitsExt> eb = b.template zext<BitsExt>();

	value<BitsY> tmp = value<BitsY>{ ea.concEq(eb) ? 0u : 1u };

	if (a.node->nature != CONST || b.node->nature != CONST) {
		simulation_logger << "Fixed: ne_uu: Will ==" << std::endl;
		Node* xorc = &simplify((*ea.node) ^ (*eb.node));
		Node* res = &simplify(Extract(0, 0, *xorc));
		for (size_t i = 1; i < BitsExt; i++) {
			res = &simplify(*res | Extract(i, i, *xorc));
			// If we find a bit that is const and equal to 1 (it means we found two bits that are const and different), skip the rest result will be 1
			if (res->nature == CONST and res->cst[0] == 0x1u)
				break;
		}
		tmp.node = &simplify(*res);
	}

	// If both operands are stable result is stable
	// If we find two bits of the same rank in both operand that are const, not equal and stable, we can conclude on the stability
	bool stable = a.is_fully_stable() and b.is_fully_stable();
	if (a.node->nature == CONST and b.node->nature == CONST and not stable) {
		for (size_t i = 0; i < ea.chunks; i++) {
			if (ea.stability[i] & eb.stability[i] & (ea.data[i] ^ eb.data[i])) {
				stable = true;
				break;
			}
		}
	}
	if ((ea.node->nature != CONST or eb.node->nature != CONST) and not stable) {
		for (size_t n = 0; n < ea.chunks; n++) {
			for (size_t i = 0; i < value<BitsExt>::chunk::bits; i++) {
				size_t bit = (n * value<BitsExt>::chunk::bits) + i;
				if (bit >= BitsExt)
					break;
				if (a.stability[n] & b.stability[n] & (0x1 << i)) {
					Node* na = &simplify(Extract(bit, bit, *ea.node));
					Node* nb = &simplify(Extract(bit, bit, *eb.node));
					if (na->nature == CONST and nb->nature == CONST and na->cst[0] != nb->cst[0]) {
						stable = true;
						break;
					}
				}
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: ne_uu: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> ne_ss(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	assert(a.node->nature == CONST and b.node->nature == CONST && "ne_ss is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr and b.ls == nullptr && "ne_ss is currently not implemented for ls concretisations.");

	constexpr size_t BitsExt = max(BitsA, BitsB);

	value<BitsExt> ea = a.template sext<BitsExt>();
	value<BitsExt> eb = b.template sext<BitsExt>();

	value<BitsY> tmp = value<BitsY>{ ea.concEq(eb) ? 0u : 1u };

	// If we can find two bits of the same rank that are stable and not equal, they drive the result
	bool stable = a.is_fully_stable() and b.is_fully_stable();
	if (a.node->nature == CONST and b.node->nature == CONST and not stable) {
		for (size_t i = 0; i < ea.chunks; i++) {
			if (ea.stability[i] & eb.stability[i] & (ea.data[i] ^ eb.data[i])) {
				stable = true;
				break;
			}
		}
	}
	if (not stable)
		tmp.stability[0] = 0x0u;

	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> eqx_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return eq_uu<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> eqx_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return eq_ss<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> nex_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return ne_uu<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> nex_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return ne_ss<BitsY>(a, b);
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> gt_uu(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	constexpr size_t BitsExt = max(BitsA, BitsB);
	//value<BitsY> tmp = value<BitsY> { b.template zext<BitsExt>().ucmp(a.template zext<BitsExt>()) ? 1u : 0u };

	value<BitsExt> ea = a.template zext<BitsExt>();
	value<BitsExt> eb = b.template zext<BitsExt>();

	value<BitsY> tmp = value<BitsY> { eb.ConcUcmp(ea) ? 1u : 0u };

	if (a.node->nature != CONST or b.node->nature != CONST) {
		simulation_logger << "Fixed: gt_uu: Will ucmp" << std::endl;
		Node* res = &simplify((Extract(BitsExt - 1, BitsExt - 1, *ea.node)) & (~Extract(BitsExt - 1, BitsExt - 1, *eb.node)));
		for (int i = BitsExt - 2; i >= 0; i--) {
			//simulation_logger << "i: " << i << std::endl;
			Node* layer = &simplify((Extract(i, i, *ea.node)) & (~Extract(i, i, *eb.node)));
			for (int j = BitsExt - 1; i < j; j--) { // Starting from layer 1 is important
				//simulation_logger << "i: " << i << ", j: " << j << std::endl;
				layer = &simplify(*layer & ~((Extract(j, j, *ea.node)) ^ (Extract(j, j, *eb.node))));
			}
			res = &simplify(*res | *layer);
		}
		tmp.node = res;
	}

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: gt_uu: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> gt_ss(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	constexpr size_t BitsExt = max(BitsA, BitsB);
	//value<BitsY> tmp = value<BitsY> { b.template sext<BitsExt>().scmp(a.template sext<BitsExt>()) ? 1u : 0u };

	value<BitsExt> ea = a.template sext<BitsExt>();
	value<BitsExt> eb = b.template sext<BitsExt>();

	value<BitsY> tmp = value<BitsY> { eb.ConcScmp(ea) ? 1u : 0u };

	if (a.node->nature != CONST or b.node->nature != CONST) {
		simulation_logger << "Fixed: gt_ss: Will scmp" << std::endl;
		Node* resSign = &simplify((~Extract(BitsExt - 1, BitsExt - 1, *ea.node)) & (Extract(BitsExt - 1, BitsExt - 1, *eb.node)));
		Node* res = nullptr;
		if (BitsExt > 1) {
			res = &simplify((Extract(BitsExt - 2, BitsExt - 2, *ea.node)) & (~Extract(BitsExt - 2, BitsExt - 2, *eb.node)));
			for (int i = BitsExt - 3; i >= 0; i--) {
				//simulation_logger << "i: " << i << std::endl;
				Node* layer = &simplify((Extract(i, i, *ea.node)) & (~Extract(i, i, *eb.node)));
				for (int j = BitsExt - 1; i < j; j--) { // Starting from layer 1 is important
					//simulation_logger << "i: " << i << ", j: " << j << std::endl;
					layer = &simplify(*layer & ~((Extract(j, j, *ea.node)) ^ (Extract(j, j, *eb.node))));
				}
				res = &simplify(*res | *layer);
			}
		}
		tmp.node = (BitsExt > 1) ? &simplify(*res | *resSign) : resSign;
	}

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: gt_ss: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> lt_uu(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	constexpr size_t BitsExt = max(BitsA, BitsB);
	//value<BitsY> tmp = value<BitsY> { a.template zext<BitsExt>().ucmp(b.template zext<BitsExt>()) ? 1u : 0u };

	value<BitsExt> ea = a.template zext<BitsExt>();
	value<BitsExt> eb = b.template zext<BitsExt>();

	value<BitsY> tmp = value<BitsY> { ea.ConcUcmp(eb) ? 1u : 0u };

	if (a.node->nature != CONST || b.node->nature != CONST) {
		simulation_logger << "Fixed: lt: Will ucmp" << std::endl;
		Node* res = &simplify((~Extract(BitsExt - 1, BitsExt - 1, *ea.node)) & (Extract(BitsExt - 1, BitsExt - 1, *eb.node)));
		for (int i = BitsExt - 2; i >= 0; i--) {
			//simulation_logger << "i: " << i << std::endl;
			Node* layer = &simplify((~Extract(i, i, *ea.node)) & (Extract(i, i, *eb.node)));
			for (int j = BitsExt - 1; i < j; j--) {
				//simulation_logger << "i: " << i << ", j: " << j << std::endl;
				layer = &simplify(*layer & ~((Extract(j, j, *ea.node)) ^ (Extract(j, j, *eb.node))));
			}
			res = &simplify(*res | *layer);
		}
		tmp.node = res;
	}

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: lt_uu: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> lt_ss(const value<BitsA> &a, const value<BitsB> &b) {
	constexpr size_t BitsExt = max(BitsA, BitsB);
	//value<BitsY> tmp = value<BitsY> { a.template sext<BitsExt>().scmp(b.template sext<BitsExt>()) ? 1u : 0u };

	value<BitsExt> ea = a.template sext<BitsExt>();
	value<BitsExt> eb = b.template sext<BitsExt>();

	value<BitsY> tmp = value<BitsY> { ea.ConcScmp(eb) ? 1u : 0u };

	if (a.node->nature != CONST or b.node->nature != CONST) {
		simulation_logger << "Fixed: lt_ss: Will scmp" << std::endl;
		Node* resSign = &simplify((Extract(BitsExt - 1, BitsExt - 1, *ea.node)) & (~Extract(BitsExt - 1, BitsExt - 1, *eb.node)));
		Node* res = nullptr;
		if (BitsExt > 1) {
			res = &simplify((~Extract(BitsExt - 2, BitsExt - 2, *ea.node)) & (Extract(BitsExt - 2, BitsExt - 2, *eb.node)));
			for (int i = BitsExt - 3; i >= 0; i--) {
				//simulation_logger << "i: " << i << std::endl;
				Node* layer = &simplify((~Extract(i, i, *ea.node)) & (Extract(i, i, *eb.node)));
				for (int j = BitsExt - 1; i < j; j--) { // Starting from layer 1 is important
					//simulation_logger << "i: " << i << ", j: " << j << std::endl;
					layer = &simplify(*layer & ~((Extract(j, j, *ea.node)) ^ (Extract(j, j, *eb.node))));
				}
				res = &simplify(*res | *layer);
			}
		}
		tmp.node = (BitsExt > 1) ? &simplify(*res | *resSign) : resSign;
	}

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	// All bits of both operands are used
	if (a.ls != nullptr or b.ls != nullptr) {
		//simulation_logger << "Fixed: lt_ss: lsconc" << std::endl;
		tmp.ls = leaks::partial_stabilize(leaks::reduce_and_merge(a.ls, b.ls), tmp.node, tmp.stability);
	}

	tmp.debug_assert();
	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> ge_uu(const value<BitsA> &a, const value<BitsB> &b) {
//	constexpr size_t BitsExt = max(BitsA, BitsB);
//	value<BitsY> tmp = value<BitsY> { !a.template zext<BitsExt>().ucmp(b.template zext<BitsExt>()) ? 1u : 0u };
    return not_u<BitsY>(lt_uu<BitsY>(a, b));
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> ge_ss(const value<BitsA> &a, const value<BitsB> &b) {
//	constexpr size_t BitsExt = max(BitsA, BitsB);
//	value<BitsY> tmp = value<BitsY> { !a.template sext<BitsExt>().scmp(b.template sext<BitsExt>()) ? 1u : 0u };
    return not_s<BitsY>(lt_ss<BitsY>(a, b));
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> le_uu(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	assert(a.node->nature == CONST and b.node->nature == CONST && "le_uu is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr and b.ls == nullptr && "le_uu is currently not implemented for ls concretisations.");

	constexpr size_t BitsExt = max(BitsA, BitsB);
	value<BitsY> tmp = value<BitsY> { !b.template zext<BitsExt>().ucmp(a.template zext<BitsExt>()) ? 1u : 0u };

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	return tmp;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> le_ss(const value<BitsA> &a, const value<BitsB> &b) {
	static_assert(BitsY == 1);
	assert(a.node->nature == CONST and b.node->nature == CONST && "le_ss is currently not implemented for symbolic elements.");
	assert(a.ls == nullptr and b.ls == nullptr && "le_ss is currently not implemented for ls concretisations.");

	constexpr size_t BitsExt = max(BitsA, BitsB);
	value<BitsY> tmp = value<BitsY> { !b.template sext<BitsExt>().scmp(a.template sext<BitsExt>()) ? 1u : 0u };

	// TODO: Could be better
	if (not a.is_fully_stable() or not b.is_fully_stable())
		tmp.stability[0] = 0x0u;

	return tmp;
}

// Arithmetic operations
template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> pos_u(const value<BitsA> &a) {
	return a.template zcast<BitsY>();
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> pos_s(const value<BitsA> &a) {
	return a.template scast<BitsY>();
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> neg_u(const value<BitsA> &a) {
	return a.template zcast<BitsY>().neg();
}

template<size_t BitsY, size_t BitsA>
CXXRTL_ALWAYS_INLINE
value<BitsY> neg_s(const value<BitsA> &a) {
	return a.template scast<BitsY>().neg();
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> add_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().add(b.template zcast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> add_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().add(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sub_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template zcast<BitsY>().sub(b.template zcast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> sub_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().sub(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> mul_uu(const value<BitsA> &a, const value<BitsB> &b) {
	constexpr size_t BitsM = BitsA >= BitsB ? BitsA : BitsB;
	return a.template zcast<BitsM>().template mul<BitsY>(b.template zcast<BitsM>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> mul_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return a.template scast<BitsY>().template mul<BitsY>(b.template scast<BitsY>());
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
std::pair<value<BitsY>, value<BitsY>> divmod_uu(const value<BitsA> &a, const value<BitsB> &b) {
	constexpr size_t Bits = max(BitsY, max(BitsA, BitsB));
	value<Bits> quotient;
	value<Bits> remainder;
	value<Bits> dividend = a.template zext<Bits>();
	value<Bits> divisor = b.template zext<Bits>();
	std::tie(quotient, remainder) = dividend.udivmod(divisor);
	return {quotient.template trunc<BitsY>(), remainder.template trunc<BitsY>()};
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
std::pair<value<BitsY>, value<BitsY>> divmod_ss(const value<BitsA> &a, const value<BitsB> &b) {
	constexpr size_t Bits = max(BitsY, max(BitsA, BitsB));
	value<Bits> quotient;
	value<Bits> remainder;
	value<Bits> dividend = a.template sext<Bits>();
	value<Bits> divisor = b.template sext<Bits>();
	std::tie(quotient, remainder) = dividend.sdivmod(divisor);
	return {quotient.template trunc<BitsY>(), remainder.template trunc<BitsY>()};
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> div_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_uu<BitsY>(a, b).first;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> div_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_ss<BitsY>(a, b).first;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> mod_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_uu<BitsY>(a, b).second;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> mod_ss(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_ss<BitsY>(a, b).second;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> modfloor_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_uu<BitsY>(a, b).second;
}

// GHDL Modfloor operator. Returns r=a mod b, such that r has the same sign as b and
// a=b*N+r where N is some integer
// In practical terms, when a and b have different signs and the remainder returned by divmod_ss is not 0
// then return the remainder + b
template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> modfloor_ss(const value<BitsA> &a, const value<BitsB> &b) {
	value<BitsY> r;
	r = divmod_ss<BitsY>(a, b).second;
	if((b.is_neg() != a.is_neg()) && !r.is_zero())
		return add_ss<BitsY>(b, r);
	return r;
}

template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> divfloor_uu(const value<BitsA> &a, const value<BitsB> &b) {
	return divmod_uu<BitsY>(a, b).first;
}

// Divfloor. Similar to above: returns q=a//b, where q has the sign of a*b and a=b*q+N.
// In other words, returns (truncating) a/b, except if a and b have different signs
// and there's non-zero remainder, subtract one more towards floor.
template<size_t BitsY, size_t BitsA, size_t BitsB>
CXXRTL_ALWAYS_INLINE
value<BitsY> divfloor_ss(const value<BitsA> &a, const value<BitsB> &b) {
	value<BitsY> q, r;
	std::tie(q, r) = divmod_ss<BitsY>(a, b);
	if ((b.is_neg() != a.is_neg()) && !r.is_zero())
		return sub_uu<BitsY>(q, value<1> { 1u });
	return q;

}

// Memory helper
struct memory_index {
	bool valid;
	// We hardcode width of an address but we need symbolic value of index so no other way
	// index was already limited to addr.data[0] anyways
	value<32> index;

	template<size_t BitsAddr>
	memory_index(const value<BitsAddr> &addr, size_t offset, size_t depth) {
		static_assert(value<BitsAddr>::chunks <= 1, "memory address is too wide");
		assert(offset == 0 && "Symbolic reads with offsets are unimplemented");
		simulation_logger << "Requested memory index: 0x" << std::hex << addr.data[0] << std::dec << ", in mem depth: " << depth << std::endl;
		if (addr.node->nature != CONST)
			simulation_logger << "Symbolic index got requested..." << std::endl;

		size_t offset_index = addr.data[0];

		// We compute the validity concretly
		valid = (offset_index >= offset && offset_index < offset + depth);
		//index = offset_index - offset;
		index = addr.template zext<32>().val();
	}
};

} // namespace cxxrtl_yosys

#endif
