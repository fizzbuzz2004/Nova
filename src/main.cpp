#include "compiler/compiler.hpp"
#include "diagnostics/error.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_usage(std::string_view executable) {
    std::cout
        << "Nova compiler " << NOVA_VERSION << "\n\n"
        << "Usage:\n"
        << "  " << executable << " <input.nova> [-o <output>] [--emit-cpp <file.cpp>] [--cxx <compiler>]\n\n"
        << "Options:\n"
        << "  -o <file>           Output native executable path\n"
        << "  --emit-cpp <file>   Generate C++ source only; do not invoke GCC/Clang\n"
        << "  --cxx <compiler>    C++ compiler executable (default: c++)\n"
        << "  -h, --help          Show this help\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                      std::string_view(argv[1]) == "--help")) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc < 2) {
        print_usage(argc > 0 ? argv[0] : "nova");
        return 2;
    }

    nova::CompileOptions options;
    options.input = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "-h" || argument == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        if (argument == "-o" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (argument == "--emit-cpp" && i + 1 < argc) {
            options.emit_cpp_only = true;
            options.output = argv[++i];
        } else if (argument == "--cxx" && i + 1 < argc) {
            options.cxx_compiler = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << argument << '\n';
            print_usage(argv[0]);
            return 2;
        }
    }

    try {
        nova::Compiler compiler;
        compiler.compile(options);
        if (options.emit_cpp_only) {
            std::cout << "Generated C++ source: " << options.output.string() << '\n';
        } else {
            auto native_output = options.output.empty()
                ? options.input.parent_path() / options.input.stem()
                : options.output;
#ifdef _WIN32
            if (native_output.extension() != ".exe") native_output += ".exe";
#endif
            std::cout << "Native compilation succeeded: " << native_output.string() << '\n';
        }
        return 0;
    } catch (const nova::CompileError& error) {
        std::cerr << options.input.string() << ':' << error.position().line << ':'
                  << error.position().column << ": error: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "nova: error: " << error.what() << '\n';
        return 1;
    }
}
