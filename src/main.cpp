#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lexer.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    std::vector<std::string> commandLineArgs(argc - 1);
    for (int i = 1; i < argc; i++) {
        commandLineArgs[i - 1] = argv[i];
    }
    size_t concurrency = 1;
    std::filesystem::path makefilePath = "Makefile";
    std::vector<std::string> targets;
    for (size_t i = 0; i < commandLineArgs.size(); i++) {
        if (commandLineArgs[i] == "-f") {
            if (i + 1 == commandLineArgs.size()) {
                throw std::runtime_error("Makefile path missing");
            } else {
                makefilePath = commandLineArgs[i + 1];
                i++;
            }
        } else if (commandLineArgs[i] == "-t") {
            if (i + 1 == commandLineArgs.size()) {
                throw std::runtime_error("concurrency argument missing");
            } else {
                concurrency = std::stoul(commandLineArgs[i + 1]);
                i++;
            }
        } else {
            targets.emplace_back(commandLineArgs[i]);
        }
    }

    std::cout << concurrency << ' ' << makefilePath << std::endl;

    std::ifstream fin(makefilePath);
    if (!fin.is_open()) {
        throw std::runtime_error("can't open makefile, path: " +
                                 makefilePath.string());
    }
    // Get the file size
    fin.seekg(0, std::ios::end);
    const std::streamsize fileSize = fin.tellg();
    fin.seekg(0, std::ios::beg);
    std::string input(fileSize, 0);
    fin.read(input.data(), fileSize);

    // Pass 1: Lexing
    auto tokens = lexer::lex(input);
    size_t lineno = 1;
    std::cout << lineno << ": ";
    for (const auto& sp : tokens) {
        std::cout << sp->toString() << ' ';
        if (sp->lineno > lineno) {
            for (size_t i = lineno + 1; i <= sp->lineno; i++) {
                std::cout << '\n' << i << ": ";
            }
            lineno = sp->lineno;
        }
    }
    std::cout << "\n";

    // Pass 2: Parsing
    auto [varDefs, rules] = parser::parse(tokens);
    std::cout << "Variable Definitions\n";
    for (const auto& vd : varDefs) {
        std::cout << vd.toString() << "\n";
    }
    std::cout << "Rules:\n";
    for (const auto& r : rules) {
        std::cout << r.toString() << "\n";
    }
}