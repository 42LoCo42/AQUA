#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <vector>

class Compiler {
public:
	static auto isWhitespace(char c) -> bool;
	static auto isText(char c) -> bool;
	static auto isNumber(char c) -> bool;

	static void compile(const std::string& filename, std::vector<std::string>& result, const std::string& mod_path = "", uint verbosity = 0);
};

#endif // COMPILER_H
