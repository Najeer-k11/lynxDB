#include "app/App.h"
#include <iostream>
#include <exception>

int main() {
    try {
        dbterm::App app;
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
