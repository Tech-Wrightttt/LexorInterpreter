#include <iostream>

#include <fstream>

#include <sstream>

#include <string>

#include <vector>

#include <map>

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

char c = src[pos];

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



if (c == '\'' || c == '\"') {

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



public:

Interpreter(std::vector<Token> t) : tokens(t) {}



void run() {

while (cur < tokens.size() && tokens[cur].type != TokenType::END_OF_FILE) {

Token t = tokens[cur];



// 1. Skip structural keywords

if (t.type == TokenType::SCRIPT_AREA || t.type == TokenType::START_SCRIPT || t.type == TokenType::NEWLINE) {

cur++; continue;

}

if (t.type == TokenType::END_SCRIPT) break;



// 2. Declaration [cite: 9, 10, 11]

if (t.type == TokenType::DECLARE) {

cur++; cur++; // Skip DECLARE and Type

do {

if (tokens[cur].type == TokenType::COMMA) cur++;

std::string name = tokens[cur++].value;

if (cur < tokens.size() && tokens[cur].type == TokenType::ASSIGN) {

cur++; vars[name] = tokens[cur++].value;

} else vars[name] = "";

} while (cur < tokens.size() && tokens[cur].type == TokenType::COMMA);

}

// 3. Assignment [cite: 12, 13]

else if (t.type == TokenType::IDENTIFIER) {

std::vector<std::string> targets;

while (cur < tokens.size() && tokens[cur].type == TokenType::IDENTIFIER) {

targets.push_back(tokens[cur++].value);

if (cur < tokens.size() && tokens[cur].type == TokenType::ASSIGN) cur++;

else break;

}

if (cur < tokens.size() && tokens[cur].type != TokenType::NEWLINE) {

std::string val = tokens[cur++].value;

for (auto& v : targets) vars[v] = val;

}

}

// 4. PRINT Logic [cite: 15, 30, 31, 32]

else if (t.type == TokenType::PRINT) {

cur++; if (cur < tokens.size() && tokens[cur].type == TokenType::COLON) cur++;



while (cur < tokens.size() && tokens[cur].type != TokenType::NEWLINE && tokens[cur].type != TokenType::END_SCRIPT) {

Token p = tokens[cur++];



if (p.type == TokenType::IDENTIFIER) {

std::cout << vars[p.value];

} else if (p.type == TokenType::INT_LIT || p.type == TokenType::FLOAT_LIT ||

p.type == TokenType::BOOL_LIT || p.type == TokenType::CHAR_LIT ||

p.type == TokenType::STRING_LIT) {

std::cout << p.value;

} else if (p.type == TokenType::DOLLAR) {

std::cout << "\n"; // Next line/carriage return [cite: 30]

} else if (p.type == TokenType::LBRACKET) {

// RECURSIVE ESCAPE LOGIC

// We print the inner content and move past the matching closing bracket

if (cur < tokens.size()) {

std::cout << tokens[cur].value;

cur++;

if (cur < tokens.size() && tokens[cur].type == TokenType::RBRACKET) cur++;

}

}

// Consume concatenator &

if (cur < tokens.size() && tokens[cur].type == TokenType::AMPERSAND) cur++;

}

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