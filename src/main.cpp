#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Runtime/Interpreter.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: lexor <source.lexor>\n";
        return 1;
    }

    // ── 1. Read source file ───────────────────────────────────────────────────
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: cannot open file '" << argv[1] << "'\n";
        return 1;
    }
    std::stringstream ss;
    ss << file.rdbuf();

    // ── 2. Lex ────────────────────────────────────────────────────────────────
    Lexer lexer(ss.str());
    std::vector<Token> tokens;
    Token tok;
    while ((tok = lexer.nextToken()).type != TokenType::END_OF_FILE)
        tokens.push_back(tok);

    for (auto& tok : tokens) {
    std::cout << "TOKEN: type=" << (int)tok.type 
              << " value='" << tok.value << "'\n";
}
    // ── 3. Parse ──────────────────────────────────────────────────────────────
    Parser parser(std::move(tokens));
    std::vector<StmtPtr> program;
    try {
        program = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    // ── 4. Interpret ──────────────────────────────────────────────────────────
    Interpreter interpreter;
    try {
        interpreter.execute(program);
    } catch (const std::exception& e) {
        std::cerr << "Runtime error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
