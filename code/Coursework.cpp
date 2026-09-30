#include "App.h"
#include <iostream>
#include <exception>

int main() {
    try {
        App app;
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "FATAL ERROR: (UNKNOWN)\n";
        return -1;
    }

    return 0;
}