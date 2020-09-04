#include "executor.h"
#include "utils.h"
#include "globals.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
using namespace std;

/*
 * Rule syntax:
 * old_state old_char new_state new_char {< > -}
 */

Executor::Executor(Module& module) : m_state(module.start_state()), m_module(module) {}

void Executor::setMem(const std::string& mem) {
	m_mem = mem;
}

void Executor::setHeadPos(size_t head_pos) {
	m_head_pos = head_pos;
}

void Executor::print() const {
	cout << "In state: " << INFO_TEXT << m_state << DEFAULT_TEXT << '\n';
	cout << m_mem << '\n';

	for(size_t i=0; i<m_head_pos; ++i) {
		cout << ' ';
	}
	cout << "↑\n";
}

auto Executor::step() -> bool {
	// find rule
	for(auto& r : m_module.rules()) {
		if(r.old_state == m_state && r.old_char == m_mem[m_head_pos]) {
			m_state = r.new_state;
			m_mem[m_head_pos] = r.new_char;

			cout << INFO_TEXT << r.old_state <<  ' ' << r.old_char << DEFAULT_TEXT << " → " << INFO_TEXT << r.new_state << ' ' << r.new_char << ' ';

			if(r.dir == LEFT) {
				if(m_head_pos == 0) {
					m_mem.insert(m_mem.begin(), ZERO_CHAR);
				} else {
					--m_head_pos;
				}

				cout << '<';
			}
			else if(r.dir == RIGHT) {
				++m_head_pos;
				if(m_head_pos == m_mem.size()) {
					m_mem.push_back(ZERO_CHAR);
				}

				cout  << '>';
			} else cout << '-';
			cout << DEFAULT_TEXT;

			// if we are in an end state, return false to indicate that we are done
			return !any_of(m_module.end_states().begin(), m_module.end_states().end(), [&](const string& s) {
				return s == m_state;
			});
		}
	}

	throw runtime_error("Executor: No rule found for state " + m_state + " and char " + m_mem[m_head_pos]);
}
