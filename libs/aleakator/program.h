#ifndef PROGRAM_H
#define PROGRAM_H

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class BinaryLoadError final : public std::runtime_error {
    public:
        BinaryLoadError(const std::string& file, const std::string& message)
            : std::runtime_error(file + ": " + message) {}
};

struct Symbol {
    uint32_t addr_;
    uint32_t size_;
};

struct RawProgram {
    std::vector<uint32_t> text_;
    std::vector<uint32_t> data_;
    std::map<std::string, Symbol> symbols_;
};

// Utility class that loads a program
class BinaryLoader {
    public:
        BinaryLoader() = delete;

        // Reads text + data + symbols
        static RawProgram load(const std::string &file);
};

#endif // PROGRAM_H
