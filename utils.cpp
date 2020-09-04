#include "utils.h"
#include <sys/stat.h>
#include <fstream>
using namespace std;

void Utils::split(const string& text, const string& delim, vector<string>& result) {
	size_t search_begin = 0;

	result.clear();
	while(true) {
		size_t search_end = 0; // no scope redution to loop, we need to use this variable often
		search_end = text.find_first_of(delim, search_begin);
		if(search_end == string::npos) {
			result.emplace_back(text.substr(search_begin, text.size() - search_begin));
			return;
		}
		result.emplace_back(text.substr(search_begin, search_end - search_begin));
		search_begin = search_end + 1;
	}
}

auto Utils::fileExists(const string& filename) -> bool {
	struct stat buffer {};
	return (stat(filename.c_str(), &buffer) == 0);
}

auto Utils::readFile(const std::string& filename, std::vector<std::string>& lines) -> bool {
	if(!fileExists(filename)) return false;
	ifstream file_in(filename);
	char buf[BUFSIZ];

	lines.clear();
	if(!file_in.good()) return false;
	while(true) {
		file_in.getline(static_cast<char*>(buf), BUFSIZ, '\n');
		if(file_in.eof()) break;
		lines.emplace_back(static_cast<char*>(buf));
	}
	file_in.close();
	return true;
}

auto Utils::writeFile(const std::string& filename, const std::vector<std::string>& lines) -> bool {
	ofstream file_out(filename);

	if(!file_out.good()) return false;
	for(auto& line : lines) {
		file_out.write((line + '\n').c_str(), static_cast<long>(line.size()+1));
	}
	file_out.close();
	return false;
}
