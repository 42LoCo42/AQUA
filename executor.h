#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include "module.h"

class Executor {
	std::string m_state {};
	std::string m_mem {};
	size_t m_head_pos = 0;
	Module m_module;

public:
	explicit Executor(Module& m);

	void setMem(const std::string& mem);
	void setHeadPos(size_t head_pos);

	void print() const;
	auto step() -> bool;
};

#endif // EXECUTOR_H
