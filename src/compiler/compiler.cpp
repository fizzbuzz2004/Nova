#include "compiler/compiler.hpp"

#include "codegen/cpp_generator.hpp"
#include "diagnostics/error.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/analyzer.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace nova {
namespace {

[[nodiscard]] std::string shell_quote(const std::string& value) {
#ifdef _WIN32
    // Quote a single Windows command-line argument for cmd.exe / CreateProcess parsing.
    std::string result = "\"";
    std::size_t backslashes = 0;
    for (const char c : value) {
        if (c == '\\') {
            ++backslashes;
        } else if (c == '"') {
            result.append(backslashes * 2U + 1U, '\\');
            result += '"';
            backslashes = 0;
        } else {
            result.append(backslashes, '\\');
            backslashes = 0;
            result += c;
        }
    }
    result.append(backslashes * 2U, '\\');
    result += '"';
    return result;
#else
    std::string result = "'";
    for (const char c : value) {
        if (c == '\'') result += "'\\''";
        else result += c;
    }
    result += '\'';
    return result;
#endif
}

} // namespace

std::string Compiler::read_source(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open input file: " + path.string());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) throw std::runtime_error("Failed while reading input file: " + path.string());
    return buffer.str();
}

void Compiler::write_file(const std::filesystem::path& path, std::string_view content) {
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    const auto temporary = path.string() + ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot create output file: " + temporary);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!output) throw std::runtime_error("Failed while writing output file: " + temporary);
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        // Windows does not replace an existing destination through rename.
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporary, path, error);
        if (error) {
            std::filesystem::remove(temporary);
            throw std::runtime_error("Cannot finalize output file: " + path.string() + ": " + error.message());
        }
    }
}

void Compiler::invoke_native_compiler(const std::string& compiler,
                                      const std::filesystem::path& cpp_file,
                                      const std::filesystem::path& output_file) {
#ifdef _WIN32
    const std::string executable = output_file.extension() == ".exe"
        ? output_file.string() : output_file.string() + ".exe";
#else
    const std::string executable = output_file.string();
#endif
    const std::string command = shell_quote(compiler) + " -std=c++20 -O2 " +
        shell_quote(cpp_file.string()) + " -o " + shell_quote(executable);
    const int status = std::system(command.c_str());
    if (status != 0) {
        throw std::runtime_error("Native compiler failed (exit status " + std::to_string(status) +
                                 "). Generated C++ was kept at: " + cpp_file.string());
    }
}

void Compiler::compile(const CompileOptions& options) const {
    if (options.input.empty()) throw std::invalid_argument("No input file was specified.");
    const std::string source = read_source(options.input);
    Lexer lexer(source);
    const auto tokens = lexer.scan_tokens();
    Parser parser(tokens);
    const Program program = parser.parse();
    SemanticAnalyzer analyzer;
    analyzer.analyze(program);
    CppGenerator generator;
    const std::string cpp = generator.generate(program);

    std::filesystem::path cpp_output;
    if (options.emit_cpp_only) {
        if (options.output.empty()) {
            cpp_output = options.input;
            cpp_output.replace_extension(".cpp");
        } else {
            cpp_output = options.output;
        }
    } else {
        cpp_output = options.output.empty()
            ? options.input.parent_path() / options.input.stem().concat("_generated.cpp")
            : std::filesystem::path(options.output.string() + ".generated.cpp");
    }
    write_file(cpp_output, cpp);

    if (!options.emit_cpp_only) {
        const auto native_output = options.output.empty()
            ? options.input.parent_path() / options.input.stem()
            : options.output;
        invoke_native_compiler(options.cxx_compiler, cpp_output, native_output);
    }
}

} // namespace nova
