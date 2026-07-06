#include "Encoding.h"

#include <clocale>
#include <iostream>

#ifdef _WIN32
#include <streambuf>
#include <string>
#include <windows.h>

namespace {
class Utf8ConsoleStreamBuf : public std::streambuf {
public:
    Utf8ConsoleStreamBuf(std::streambuf* fallback, HANDLE output)
        : fallback_(fallback), output_(output) {}

    ~Utf8ConsoleStreamBuf() override {
        sync();
    }

protected:
    int overflow(int ch) override {
        if (ch == traits_type::eof()) {
            return sync() == 0 ? traits_type::not_eof(ch) : traits_type::eof();
        }

        const char value = static_cast<char>(ch);
        return writeUtf8(&value, 1) ? ch : traits_type::eof();
    }

    std::streamsize xsputn(const char* text, std::streamsize count) override {
        return writeUtf8(text, static_cast<std::size_t>(count)) ? count : 0;
    }

    int sync() override {
        return flushPending(true) ? 0 : -1;
    }

private:
    std::streambuf* fallback_;
    HANDLE output_;
    std::string pending_;

    bool writeUtf8(const char* text, std::size_t count) {
        pending_.append(text, count);
        return flushPending(false);
    }

    bool flushPending(bool flushAll) {
        std::size_t prefixLength = completeUtf8PrefixLength(pending_);
        if (prefixLength == 0 && flushAll) {
            prefixLength = pending_.size();
        }
        if (prefixLength == 0) {
            return true;
        }

        const std::string utf8 = pending_.substr(0, prefixLength);
        pending_.erase(0, prefixLength);
        return writeConsoleUtf8(utf8);
    }

    bool writeConsoleUtf8(const std::string& utf8) {
        if (utf8.empty()) {
            return true;
        }

        const int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                                   static_cast<int>(utf8.size()), nullptr, 0);
        if (wideLength <= 0) {
            return fallback_->sputn(utf8.data(), static_cast<std::streamsize>(utf8.size())) ==
                   static_cast<std::streamsize>(utf8.size());
        }

        std::wstring wide(static_cast<std::size_t>(wideLength), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                            wide.data(), wideLength);

        DWORD written = 0;
        return WriteConsoleW(output_, wide.data(), static_cast<DWORD>(wide.size()), &written,
                             nullptr) != 0;
    }

    static std::size_t completeUtf8PrefixLength(const std::string& text) {
        std::size_t index = 0;
        while (index < text.size()) {
            const unsigned char ch = static_cast<unsigned char>(text[index]);
            std::size_t length = 1;
            if ((ch & 0x80) == 0) {
                length = 1;
            } else if ((ch & 0xE0) == 0xC0) {
                length = 2;
            } else if ((ch & 0xF0) == 0xE0) {
                length = 3;
            } else if ((ch & 0xF8) == 0xF0) {
                length = 4;
            }

            if (index + length > text.size()) {
                break;
            }

            bool validContinuation = true;
            for (std::size_t offset = 1; offset < length; ++offset) {
                const unsigned char next = static_cast<unsigned char>(text[index + offset]);
                if ((next & 0xC0) != 0x80) {
                    validContinuation = false;
                    break;
                }
            }
            index += validContinuation ? length : 1;
        }
        return index;
    }
};

bool stdoutIsConsole(HANDLE output) {
    DWORD mode = 0;
    return output != INVALID_HANDLE_VALUE && output != nullptr && GetConsoleMode(output, &mode) != 0;
}
}  // namespace
#endif

void Encoding::setupConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (std::setlocale(LC_ALL, ".UTF-8") == nullptr) {
        std::setlocale(LC_ALL, "");
    }

    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    static Utf8ConsoleStreamBuf consoleBuffer(std::cout.rdbuf(), output);
    static bool installed = false;
    if (!installed && stdoutIsConsole(output)) {
        std::cout.rdbuf(&consoleBuffer);
        installed = true;
    }
#else
    std::setlocale(LC_ALL, "");
#endif

    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
}
