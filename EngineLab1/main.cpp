#include "Application.h"
#include "Logger.h"

int main() {
    Application app;

    if (!app.Init()) {
        Logger::Error("Application failed to initialize");
        return -1;
    }

    app.Run();
    return 0;
}