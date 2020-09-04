#ifndef RULEPARSER_H
#define RULEPARSER_H

#include <vector>
#include <string>
#include <utility>

enum MoveDir {
	LEFT,
	RIGHT,
	NOP
};

struct Rule {
	std::string old_state;
	char old_char;
	std::string new_state;
	char new_char;
	MoveDir dir;

	Rule(std::string _old_state = "", char _old_char = ' ', std::string _new_state = "", char _new_char = ' ', MoveDir _dir = NOP) :
		old_state(std::move(_old_state)), old_char(_old_char), new_state(std::move(_new_state)), new_char(_new_char), dir(_dir) {}
};

static const std::string DELIM = " ";
static constexpr size_t TOKEN_COUNT = 5;

class Module {
	std::string m_name {};
	std::string m_start_state {};
	std::vector<std::string> m_end_states {};
	std::vector<Rule> m_rules {};

public:
	explicit Module(const std::string& name);
	Module(const std::string& name, const std::string& mod_path);
	//static auto parse(const std::string& data) -> Rule;

	auto name() -> std::string& {return m_name;}
	auto start_state() -> std::string& {return m_start_state;}
	auto end_states() -> std::vector<std::string>& {return m_end_states;}
	auto rules() -> std::vector<Rule>& {return m_rules;}
};

#endif // RULEPARSER_H
