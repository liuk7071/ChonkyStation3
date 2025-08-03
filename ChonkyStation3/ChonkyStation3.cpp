#include <iostream>

#include <Frontend/GameWindow.hpp>
#ifdef CHONKYSTATION3_QT_BUILD
#include <Frontend/MainWindow.hpp>
#else
#include "PlayStation3.hpp" // Don't need this here in Qt builds because it's included in MainWindow.hpp
#endif


#ifdef _WIN32
#include <windows.h>
// Gently ask to use the discrete Nvidia/AMD GPU if possible instead of
// integrated graphics
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif


int main(int argc, char** argv) {
    printf("ChonkyStation3\n\n");

#ifdef CHONKYSTATION3_QT_BUILD
    QApplication::setStyle("fusion");
    QCoreApplication::addLibraryPath("./Qt6");
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Resources/icon-small.ico"));
    MainWindow main_window = MainWindow();
    int ret = app.exec();
    main_window.onExit();
    app.exit();
    return ret;
#else

    fs::path file = "";
    if (argc >= 2)
        file = argv[1];

    PlayStation3* ps3 = new PlayStation3(file);
    if (argc == 1)
        ps3->gameSelector();    // Show a game selector dialog to choose one of the installed games if no direct elf path is passed via CLI
    ps3->init();

    GameWindow game_window = GameWindow();
    game_window.run(ps3);
    return 0;
#endif
}
