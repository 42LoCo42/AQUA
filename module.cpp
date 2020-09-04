#include "module.h"
#include "globals.h"
#include "utils.h"
#include <vector>
#include <stdexcept>
using namespace std;

Module::Module(const string& name) : Module(name, "") {}

Module::Module(const string& name, const string& mod_path) : m_name(name) {
	vector<string> lines;
	// find file here or in mod_path
	bool found = Utils::readFile(name + AQUA_COMPILED_EXT, lines);
	if(!found) found = Utils::readFile(mod_path + name + AQUA_COMPILED_EXT, lines);

	if(found) {
		for(auto& l : lines) {
			if(l[0] == COMMENT_CHAR) continue;

			if(m_end_states.empty()) { // read line as state header
				Utils::split(l, DELIM, m_end_states);
				if(m_end_states.size() < 2) {
					throw runtime_error("Module: invalid state header: " + l);
				}
				m_start_state = m_end_states[0];
				m_end_states.erase(m_end_states.begin());
			} else { // read line as rule
				vector<string> tokens;
				Utils::split(l, DELIM, tokens);
				if(tokens.size() != TOKEN_COUNT) {
					throw runtime_error("Module: invalid rule length: " + l);
				}

				Rule new_rule;

				// read states
				new_rule.old_state = tokens[0];
				new_rule.new_state = tokens[2];

				// parse chars
				if(tokens[1].size() > 1 || tokens[3].size() > 1) {
					throw runtime_error("Module: Character token too large at: " + l);
				}
				if((tokens[1][0] != ZERO_CHAR && tokens[1][0] != ONE_CHAR) || (tokens[3][0] != ZERO_CHAR && tokens[3][0] != ONE_CHAR)) {
					throw runtime_error("Module: Invalid character token at: " + l);
				}
				new_rule.old_char = tokens[1][0];
				new_rule.new_char = tokens[3][0];

				// parse move direction
				if(tokens[4][0] == LEFT_CHAR) {
					new_rule.dir = LEFT;
				}
				else if(tokens[4][0] == RIGHT_CHAR) {
					new_rule.dir = RIGHT;
				}
				else if(tokens[4][0] == NOP_CHAR) {
					new_rule.dir = NOP;
				}
				else throw runtime_error("Module: Invalid move token at:" + l);

				m_rules.push_back(new_rule);
			}
		}
	} else throw runtime_error("Module: File " + mod_path + name + AQUA_COMPILED_EXT + " not found!");
}
