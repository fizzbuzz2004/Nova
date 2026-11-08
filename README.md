# Nova

Nova is a small statically checked programming language whose compiler is written in C++20.
The first backend emits portable C++ source code. GCC or Clang can then compile that
source into a native executable.

## Requirements

- CMake 3.20+
- A C++20 compiler to build Nova
- GCC, Clang (`g++` or `clang++`), or MSVC (`cl.exe`) to build generated programs

## Build Nova

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Compile a Nova program

```sh
./build/nova examples/hello.nova
```

This creates `examples/hello` (or `examples/hello.exe` on Windows) and runs the native
compiler. You can also only emit generated C++:

```sh
./build/nova examples/hello.nova --emit-cpp examples/hello.cpp
```

On Windows, the executable may be located at `build/Debug/nova.exe` or
`build/Release/nova.exe`, depending on the CMake generator.

## Language overview

```nova
fn fibonacci(n: int) -> int {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

let answer: int = fibonacci(10);
print(answer);
```

Supported in this initial vertical slice: `int`, `bool`, variables, assignment,
integer arithmetic, comparisons, logical operators, `print`, blocks, `if`/`else`,
`while`, `for` loops, typed functions, calls and `return`. Function declarations must be at top level;
function bodies cannot capture top-level variables in this version. Every function must
return a value on all statically obvious control-flow paths.

## Pipeline

1. Lexer: source text -> tokens
2. Parser: tokens -> abstract syntax tree (AST)
3. Semantic analyzer: name and type checks
4. C++ generator: AST -> C++20 source
5. Native toolchain: generated C++ -> executable

This is an educational compiler foundation, not a production security boundary.
