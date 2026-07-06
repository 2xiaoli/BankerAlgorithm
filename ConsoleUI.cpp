#include "ConsoleUI.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

void ConsoleUI::run() {
    printWelcome();

    std::string line;
    while (running_) {
        std::cout << "banker> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n检测到输入结束，程序退出。\n";
            break;
        }

        Command command = parser_.parse(line);
        executeCommand(command);
    }
}

void ConsoleUI::printWelcome() const {
    std::cout << "============================================\n";
    std::cout << " 银行家算法资源分配模拟系统\n";
    std::cout << " 操作系统课程设计 - 死锁避免\n";
    std::cout << "============================================\n";
    std::cout << "输入 help 查看命令，输入 demo 可快速载入样例。\n\n";
}

void ConsoleUI::printGeneralHelp() const {
    std::cout << "\n可用命令\n";
    std::cout << "  help                         显示全部命令\n";
    std::cout << "  help <command>               查看指定命令用法\n";
    std::cout << "  demo                         载入经典银行家算法样例\n";
    std::cout << "  new <进程数> <资源类型数>    引导式输入一个新系统\n";
    std::cout << "  show                         显示 Available、Max、Allocation、Need\n";
    std::cout << "  safe                         执行安全性检查\n";
    std::cout << "  request <pid> <r1> ... <rm>  进程请求资源\n";
    std::cout << "  release <pid> <r1> ... <rm>  进程释放资源\n";
    std::cout << "  reset                        清空当前系统状态\n";
    std::cout << "  exit / quit                  退出程序\n";
    std::cout << "\n示例\n";
    std::cout << "  demo\n";
    std::cout << "  request 1 1 0 2\n";
    std::cout << "  release 1 1 0 0\n\n";
}

void ConsoleUI::printCommandHelp(const std::string& commandName) const {
    const std::string name = toLower(commandName);
    if (name == "help") {
        std::cout << "用法：help 或 help <command>\n";
    } else if (name == "demo") {
        std::cout << "用法：demo\n说明：载入 5 个进程、3 类资源的经典样例。\n";
    } else if (name == "new") {
        std::cout << "用法：new <进程数> <资源类型数>\n";
        std::cout << "说明：随后按提示输入总资源、Max 矩阵和 Allocation 矩阵。\n";
    } else if (name == "show") {
        std::cout << "用法：show\n说明：显示系统当前各矩阵和可用资源。\n";
    } else if (name == "safe") {
        std::cout << "用法：safe\n说明：执行银行家算法安全性检查并输出安全序列。\n";
    } else if (name == "request") {
        std::cout << "用法：request <pid> <r1> ... <rm>\n";
        std::cout << "说明：pid 从 0 开始，资源值数量必须等于资源类型数。\n";
    } else if (name == "release") {
        std::cout << "用法：release <pid> <r1> ... <rm>\n";
        std::cout << "说明：释放量不能超过该进程当前 Allocation。\n";
    } else if (name == "reset") {
        std::cout << "用法：reset\n说明：清空当前系统数据。\n";
    } else if (name == "exit" || name == "quit") {
        std::cout << "用法：exit 或 quit\n说明：退出程序。\n";
    } else {
        std::cout << "没有找到命令 \"" << commandName << "\" 的帮助。输入 help 查看全部命令。\n";
    }
    std::cout << "\n";
}

void ConsoleUI::executeCommand(const Command& command) {
    switch (command.type) {
        case CommandType::Empty:
            return;
        case CommandType::Invalid:
            std::cout << "[错误] " << command.error << " 输入 help 查看命令帮助。\n\n";
            return;
        case CommandType::Help:
            if (command.args.empty()) {
                printGeneralHelp();
            } else {
                printCommandHelp(command.args.front());
            }
            return;
        case CommandType::Demo:
            printResult(system_.loadDemo());
            return;
        case CommandType::NewSystem: {
            const int processCount = command.numbers[0];
            const int resourceCount = command.numbers[1];
            if (processCount < Limits::MinProcessCount || processCount > Limits::MaxProcessCount) {
                std::cout << "[错误] 进程数量必须在 1 到 20 之间。\n\n";
                return;
            }
            if (resourceCount < Limits::MinResourceCount || resourceCount > Limits::MaxResourceCount) {
                std::cout << "[错误] 资源类型数量必须在 1 到 10 之间。\n\n";
                return;
            }
            handleNewSystem(processCount, resourceCount);
            return;
        }
        case CommandType::Show:
            if (!system_.isInitialized()) {
                printNeedInitialized();
                return;
            }
            std::cout << system_.showState();
            return;
        case CommandType::Safe:
            if (!system_.isInitialized()) {
                printNeedInitialized();
                return;
            }
            printResult(system_.checkSafety());
            return;
        case CommandType::Request: {
            if (!system_.isInitialized()) {
                printNeedInitialized();
                return;
            }
            ResourceVector request(command.numbers.begin() + 1, command.numbers.end());
            printResult(system_.requestResources(command.numbers.front(), request));
            return;
        }
        case CommandType::Release: {
            if (!system_.isInitialized()) {
                printNeedInitialized();
                return;
            }
            ResourceVector release(command.numbers.begin() + 1, command.numbers.end());
            printResult(system_.releaseResources(command.numbers.front(), release));
            return;
        }
        case CommandType::Reset:
            system_.reset();
            std::cout << "系统状态已清空。可使用 demo 或 new 重新初始化。\n\n";
            return;
        case CommandType::Exit:
            running_ = false;
            std::cout << "程序已退出。\n";
            return;
    }
}

void ConsoleUI::handleNewSystem(int processCount, int resourceCount) {
    std::cout << "\n开始初始化系统。每行输入 " << resourceCount
              << " 个非负整数，使用空格分隔；输入 cancel 可取消。\n";

    ResourceVector totalResources;
    if (!readVector("请输入系统总资源向量 Total: ", resourceCount, totalResources)) {
        std::cout << "初始化已取消。\n\n";
        return;
    }

    ResourceMatrix max;
    if (!readMatrix("请输入 Max 最大需求矩阵", processCount, resourceCount, max)) {
        std::cout << "初始化已取消。\n\n";
        return;
    }

    ResourceMatrix allocation;
    if (!readMatrix("请输入 Allocation 已分配矩阵", processCount, resourceCount, allocation)) {
        std::cout << "初始化已取消。\n\n";
        return;
    }

    printResult(system_.initialize(totalResources, max, allocation));
}

void ConsoleUI::printResult(const OperationResult& result) const {
    std::cout << (result.success ? "[成功] " : "[失败] ") << result.message << "\n";
    if (!result.safeSequence.empty()) {
        std::cout << "安全序列: ";
        for (std::size_t i = 0; i < result.safeSequence.size(); ++i) {
            if (i > 0) {
                std::cout << " -> ";
            }
            std::cout << "P" << result.safeSequence[i];
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void ConsoleUI::printNeedInitialized() const {
    std::cout << "[提示] 系统尚未初始化。请先输入 demo 载入样例，或使用 new <进程数> <资源类型数> 创建系统。\n\n";
}

bool ConsoleUI::readVector(const std::string& prompt, int expectedSize, ResourceVector& values) const {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        if (toLower(line) == "cancel") {
            return false;
        }

        std::string error;
        if (parseVectorLine(line, expectedSize, values, error)) {
            return true;
        }
        std::cout << "[错误] " << error << " 请重新输入，或输入 cancel 取消。\n";
    }
}

bool ConsoleUI::readMatrix(const std::string& title,
                           int rows,
                           int columns,
                           ResourceMatrix& matrix) const {
    std::cout << title << "：\n";
    matrix.clear();
    matrix.reserve(rows);

    for (int i = 0; i < rows; ++i) {
        ResourceVector row;
        std::ostringstream prompt;
        prompt << "  P" << i << ": ";
        if (!readVector(prompt.str(), columns, row)) {
            return false;
        }
        matrix.push_back(row);
    }
    return true;
}

bool ConsoleUI::parseVectorLine(const std::string& line,
                                int expectedSize,
                                ResourceVector& values,
                                std::string& error) const {
    std::istringstream input(line);
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }

    if (static_cast<int>(tokens.size()) != expectedSize) {
        std::ostringstream message;
        message << "需要输入 " << expectedSize << " 个整数，当前输入了 " << tokens.size() << " 个。";
        error = message.str();
        return false;
    }

    values.clear();
    values.reserve(expectedSize);
    for (const auto& item : tokens) {
        if (!std::all_of(item.begin(), item.end(), [](unsigned char ch) {
                return std::isdigit(ch) != 0;
            })) {
            error = "存在非整数或负数参数 \"" + item + "\"。";
            return false;
        }

        std::istringstream numberInput(item);
        long long parsed = 0;
        numberInput >> parsed;
        if (!numberInput || parsed > Limits::MaxResourceValue) {
            error = "资源值过大，单个资源数不能超过 100000。";
            return false;
        }
        values.push_back(static_cast<int>(parsed));
    }
    return true;
}

std::string ConsoleUI::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
