#include "aes_herbst_setup.h"
#include "cxxrtl_cortex_m3.h"

// Function that need a global context
namespace aes_herbst_ns {
    cxxrtl_design::p_top* circuit;
    Program* program;

    cxxrtl::value<32> read_result;
    ArrayExp* sbox_array;
    int sboxp_access = 0;

    cxxrtl::value<32>& SBoxPrime(cxxrtl::value<32>& e) {
        // Key scheduele
        cxxrtl::value<8> temp;
        if (sboxp_access < 40)
            temp = circuit->memory_p_uRAM_2e_RAM[program->get_index("ram", "temp")+(sboxp_access%4)].trunc<8>().val();
        else
            temp = circuit->memory_p_uRAM_2e_RAM[program->get_index("ram", "temp_sub")].trunc<8>().val();

        cxxrtl::value<8> m = circuit->memory_p_uRAM_2e_RAM[program->get_index("ram", "m")].shr(cxxrtl::value<32>{static_cast<uint32_t>(program->get_position("m")*8)}).trunc<8>().val();
        cxxrtl::value<8> mp = circuit->memory_p_uRAM_2e_RAM[program->get_index("ram", "mp")].shr(cxxrtl::value<32>{static_cast<uint32_t>(program->get_position("mp")*8)}).trunc<8>().val();

        cxxrtl::value<8> index = cxxrtl_yosys::xor_uu<8>(temp, m);
        cxxrtl::value<8> sboxEq = circuit->memory_p_uRAM_2e_RAM[program->get_index("ram", "sbox") + index.data[0]].trunc<8>().val();
        sboxEq.setNode(&simplify((*sbox_array)[*index.node]));

        read_result = cxxrtl_yosys::xor_uu<8>(sboxEq, mp).template zext<32>().val();

        //raise(SIGTRAP);
        sboxp_access++;
        return read_result;
    }
}

void aes_herbst::init_implem(Manager& manager, cxxrtl_design::p_top& top) {
    assert(symbols_.contains("start_analysis") && "start_analysis symbol is mandatory for aes_herbst.");
    assert(symbols_.contains("end_analysis") && "end_analysis symbol is mandatory for aes_herbst.");

    for (int i = 0; i < 16; ++i) {
        spt.push_back(&symbol(("aes_pt" + std::to_string(i)).c_str(), 'P', 8));
        skey.push_back(&symbol(("aes_key" + std::to_string(i)).c_str(), 'S', 8));
    }
    sm = &symbol("aes_m", 'M', 8);
    smt = &Concat(symbol("aes_mt4", 'M', 8), symbol("aes_mt3", 'M', 8), symbol("aes_mt2", 'M', 8), symbol("aes_mt1", 'M', 8));
    smp = &symbol("aes_mp", 'M', 8);
    for (int i = 0; i < 10; i++)
        srcon.push_back(&symbol(("rcon_" + std::to_string(i)).c_str(), 'P', 8));

    assert(this->get_position("pt") == 0 && this->get_size("pt") == 4*4);
    assert(this->get_position("key") == 0 && this->get_size("key") == 4*4);
    assert(this->get_position("sboxp") == 0 && this->get_size("sboxp") == 1024);

    // Prepare every value
    cxxrtl::value<32> mt = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "mt")];
    cxxrtl::value<32> m = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "m")];
    cxxrtl::value<32> mp = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "mp")];

    cxxrtl::value<32> pt0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "pt") + 0];
    cxxrtl::value<32> pt1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "pt") + 1];
    cxxrtl::value<32> pt2 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "pt") + 2];
    cxxrtl::value<32> pt3 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "pt") + 3];

    cxxrtl::value<32> key0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "key") + 0];
    cxxrtl::value<32> key1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "key") + 1];
    cxxrtl::value<32> key2 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "key") + 2];
    cxxrtl::value<32> key3 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "key") + 3];

    cxxrtl::value<32> rcon0 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "rcon") + 0];
    cxxrtl::value<32> rcon1 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "rcon") + 1];
    cxxrtl::value<32> rcon2 = top.memory_p_uRAM_2e_RAM[this->get_index("ram", "rcon") + 2];

    // Init sboxp
    uint32_t concMVal = (m.get<uint32_t>() & this->get_mask("m").get<uint32_t>()) >> (this->get_position("m") * 8); //Concretisation but at init
    uint32_t concMPVal = (mp.get<uint32_t>() & this->get_mask("mp").get<uint32_t>()) >> (this->get_position("mp") * 8); //Concretisation but at init
    for (uint32_t i = 0; i < 256; i++) {
        int lineIdx = (i ^ concMVal);

        // sboxp[i ^ m] = sbox[i] ^ mp;
        cxxrtl::value<32> preVal;
        preVal.set<uint32_t, true>(sbox[i] ^ concMPVal);
        preVal.setNode(&simplify(constant(sbox[i], 32) ^ Concat(constant(0, 24), *smp)));
        top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "sboxp") + lineIdx, preVal, cxxrtl::value<32>(0x000000FFu));
    }

    pt0.setNode(&simplify(Concat(*spt[3], *spt[2], *spt[1], *spt[0])));
    pt1.setNode(&simplify(Concat(*spt[7], *spt[6], *spt[5], *spt[4])));
    pt2.setNode(&simplify(Concat(*spt[11], *spt[10], *spt[9], *spt[8])));
    pt3.setNode(&simplify(Concat(*spt[15], *spt[14], *spt[13], *spt[12])));

    key0.setNode(&simplify(Concat(*skey[3], *skey[2], *skey[1], *skey[0])));
    key1.setNode(&simplify(Concat(*skey[7], *skey[6], *skey[5], *skey[4])));
    key2.setNode(&simplify(Concat(*skey[11], *skey[10], *skey[9], *skey[8])));
    key3.setNode(&simplify(Concat(*skey[15], *skey[14], *skey[13], *skey[12])));

    rcon0.setNode(&simplify(Concat(*srcon[3], *srcon[2], *srcon[1], *srcon[0])));
    rcon1.setNode(&simplify(Concat(*srcon[7], *srcon[6], *srcon[5], *srcon[4])));
    rcon2.setNode(&simplify(Concat(constant(0, 8), constant(0, 8), *srcon[9], *srcon[8])));

    mt.setNode(smt);
    m.setNode(&simplify(Concat(constant(0, 24), *sm) << (this->get_position("m") * 8)));
    mp.setNode(&simplify(Concat(constant(0, 24), *smp) << (this->get_position("mp") * 8)));

    // Write in memory
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "pt") + 0, pt0, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "pt") + 1, pt1, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "pt") + 2, pt2, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "pt") + 3, pt3, cxxrtl::value<32>(0xFFFFFFFFu));

    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "key") + 0, key0, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "key") + 1, key1, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "key") + 2, key2, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "key") + 3, key3, cxxrtl::value<32>(0xFFFFFFFFu));

    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "rcon") + 0, rcon0, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "rcon") + 1, rcon1, cxxrtl::value<32>(0xFFFFFFFFu));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "rcon") + 2, rcon2, cxxrtl::value<32>(0x0000FFFFu));

    // We may have more than on update on the same memory space as we do not know where the symbols are
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "mt"), mt, this->get_mask("mt"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "m"), m, this->get_mask("m"));
    top.memory_p_uRAM_2e_RAM.update(this->get_index("ram", "mp"), mp, this->get_mask("mp"));

    for (int i = 0; i < 4; i++) {
        symbol_to_conc_.insert({spt[0+i], &constant((pt0.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({spt[4+i], &constant((pt1.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({spt[8+i], &constant((pt2.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({spt[12+i], &constant((pt3.get<uint32_t>() >> i*8) & 0xFFu, 8)});

        symbol_to_conc_.insert({skey[0+i], &constant((key0.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({skey[4+i], &constant((key1.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({skey[8+i], &constant((key2.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({skey[12+i], &constant((key3.get<uint32_t>() >> i*8) & 0xFFu, 8)});

        symbol_to_conc_.insert({srcon[0+i], &constant((rcon0.get<uint32_t>() >> i*8) & 0xFFu, 8)});
        symbol_to_conc_.insert({srcon[4+i], &constant((rcon1.get<uint32_t>() >> i*8) & 0xFFu, 8)});

        symbol_to_conc_.insert({&Extract(((i+1)*8)-1, (i*8), *smt), &constant((mt.get<uint32_t>() >> i*8) & 0xFFu, 8)});
    }
    symbol_to_conc_.insert({srcon[8], &constant((rcon2.get<uint32_t>() >> 0) & 0xFFu, 8)});
    symbol_to_conc_.insert({srcon[9], &constant((rcon2.get<uint32_t>() >> 8) & 0xFFu, 8)});

    symbol_to_conc_.insert({sm, &constant(concMVal, 8)});
    symbol_to_conc_.insert({smp, &constant(concMPVal, 8)});

    // Register sbox as array
    aes_herbst_ns::circuit = &top;
    aes_herbst_ns::program = this;
    registerArray("sbox", 8, 8, 0x0, 256, NULL, sbox, 4);
    aes_herbst_ns::sbox_array = &getArrayByName("sbox");

    size_t startingIndex = this->get_index("ram", "sboxp");
    size_t endingIndex = startingIndex + (this->get_size("sboxp") / 4);
    std::tuple<size_t, size_t, std::function<cxxrtl::value<32>&(cxxrtl::value<32>&)>> sbp{startingIndex, endingIndex, aes_herbst_ns::SBoxPrime};
    top.memory_p_uRAM_2e_RAM.funcArrays.push_back(sbp);
}

void aes_herbst::conclude_implem(Manager& manager, cxxrtl_design::p_top& top) {
    size_t cipher_index = this->get_index("ram", "ciphered_text");
    for (int i = 0; i < 4; ++i) {
        std::cout << top.memory_p_uRAM_2e_RAM[cipher_index + i] << std::endl;
    }
    std::cout << "Sbox accesses : " << std::dec << aes_herbst_ns::sboxp_access << std::endl;
    bool isResultValid = true;
    for (int i = 0; i < 4; ++i)
        if (&getExpValue(*top.memory_p_uRAM_2e_RAM[cipher_index + i].node, symbol_to_conc_) != &constant(top.memory_p_uRAM_2e_RAM[cipher_index + i].data[0], 32))
            isResultValid = false;

    if (isResultValid)
        std::cout << "Concretizing result expressions provides the correct concrete result !" << std::endl;
    else
        std::cout << "Concretizing result expressions DOES NOT provide the correct concrete result, something went wrong." << std::endl;
}

void aes_herbst::hook_implem(Manager& manager, cxxrtl_design::p_top& top) {
    for (int i = 0; i < 16; i++) {
        assert(!top.memory_p_uRAM_2e_RAM[this->get_index("ram", "x") + i].node->hasPlus);
//        assert(simplify(Extract(31, 8, *top.memory_p_uRAM_2e_RAM[getIndex("ram", "x") + i].node)).nature == CONST);
    }

//    if (symbolTable["mix_column"].first == pc(top)) {
//        std::cout << "Found mix_column symbol, printing state:" << std::endl;
//        for (int i = 0; i < 16; i++)
//           std::cout << *top.memory_p_uRAM_2e_RAM[getIndex("ram", "x") + i].node << std::endl;
//    }

    if (symbols_["start_analysis"].addr == pc(top)) {
        std::cout << "Found start_analysys symbol, adjusting skip_verif_cycles !" << std::endl;
        //CFG.SKIP_VERIF_CYCLES = STEPS+3;
        manager.begin_measure();
    }

    if (symbols_["end_analysis"].addr == pc(top)) {
        std::cout << "Found end_analysys symbol, adjusting skip_verif_cycles !" << std::endl;
        //CFG.SKIP_VERIF_CYCLES = 99999;
        manager.end_measure();
    }
}
