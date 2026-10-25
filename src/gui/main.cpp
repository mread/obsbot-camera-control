#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Body text at 16 px or more, whatever the desktop default.
    QFont font = app.font();
    if (font.pixelSize() > 0 ? font.pixelSize() < 16 : font.pointSizeF() * app.devicePixelRatio() * 96.0 / 72.0 < 16.0) {
        font.setPixelSize(16);
        app.setFont(font);
    }

    MainWindow window;
    if (!window.isStartingMinimized()) {
        window.show();
    }

    return app.exec();
}
