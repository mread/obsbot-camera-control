#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <cstring>
#include "ControlService.h"
#include "MainWindow.h"

namespace {

// `obsbot-gui --action preset 1` talks to the running instance over D-Bus and
// exits, without opening a window or touching the camera.
int runAction(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(ControlService::usage());
    parser.addHelpOption();
    QCommandLineOption actionOption("action", "Send an action to the running instance.", "name");
    parser.addOption(actionOption);
    parser.addPositionalArgument("argument", "Argument for the action, if it takes one.");
    parser.process(app);

    QStringList action{parser.value(actionOption)};
    action += parser.positionalArguments();
    return ControlService::sendAction(action);
}

} // namespace

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--action") == 0 || std::strncmp(argv[i], "--action=", 9) == 0)
            return runAction(argc, argv);
    }

    QApplication app(argc, argv);

    // Body text at 16 px or more, whatever the desktop default.
    QFont font = app.font();
    if (font.pixelSize() > 0 ? font.pixelSize() < 16 : font.pointSizeF() * app.devicePixelRatio() * 96.0 / 72.0 < 16.0) {
        font.setPixelSize(16);
        app.setFont(font);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Control OBSBOT cameras.\n\nWith --action <name> [argument], send an action to the running "
        "instance instead of opening a window. Bind these to keys in your desktop, e.g. in sway:\n"
        "  bindsym $mod+Ctrl+1 exec obsbot-gui --action preset 1\n\n") + ControlService::usage());
    parser.addHelpOption();
    QCommandLineOption screenshotOption("screenshot",
        "Save a PNG of the window once the camera has connected, then quit.", "file");
    QCommandLineOption tabOption("tab", "Tab to show for --screenshot (0 = first).", "index", "0");
    QCommandLineOption previewOption("start-preview", "With --screenshot, start the preview first.");
    parser.addOption(screenshotOption);
    parser.addOption(tabOption);
    parser.addOption(previewOption);
    parser.process(app);

    MainWindow window;
    if (!window.isStartingMinimized() || parser.isSet(screenshotOption)) {
        window.show();
    }

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        const int tab = parser.value(tabOption).toInt();
        const bool startPreview = parser.isSet(previewOption);
        // SDK discovery takes ~3.5 s; give the UI time to fill in.
        QTimer::singleShot(9000, &window, [&window, path, tab, startPreview]() {
            if (auto *tabs = window.findChild<QTabWidget *>("controlTabs"))
                tabs->setCurrentIndex(tab);
            if (startPreview) {
                if (auto *button = window.findChild<QPushButton *>("primaryAction"))
                    button->setChecked(true);
            }
            QTimer::singleShot(1500, &window, [&window, path]() {
                window.grab().save(path);
                QApplication::quit();
            });
        });
    }

    return app.exec();
}
