#include <iostream>

#include <llvm/Object/Binary.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include "program.h"

namespace {

llvm::object::OwningBinary<llvm::object::Binary> open_elf(const std::string& file) {
    llvm::Expected<llvm::object::OwningBinary<llvm::object::Binary>> binaryOrErr = llvm::object::createBinary(file);
    if (not binaryOrErr)
        throw BinaryLoadError(file, llvm::toString(binaryOrErr.takeError()));

    if (not binaryOrErr->getBinary()->isELF())
        throw BinaryLoadError(file, "is not an ELF object file");
    return std::move(*binaryOrErr);
}

std::vector<uint32_t> section_to_words(const std::string& file,
                                       const llvm::object::ObjectFile& obj,
                                       const std::string& name) {
    for (const llvm::object::SectionRef& section : obj.sections()) {
        llvm::Expected<llvm::StringRef> nameOrErr = section.getName();
        if (not nameOrErr || nameOrErr.get() != name)
            continue;

        llvm::Expected<llvm::StringRef> contentsOrErr = section.getContents();
        if (not contentsOrErr)
            throw BinaryLoadError(file, name + ": cannot read section contents: " +
                                       llvm::toString(contentsOrErr.takeError()));

        const llvm::StringRef bytes = contentsOrErr.get();
        if (bytes.size() % sizeof(uint32_t) != 0)
            throw BinaryLoadError(file, name + ": section size is not a multiple of 4 bytes");

        std::vector<uint32_t> words(bytes.size() / sizeof(uint32_t));
        const uint8_t* data = reinterpret_cast<const uint8_t*>(bytes.data());
        for (size_t i = 0; i < words.size(); ++i)
            words[i] = *(reinterpret_cast<const uint32_t*>(&data[i * 4]));
        return words;
    }
    throw BinaryLoadError(file, "section '" + name + "' not found");
}

std::map<std::string, Symbol> collect_symbols(const std::string& file,
                                              const llvm::object::ObjectFile& obj) {
    std::map<std::string, Symbol> symbols;
    for (const auto& symb : obj.symbols()) {
        llvm::Expected<llvm::StringRef> name = symb.getName();
        if (not name)
            continue;
        llvm::Expected<llvm::object::section_iterator> sec = symb.getSection();
        if (not sec or sec.get() == obj.section_end())
            continue;
        if (not (sec.get()->isData() || sec.get()->isBSS() || sec.get()->isText()))
            continue;
        llvm::Expected<uint64_t> addr = symb.getAddress();
        if (not addr)
            continue;

        symbols[name.get().str()] = Symbol{
            static_cast<uint32_t>(addr.get()),
            static_cast<uint32_t>(symb.getCommonSize())};
    }
    return symbols;
}

} // namespace

RawProgram BinaryLoader::load(const std::string &file) {
    llvm::object::OwningBinary<llvm::object::Binary> owning = open_elf(file);
    llvm::object::ObjectFile* obj = llvm::dyn_cast<llvm::object::ObjectFile>(owning.getBinary());

    // If program
    RawProgram program;
    program.text_    = section_to_words(file, *obj, ".text");
    program.data_    = section_to_words(file, *obj, ".data");
    program.symbols_ = collect_symbols(file, *obj);
    return program;
}
