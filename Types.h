#pragma once

#include <string>
#include <vector>

using ResourceVector = std::vector<int>;
using ResourceMatrix = std::vector<ResourceVector>;

namespace Limits {
constexpr int MinProcessCount = 1;
constexpr int MaxProcessCount = 20;
constexpr int MinResourceCount = 1;
constexpr int MaxResourceCount = 10;
constexpr int MaxResourceValue = 100000;
}

enum class CommandType {
    Empty,
    Help,
    Demo,
    NewSystem,
    Show,
    Safe,
    Request,
    Release,
    Reset,
    Exit,
    Invalid
};

struct OperationResult {
    bool success = false;
    std::string message;
    std::vector<int> safeSequence;
};

struct SystemState {
    bool initialized = false;
    ResourceVector available;
    ResourceMatrix max;
    ResourceMatrix allocation;
    ResourceMatrix need;
};

struct Command {
    CommandType type = CommandType::Empty;
    std::vector<std::string> args;
    std::vector<int> numbers;
    std::string error;
};
