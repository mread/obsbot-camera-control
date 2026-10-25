#include <QApplication>
#include <QCommandLineParser>
#include <QTabWidget>
#include <QTimer>
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

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption screenshotOption("screenshot",
        "Save a PNG of the window once the camera has connected, then quit.", "file");
    QCommandLineOption tabOption("tab", "Tab to show for --screenshot (0 = first).", "index", "0");
    parser.addOption(screenshotOption);
    parser.addOption(tabOption);
    parser.process(app);

    MainWindow window;
    if (!window.isStartingMinimized() || parser.isSet(screenshotOption)) {
        window.show();
    }

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        const int tab = parser.value(tabOption).toInt();
        // SDK discovery takes ~3.5 s; give the UI time to fill in.
        QTimer::singleShot(9000, &window, [&window, path, tab]() {
            if (auto *tabs = window.findChild<QTabWidget *>("controlTabs"))
                tabs->setCurrentIndex(tab);
            QTimer::singleShot(1500, &window, [&window, path]() {
                window.grab().save(path);
                QApplication::quit();
            });
        });
    }

    return app.exec();
}
