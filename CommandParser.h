#pragma once

#include "Types.h"

#include <string>

class CommandParser {
public:
    Command parse(const std::string& line) const;

private:
    static std::string toLower(std::string value);
    static bool parseNonNegativeInt(const std::string& text, int& value);
};
