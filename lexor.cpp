#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <cctype>

enum class TokenType {
    SCRIPT_AREA, START_SCRIPT, END_SCRIPT,
    DECLARE, INT_TYPE, CHAR_TYPE, BOOL_TYPE, FLOAT_TYPE,
    PRINT, IDENTIFIER, INT_LIT, FLOAT_LIT, CHAR_LIT, BOOL_LIT, STRING_LIT,
    ASSIGN, AMPERSAND, DOLLAR, LBRACKET, RBRACKET, HASH, COLON, COMMA,
    NEWLINE, END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
    std::string src;
    size_t pos = 0;
public:
    Lexer(std::string s) : src(s) {}

    Token nextToken() {
        while (pos < src.length()) {
            unsigned char c = src[pos];
            if (isspace(c) && c != '\n') { pos++; continue; }
            if (c == '\n') { pos++; return {TokenType::NEWLINE, "\n"}; }

            if (c == '%' && pos + 1 < src.length() && src[pos+1] == '%') {
                while (pos < src.length() && src[pos] != '\n') pos++;
                continue;
            }

            if (isalpha(c) || c == '_') {
                std::string val;
                while (pos < src.length() && (isalnum(src[pos]) || src[pos] == '_')) val += src[pos++];
                if (val == "SCRIPT") {
                    size_t t = pos; while(t < src.length() && isspace(src[t])) t++;
                    if (src.substr(t, 4) == "AREA") { pos = t + 4; return {TokenType::SCRIPT_AREA, "SCRIPT AREA"}; }
                }
                if (val == "START") {
                    size_t t = pos; while(t < src.length() && isspace(src[t])) t++;
                    if (src.substr(t, 6) == "SCRIPT") { pos = t + 6; return {TokenType::START_SCRIPT, "START SCRIPT"}; }
                }
                if (val == "END") {
                    size_t t = pos; while(t < src.length() && isspace(src[t])) t++;
                    if (src.substr(t, 6) == "SCRIPT") { pos = t + 6; return {TokenType::END_SCRIPT, "END SCRIPT"}; }
                }
                if (val == "DECLARE") return {TokenType::DECLARE, "DECLARE"};
                if (val == "INT") return {TokenType::INT_TYPE, "INT"};
                if (val == "CHAR") return {TokenType::CHAR_TYPE, "CHAR"};
                if (val == "BOOL") return {TokenType::BOOL_TYPE, "BOOL"};
                if (val == "FLOAT") return {TokenType::FLOAT_TYPE, "FLOAT"};
                if (val == "PRINT") return {TokenType::PRINT, "PRINT"};
                return {TokenType::IDENTIFIER, val};
            }

            if (isdigit(c)) {
                std::string val;
                while (pos < src.length() && isdigit(src[pos])) val += src[pos++];
                if (pos < src.length() && src[pos] == '.') {
                    val += src[pos++];
                    while (pos < src.length() && isdigit(src[pos])) val += src[pos++];
                    return {TokenType::FLOAT_LIT, val};
                }
                return {TokenType::INT_LIT, val};
            }

            // Strips quotes, returning inner content as STRING_LIT
            if (c == '"' || c == '\'') {
                char quote = c;
                pos++; std::string val = "";
                while (pos < src.length() && src[pos] != quote) val += src[pos++];
                if (pos < src.length()) pos++;
                if (val == "TRUE" || val == "FALSE") return {TokenType::BOOL_LIT, val};
                return {TokenType::STRING_LIT, val};
            }

            pos++;
            if (c == '=') return {TokenType::ASSIGN, "="};
            if (c == ':') return {TokenType::COLON, ":"};
            if (c == '&') return {TokenType::AMPERSAND, "&"};
            if (c == '$') return {TokenType::DOLLAR, "$"};
            if (c == '[') return {TokenType::LBRACKET, "["};
            if (c == ']') return {TokenType::RBRACKET, "]"};
            if (c == '#') return {TokenType::HASH, "#"};
            if (c == ',') return {TokenType::COMMA, ","};
        }
        return {TokenType::END_OF_FILE, ""};
    }
};

class Interpreter {
    std::vector<Token> tokens;
    size_t cur = 0;
    std::map<std::string, std::string> vars;

    // The PDA logic applied to strings to resolve nested boundaries
    std::string resolveEscapes(const std::string& input) {
        std::string result = "";
        size_t i = 0;
        while (i < input.length()) {
            if (input[i] == '[') {
                // Find matching closing bracket dynamically
                size_t close_idx = i + 1;
                while (close_idx < input.length() && input[close_idx] != ']') {
                    close_idx++;
                }
                if (close_idx < input.length()) {
                    // Extract the content and peel off the outer [ ]
                    result += input.substr(i + 1, close_idx - i - 1);
                    i = close_idx + 1; // Move past the closing bracket
                } else {
                    result += input[i++];
                }
            } else {
                result += input[i++];
            }
        }
        return result;
    }

    // Universal evaluator for ALL token values
    std::string evaluate(Token t) {
        if (t.type == TokenType::IDENTIFIER) {
            return vars[t.value]; // Resolves variables
        }
        if (t.type == TokenType::STRING_LIT || t.type == TokenType::CHAR_LIT) {
            return resolveEscapes(t.value); // Applies PDA logic to string contents
        }
        return t.value;
    }

public:
    Interpreter(std::vector<Token> t) : tokens(t) {}

    void run() {
        std::stack<TokenType> controlStack; // Universal State Machine Stack

        while (cur < tokens.size() && tokens[cur].type != TokenType::END_OF_FILE) {
            Token t = tokens[cur];

            // Structural bypass - doesn't change active state
            if (t.type == TokenType::SCRIPT_AREA || t.type == TokenType::START_SCRIPT || t.type == TokenType::NEWLINE) {
                cur++; continue;
            }
            if (t.type == TokenType::END_SCRIPT) break;

            // PDA STATE: DECLARATION
            if (t.type == TokenType::DECLARE) {
                controlStack.push(TokenType::DECLARE);
                cur += 2; // Skip DECLARE and Type
                while (cur < tokens.size() && tokens[cur].type != TokenType::NEWLINE) {
                    if (tokens[cur].type == TokenType::COMMA) {
                        cur++;
                    } else if (tokens[cur].type == TokenType::IDENTIFIER) {
                        std::string name = tokens[cur++].value;
                        if (cur < tokens.size() && tokens[cur].type == TokenType::ASSIGN) {
                            cur++;
                            vars[name] = evaluate(tokens[cur++]); // Fixed variable assignment here
                        } else {
                            vars[name] = "";
                        }
                    } else {
                        break;
                    }
                }
                controlStack.pop();
            }
            // PDA STATE: ASSIGNMENT
            else if (t.type == TokenType::IDENTIFIER) {
                controlStack.push(TokenType::ASSIGN);
                std::vector<std::string> targets;
                while (cur < tokens.size() && tokens[cur].type == TokenType::IDENTIFIER) {
                    targets.push_back(tokens[cur++].value);
                    if (cur < tokens.size() && tokens[cur].type == TokenType::ASSIGN) cur++;
                    else break;
                }
                if (cur < tokens.size()) {
                    std::string val = evaluate(tokens[cur++]);
                    for (auto& v : targets) vars[v] = val;
                }
                controlStack.pop();
            }
            // PDA STATE: PRINT
            else if (t.type == TokenType::PRINT) {
                controlStack.push(TokenType::PRINT);
                cur++; if (cur < tokens.size() && tokens[cur].type == TokenType::COLON) cur++;

                while (cur < tokens.size() && tokens[cur].type != TokenType::NEWLINE && tokens[cur].type != TokenType::END_SCRIPT) {
                    Token p = tokens[cur++];

                    // Handling unquoted PDA brackets (just in case)
                    if (p.type == TokenType::LBRACKET) {
                        std::stack<int> pda;
                        pda.push(1);
                        while (cur < tokens.size() && !pda.empty()) {
                            if (tokens[cur].type == TokenType::LBRACKET) {
                                std::cout << "["; pda.push(1);
                            } else if (tokens[cur].type == TokenType::RBRACKET) {
                                pda.pop();
                                if (!pda.empty()) std::cout << "]";
                            } else {
                                std::cout << evaluate(tokens[cur]);
                            }
                            cur++;
                        }
                    }
                    else if (p.type == TokenType::DOLLAR) {
                        std::cout << "\n";
                    }
                    else if (p.type == TokenType::AMPERSAND) {
                        // Consumed: Never printed
                    }
                    else {
                        // Prints variables, numbers, and dynamically unescapes strings
                        std::cout << evaluate(p);
                    }
                }
                controlStack.pop();
            }
            else cur++;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::ifstream f(argv[1]);
    std::stringstream ss; ss << f.rdbuf();
    Lexer lex(ss.str());
    std::vector<Token> tokens;
    Token t;
    while ((t = lex.nextToken()).type != TokenType::END_OF_FILE) tokens.push_back(t);
    Interpreter inter(tokens);
    inter.run();
    return 0;
}