#include "CommandParser.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

Command CommandParser::parse(const std::string& line) const {
    std::istringstream input(line);
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return {CommandType::Empty, {}, {}, ""};
    }

    const std::string commandName = toLower(tokens.front());
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    Command command;
    command.args = args;

    auto parseNumericArgs = [&]() -> bool {
        for (const auto& arg : args) {
            int number = 0;
            if (!parseNonNegativeInt(arg, number)) {
                command.type = CommandType::Invalid;
                command.error = "参数 \"" + arg + "\" 不是合法的非负整数。";
                return false;
            }
            command.numbers.push_back(number);
        }
        return true;
    };

    if (commandName == "help") {
        if (args.size() > 1) {
            command.type = CommandType::Invalid;
            command.error = "help 命令最多接收一个命令名参数。";
            return command;
        }
        command.type = CommandType::Help;
        return command;
    }

    if (commandName == "demo") {
        command.type = args.empty() ? CommandType::Demo : CommandType::Invalid;
        command.error = args.empty() ? "" : "demo 命令不需要参数。";
        return command;
    }

    if (commandName == "new") {
        if (args.size() != 2) {
            return {CommandType::Invalid, args, {}, "new 命令需要 2 个参数：进程数量和资源类型数量。"};
        }
        command.type = CommandType::NewSystem;
        if (!parseNumericArgs()) {
            return command;
        }
        return command;
    }

    if (commandName == "show") {
        command.type = args.empty() ? CommandType::Show : CommandType::Invalid;
        command.error = args.empty() ? "" : "show 命令不需要参数。";
        return command;
    }

    if (commandName == "safe") {
        command.type = args.empty() ? CommandType::Safe : CommandType::Invalid;
        command.error = args.empty() ? "" : "safe 命令不需要参数。";
        return command;
    }

    if (commandName == "request") {
        if (args.size() < 2) {
            return {CommandType::Invalid, args, {}, "request 命令至少需要进程编号和一个资源请求值。"};
        }
        command.type = CommandType::Request;
        if (!parseNumericArgs()) {
            return command;
        }
        return command;
    }

    if (commandName == "release") {
        if (args.size() < 2) {
            return {CommandType::Invalid, args, {}, "release 命令至少需要进程编号和一个资源释放值。"};
        }
        command.type = CommandType::Release;
        if (!parseNumericArgs()) {
            return command;
        }
        return command;
    }

    if (commandName == "reset") {
        command.type = args.empty() ? CommandType::Reset : CommandType::Invalid;
        command.error = args.empty() ? "" : "reset 命令不需要参数。";
        return command;
    }

    if (commandName == "exit" || commandName == "quit") {
        command.type = args.empty() ? CommandType::Exit : CommandType::Invalid;
        command.error = args.empty() ? "" : "exit/quit 命令不需要参数。";
        return command;
    }

    return {CommandType::Invalid, args, {}, "未知命令 \"" + tokens.front() + "\"。"};
}

std::string CommandParser::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool CommandParser::parseNonNegativeInt(const std::string& text, int& value) {
    if (text.empty()) {
        return false;
    }
    if (!std::all_of(text.begin(), text.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        })) {
        return false;
    }

    std::istringstream input(text);
    long long parsed = 0;
    input >> parsed;
    if (!input || parsed > std::numeric_limits<int>::max()) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}
