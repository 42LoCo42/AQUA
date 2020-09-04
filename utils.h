#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

class Utils {
public:
	static void split(const std::string& text, const std::string& delim, std::vector<std::string>& result);
	static auto fileExists(const std::string& filename) -> bool;
	static auto readFile(const std::string& filename, std::vector<std::string>& lines) -> bool;
	static auto writeFile(const std::string& filename, const std::vector<std::string>& lines) -> bool;
};

#endif // UTILS_H
