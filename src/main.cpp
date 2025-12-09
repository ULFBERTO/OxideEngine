#include "core/application.h"

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Application app;
    if (app.Init()) {
        app.Run();
    }
    return 0;
}
#else
int main() {
    Application app;
    if (app.Init()) {
        app.Run();
    }
    return 0;
}
#endif