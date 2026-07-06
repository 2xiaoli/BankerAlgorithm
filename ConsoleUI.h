#pragma once

#include "BankerSystem.h"
#include "CommandParser.h"

#include <string>

class ConsoleUI {
public:
    void run();

private:
    BankerSystem system_;
    CommandParser parser_;
    bool running_ = true;

    void printWelcome() const;
    void printGeneralHelp() const;
    void printCommandHelp(const std::string& commandName) const;
    void executeCommand(const Command& command);
    void handleNewSystem(int processCount, int resourceCount);
    void printResult(const OperationResult& result) const;
    void printNeedInitialized() const;

    bool readVector(const std::string& prompt, int expectedSize, ResourceVector& values) const;
    bool readMatrix(const std::string& title, int rows, int columns, ResourceMatrix& matrix) const;
    bool parseVectorLine(const std::string& line,
                         int expectedSize,
                         ResourceVector& values,
                         std::string& error) const;
    static std::string toLower(std::string value);
};
