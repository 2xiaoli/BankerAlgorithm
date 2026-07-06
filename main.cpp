#include "ConsoleUI.h"
#include "Encoding.h"

int main() {
    Encoding::setupConsoleEncoding();

    ConsoleUI ui;
    ui.run();
    return 0;
}
