#ifndef COMMENT_H
#define COMMENT_H

#include <string>

static constexpr char ZERO_CHAR = '0';
static constexpr char ONE_CHAR = '1';

static constexpr char LEFT_CHAR = '<';
static constexpr char RIGHT_CHAR = '>';
static constexpr char NOP_CHAR = '-';

static constexpr char BINDING_OPEN = '(';
static constexpr char BINDING_CLOSE = ')';
static constexpr char EXPLICIT_STATE = ':';

static constexpr char COMMENT_CHAR = '#';
static constexpr char ACTION_CHAR = '!';

static const std::string MODULE_STRING = "module";

static const std::string AQUA_SOURCE_EXT = ".aquasrc";
static const std::string AQUA_COMPILED_EXT = ".aquacomp";

static const std::string DEFAULT_TEXT = "\033[0m";
static const std::string FAULT_TEXT = "\033[31;1m";
static const std::string INFO_TEXT = "\033[32;1m";

#endif // GLOBALS_H
