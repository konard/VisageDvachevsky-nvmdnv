// Simple test to debug validator timeout
#include <iostream>
#include <chrono>
#include "NovelMind/scripting/lexer.hpp"
#include "NovelMind/scripting/parser.hpp"
#include "NovelMind/scripting/validator.hpp"

using namespace NovelMind::scripting;

const char* SCRIPT_WITH_VARIABLES = R"(
character Hero(name="Hero", color="#FF0000")

scene intro {
    set points = 0
    set flag visited = false
    say Hero "Starting adventure..."
    set points = points + 10
    set flag visited = true
    goto ending
}

scene ending {
    if points > 5 {
        say Hero "You scored high!"
    }
}
)";

int main() {
    std::cout << "Starting validator test..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "Tokenizing..." << std::endl;
    Lexer lexer;
    auto tokens = lexer.tokenize(SCRIPT_WITH_VARIABLES);
    if (!tokens.isOk()) {
        std::cerr << "Lexer error: " << tokens.error() << std::endl;
        return 1;
    }
    auto tokenEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Tokenized in " << std::chrono::duration_cast<std::chrono::milliseconds>(tokenEnd - start).count() << "ms" << std::endl;

    std::cout << "Parsing..." << std::endl;
    Parser parser;
    auto program = parser.parse(tokens.value());
    if (!program.isOk()) {
        std::cerr << "Parser error: " << program.error() << std::endl;
        return 1;
    }
    auto parseEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Parsed in " << std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - tokenEnd).count() << "ms" << std::endl;

    std::cout << "Validating..." << std::endl;
    Validator validator;
    validator.setReportUnused(false);
    auto result = validator.validate(program.value());
    auto validateEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Validated in " << std::chrono::duration_cast<std::chrono::milliseconds>(validateEnd - parseEnd).count() << "ms" << std::endl;

    std::cout << "isValid: " << (result.isValid ? "true" : "false") << std::endl;
    std::cout << "hasErrors: " << (result.hasErrors() ? "true" : "false") << std::endl;

    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(validateEnd - start).count();
    std::cout << "Total time: " << total << "ms" << std::endl;

    return 0;
}
