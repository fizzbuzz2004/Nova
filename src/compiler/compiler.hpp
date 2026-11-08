#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace nova {

struct CompileOptions {
    std::filesystem::path input;
    std::filesystem::path output;
    bool emit_cpp_only{false};
    std::string cxx_compiler{"c++"};
};

class Compiler final {
public:
    void compile(const CompileOptions& options) const;

private:
    [[nodiscard]] static std::string read_source(const std::filesystem::path& path);
    static void write_file(const std::filesystem::path& path, std::string_view content);
    static void invoke_native_compiler(const std::string& compiler,
                                       const std::filesystem::path& cpp_file,
                                       const std::filesystem::path& output_file);
};

} // namespace nova
