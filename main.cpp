#include <iostream>
#include <windows.h>
#ifdef _DEBUG
// The following line is necessary for the GetConsoleWindow() function to work!
// It basically says that you are running this program on Windows 2000 or higher
//#define _WIN32_WINNT 0x0500
#endif

#include "MediaArch.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "en_US.UTF-8");
#ifdef _DEBUG
    // Show Console Window
    HWND console = GetConsoleWindow();
    MoveWindow(console, 0, 0, 720, 1420, TRUE);
#endif
    QApplication a(argc, argv);
    MediaArch w;
    w.show();
    return a.exec();
}
