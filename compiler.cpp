#include "compiler.h"
#include "utils.h"
#include "globals.h"
#include "module.h"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <algorithm>
using namespace std;

auto Compiler::isWhitespace(char c) -> bool {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
auto Compiler::isText(char c) -> bool {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

auto Compiler::isNumber(char c) -> bool {
	return c >= '0' && c <= '9';
}

void printRules(const vector<Rule>& rules) {
	for(auto& r : rules) {
		string move_string = "ERROR_INVALID_MOVE";
		if(r.dir == LEFT) move_string = "<";
		else if(r.dir == RIGHT) move_string = ">";
		else if(r.dir == NOP) move_string = "-";
		cout << r.old_state + " " + string(1, r.old_char) + " " + r.new_state + " " + string(1, r.new_char) + " " + move_string << endl;
	}
}

auto getModuleIndex(vector<pair<Module, size_t>>& module_list, const string& name) -> size_t {
	size_t index = 0;
	for(; index < module_list.size(); ++index) {
		if(module_list[index].first.name() == name) break;
	}
	if(index >= module_list.size()) {
		throw runtime_error("Compiler: Module " + name + " not loaded!");
	}
	return index;
}

void getModuleBindings(Module& module, const string& binding_string, const string& default_binding, vector<string>& bindings) {

	bindings.clear();

	if(binding_string.empty()) { // type 0 binding
		// bind all end states to default state
		bindings = vector<string>(module.end_states().size(), default_binding);
		return;
	}

	// split binding string
	vector<string> binding_string_tokens {};
	Utils::split(binding_string, " ", binding_string_tokens);

	// check for type 1 binding
	size_t first_expl_pos = binding_string_tokens[0].find_first_of(EXPLICIT_STATE);
	if(first_expl_pos == string::npos) {
		string& first_token = binding_string_tokens[0]; // will always exist due to splitting logic
		if(binding_string_tokens.size() != 1) {
			throw runtime_error("Compiler: No bindings allowed after type 1 binding at: " + binding_string);
		}

		if(first_token[0] == ACTION_CHAR) { // special type 1 binds all end states to the specified state
			first_token.erase(first_token.begin()); // remove ACTION_CHAR
			bindings = vector<string>(module.end_states().size(), first_token);
			return;
		}

		// pure type 1
		if(module.end_states().size() > 1) {
			throw runtime_error("Compiler: Pure type 1 binding not allowed with multiple end states at: " + binding_string);
		}
		bindings.push_back(first_token);
		return;
	}

	// type 2 binding, split binding tokens into end and target state
	vector<pair<string, string>> expl_bindings {};
	for(auto& token : binding_string_tokens) {
		size_t expl_pos = token.find_first_of(EXPLICIT_STATE);
		expl_bindings.emplace_back(token.substr(0, expl_pos), token.substr(expl_pos+1, token.size() - (expl_pos + 1)));
	}

	// bind all end states
	for(auto& es : module.end_states()) {
		// find explicit binding
		size_t expl_index = 0;
		for(; expl_index < expl_bindings.size(); ++expl_index) {
			if(es == module.name() + BINDING_OPEN + expl_bindings[expl_index].first) {
				break;
			}
		}

		if(expl_index < expl_bindings.size()) { // explicit binding found
			bindings.push_back(expl_bindings[expl_index].second);
		} else { // use default binding
			bindings.push_back(default_binding);
		}
	}
}

void insertModule(pair<Module, size_t>& module_pair, vector<Rule>& target, const string& entry, const vector<string>& exits) {
	Module& module = module_pair.first;

	if(module.end_states().size() != exits.size()) {
		throw runtime_error("Compiler: On insertion of module " + module.name() + ": Can't bind " + to_string(module.end_states().size()) + " end states to " + to_string(exits.size()) + " exits!");
	}

	// define invocation scope
	const string scope = string("_i") + to_string(module_pair.second) + BINDING_CLOSE;

	// match end states to bindings
	vector<pair<const string*, const string*>> bindings;
	for(size_t i=0; i<module.end_states().size(); ++i) {
		bindings.emplace_back();
		bindings[bindings.size()-1].first = &module.end_states()[i];
		bindings[bindings.size()-1].second = &exits[i];
	}

	// insert module entry point
	const string entry_state = string(module.start_state()).append(scope);
	target.emplace_back(entry, ZERO_CHAR, entry_state, ZERO_CHAR, NOP);
	target.emplace_back(entry, ONE_CHAR, entry_state, ONE_CHAR, NOP);


	// insert rules of module. rules are copied for scoping modifications
	for(const auto& r : module.rules()) {
		const string* new_state_prefix = nullptr;
		const string* new_state_suffix = nullptr;

		// is the new state an end state? if so, bind it
		bool is_end_state = false; // for some reason, find_if returns a nonzero but invalid pointer if the predicate never succeeded. this thing keeps track of that.
		new_state_prefix = find_if(bindings.begin(), bindings.end(), [&](const pair<const string*, const string*>& b) {
			if(*b.first == r.new_state) {
				is_end_state = true;
				return true;
			}
			return false;
		})->second;

		// this is no end state, scope normally (module-internally)
		if(!is_end_state) {
			new_state_prefix = &r.new_state;
			new_state_suffix = &scope;
		}

		// create invocation scope for old state and import already scoped new state
		target.emplace_back(r.old_state + scope, r.old_char,
							*new_state_prefix + (new_state_suffix ? *new_state_suffix : ""),
							r.new_char, r.dir);
	}

	// update invocation count
	++module_pair.second;
}

auto NOPtimizeRule(const vector<Rule>& rules, Rule& rule, vector<string>& end_states) -> bool {
	while(rule.dir == NOP) {
		// find target rule
		bool found = false;
		for(auto& tr : rules) {
			if(tr.old_char == rule.new_char && tr.old_state == rule.new_state) {
				rule.dir = tr.dir;
				rule.new_char = tr.new_char;
				rule.new_state = tr.new_state;

				// is the new state an end state? if so, we are done here
				if(any_of(end_states.begin(), end_states.end(), [&](const string& es) {
					return rule.new_state == es;
				})) return true;

				found = true;
				break;
			}
		}
		if(!found) {
			throw runtime_error("Compiler: While NOPtimizing: state not found: " + rule.new_state);
		}
	}
	return true;
}

void getReachableStates(vector<Rule>& rules, const string& start_state, const vector<string>& end_states, vector<string>& result) {
	// find all rules that start with start_state
	for(auto& rule : rules) {
		if(rule.old_state == start_state) {
			bool stop = rule.dir == NOP; // after NOPtimizing, all rules except ending ones move
			if(stop) continue; // ending rules don't reach any states

			// is the new state already reachable?
			stop = any_of(result.begin(), result.end(), [&](const string& rs) {
				return rs == rule.new_state;
			});
			if(stop) continue; // state already reachable, we don't need to scan for it

			// this rule reaches a new state, save it and recurse for it
			result.push_back(rule.new_state);
			getReachableStates(rules, rule.new_state, end_states, result);
		}
	}
}

void Compiler::compile(const string& filename, vector<string>& result, const string& mod_path, uint verbosity) {
	result.clear();

	// read file
	vector<string> lines;
	if(!Utils::readFile(filename, lines)) {
		throw runtime_error("Compiler: During startup: File " + filename + " not found!");
	}

	// the Module list. int is the invocation count for each module
	vector<pair<Module, size_t>> module_list;

	string start_state {};
	string current_state {};
	size_t next_implicit_state = 0;
	vector<string> end_states {};

	vector<Rule> rules;

	// compile program
	if(verbosity > 0) cout << "Beginning compilation of file " << INFO_TEXT << filename << DEFAULT_TEXT << " ...\n";
	auto start_time = chrono::high_resolution_clock::now();
	for(auto& line : lines) {
		if(line.empty()) continue;
		if(line[0] == COMMENT_CHAR) continue;

		// parse action
		if(line[0] == ACTION_CHAR) {
			// parse module
			size_t pos = line.find_first_of(MODULE_STRING);
			if(pos < string::npos) {
				string module_name = line.substr(pos + MODULE_STRING.size() + 1, line.size() - (pos + MODULE_STRING.size() + 1));

				// is module already loaded?
				for(auto& m : module_list) {
					if(m.first.name() == module_name) {
						throw runtime_error("Compiler: Module " + module_name + " already loaded!");
					}
				}
				if(verbosity > 0) cout << "Loading module " << INFO_TEXT << module_name << DEFAULT_TEXT << " ... ";

				// store new module
				module_list.emplace_back(Module(module_name, mod_path), 0);
				Module& m = module_list[module_list.size()-1].first;

				// define scope
				const string scope = module_name + BINDING_OPEN;

				// apply scope to start state
				m.start_state() = scope + m.start_state();

				// apply scope to end states
				for(auto& es : m.end_states()) {
					es.insert(es.begin(), scope.begin(), scope.end());
					//es = scope + es;
				}

				// apply scope to rules
				for(auto& rule : module_list[module_list.size()-1].first.rules()) {
					rule.old_state = scope + rule.old_state;
					rule.new_state = scope + rule.new_state;
				}
				if(verbosity > 0) cout << "Done.\n";
			}
		}

		// try to load line as state header
		else if(end_states.empty()) {
			Utils::split(line, " ", end_states);
			if(end_states.size() < 2) {
				throw runtime_error("Compiler: During startup: Parsing of " + line + " as state header failed: too few tokens!");
			}
			current_state = end_states[0];
			start_state = current_state; // save start state for later
			end_states.erase(end_states.begin());
			if(verbosity > 0) {
				cout << "State header loaded!\n";
				cout << "Start state is " << INFO_TEXT << current_state << DEFAULT_TEXT << ", end states are " << INFO_TEXT;
				for(auto& s : end_states) {
					cout << s << ' ';
				}
				cout << DEFAULT_TEXT << '\n';
			}

			result.push_back(line);
		}

		// load line as program data, read single characters
		else {
			size_t pos = 0;
			string symbol {};
			while(pos < line.size()) {
				if(line[pos] == COMMENT_CHAR) {
					pos = line.size(); // skip rest of line
				}

				if(line[pos] == LEFT_CHAR) {
					rules.emplace_back(current_state, ZERO_CHAR, to_string(next_implicit_state), ZERO_CHAR, LEFT);
					rules.emplace_back(current_state, ONE_CHAR, to_string(next_implicit_state), ONE_CHAR, LEFT);

					current_state = to_string(next_implicit_state);
					++next_implicit_state;
					++pos;
				}
				else if(line[pos] == RIGHT_CHAR) {
					rules.emplace_back(current_state, ZERO_CHAR, to_string(next_implicit_state), ZERO_CHAR, RIGHT);
					rules.emplace_back(current_state, ONE_CHAR, to_string(next_implicit_state), ONE_CHAR, RIGHT);

					current_state = to_string(next_implicit_state);
					++next_implicit_state;
					++pos;
				}
				else if(symbol.empty() && line[pos] == ZERO_CHAR) {
					rules.emplace_back(current_state, ZERO_CHAR, to_string(next_implicit_state), ZERO_CHAR, NOP);
					rules.emplace_back(current_state, ONE_CHAR, to_string(next_implicit_state), ZERO_CHAR, NOP);

					current_state = to_string(next_implicit_state);
					++next_implicit_state;
					++pos;
				}
				else if(symbol.empty() && line[pos] == ONE_CHAR) {
					rules.emplace_back(current_state, ZERO_CHAR, to_string(next_implicit_state), ONE_CHAR, NOP);
					rules.emplace_back(current_state, ONE_CHAR, to_string(next_implicit_state), ONE_CHAR, NOP);

					current_state = to_string(next_implicit_state);
					++next_implicit_state;
					++pos;
				}
				else if(isWhitespace(line[pos])) {
					if(symbol.empty()) ++pos;
					else { // this is an in-place module call, route all outputs to next implicit state
						vector<string> bindings;
						size_t index = getModuleIndex(module_list, symbol);
						getModuleBindings(module_list[index].first, "", to_string(next_implicit_state), bindings);
						insertModule(module_list[index], rules, current_state, bindings);

						current_state = to_string(next_implicit_state);
						++next_implicit_state;
						++pos;
						symbol = {};
					}
				}
				else if(line[pos] == BINDING_OPEN) { // this is a bound module call
					if(symbol.empty()) {
						throw runtime_error("Compiler: missing module name on line " + line);
					}

					size_t closing = line.find(BINDING_CLOSE, pos);
					if(closing == string::npos) {
						throw runtime_error("Compiler: Unterminated " + string(1, BINDING_OPEN) + " on line " + line);
					}
					// locate bindings
					string binding_string = line.substr(pos + 1, closing - (pos + 1));
					pos = closing + 1; // consume enclosed binding string
					vector<string> bindings;

					// insert bound module
					size_t index = getModuleIndex(module_list, symbol);
					getModuleBindings(module_list[index].first, binding_string, to_string(next_implicit_state), bindings);
					insertModule(module_list[index], rules, current_state, bindings);

					current_state = to_string(next_implicit_state);
					++next_implicit_state;
					symbol = {};
				}
				else if(!symbol.empty() && line[pos] == EXPLICIT_STATE) {
					// bind current state to symbol
					rules.emplace_back(current_state, ZERO_CHAR, symbol, ZERO_CHAR, NOP);
					rules.emplace_back(current_state, ONE_CHAR, symbol, ONE_CHAR, NOP);

					// change current state to symbol
					current_state = symbol;
					symbol = {};
					++pos;
				}
				else if(
						(symbol.empty() && isText(line[pos])) || // read text
						(symbol.empty() && line[pos] != ZERO_CHAR && line[pos] != ONE_CHAR) || // read all numbers except 1 and 0
						(!symbol.empty() && (isText(line[pos]) || isNumber(line[pos]))) // tmp is already filled, read every number and characte
						) {
					symbol.push_back(line[pos]);
					++pos;
				} else {
					throw runtime_error("Compiler: Invalid character " + string(1, line[pos]) + " on line " + line + " !");
				}
			}
		}
	}

	// bind current state to first end binding
	rules.emplace_back(current_state, ZERO_CHAR, end_states[0], ZERO_CHAR, NOP);
	rules.emplace_back(current_state, ONE_CHAR, end_states[0], ONE_CHAR, NOP);


	if(verbosity > 0) {
		cout << "Compilation complete, currently " << INFO_TEXT << rules.size() << DEFAULT_TEXT << " rules.\n";
		if(verbosity >= 2) {
			cout << "\nCurrent rule table:\n";
			printRules(rules);
			cout << '\n';
		}
		cout << "NOPtimizer started... ";
	}

	for(auto& rule : rules) { // find next NOP rule
		if(rule.dir == NOP) {
			for(auto& es : end_states) {
				if(rule.new_state == es) goto optimize_next_rule; // can't optimize ending rule
			}

			if(!NOPtimizeRule(rules, rule, end_states)) continue; // rule can't be optimized further
optimize_next_rule:
			continue;
		}
	}

	if(verbosity > 0) {
		cout << "Done!\n";
		if(verbosity >= 2) {
			cout << "\nCurrent rule table:\n";
			printRules(rules);
			cout << '\n';
		}
		cout << "Searching for reachable states... ";
	}

	vector<string> reachable_states {};
	getReachableStates(rules, start_state, end_states, reachable_states);
	reachable_states.push_back(start_state); // we also reach the start state implicitly

	if(verbosity > 0) {
		cout << "Done with " << INFO_TEXT << reachable_states.size() << DEFAULT_TEXT << " reachable states!\n";
		if(verbosity >= 2) {
			cout << "\nReachable states:\n";
			for(auto& rs : reachable_states) {
				cout << rs << endl;
			}
			cout << '\n';
		}
		cout << "Removing unreachable rules... ";
	}

	bool changed = true;
	while(changed) {
		changed = false;

		for(size_t i=0; i<rules.size(); ++i) {
			Rule& rule = rules[i];
			bool is_reachable = any_of(reachable_states.begin(), reachable_states.end(), [&](const string& rs) {
				return rs == rule.old_state;
			});

			if(!is_reachable) {
				rules.erase(rules.begin() + static_cast<long>(i));
				changed = true;
				break;
			}
		}
	}

	auto elapsed_time = chrono::high_resolution_clock::now() - start_time;
	if(verbosity > 0) {
		cout << "Done, finished with " << INFO_TEXT << rules.size() << DEFAULT_TEXT << " rules!\n";
		cout << "Compilation took " << INFO_TEXT << chrono::duration_cast<chrono::microseconds>(elapsed_time).count() << " µs" << DEFAULT_TEXT << '\n';

		// list unused modules
		for(auto& m : module_list) {
			if(m.second == 0) {
				cout << FAULT_TEXT << "Module " << INFO_TEXT << m.first.name() << FAULT_TEXT << " was not used!\n" << DEFAULT_TEXT;
			}
		}
	}

	for(auto& r : rules) {
		string move_string = "ERROR_INVALID_MOVE";
		if(r.dir == LEFT) move_string = "<";
		else if(r.dir == RIGHT) move_string = ">";
		else if(r.dir == NOP) move_string = "-";
		result.push_back(r.old_state + " " + string(1, r.old_char) + " " + r.new_state + " " + string(1, r.new_char) + " " + move_string);
	}
}
