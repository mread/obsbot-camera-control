#ifndef CONTROLSERVICE_H
#define CONTROLSERVICE_H

#include <QObject>
#include <QString>
#include <functional>

/**
 * @brief D-Bus interface for keyboard shortcuts and scripts.
 *
 * Wayland compositors such as sway have no API for an app to grab global
 * hotkeys, so the app exposes its actions on the session bus instead and
 * the compositor binds keys to `obsbot-gui --action ...`:
 *
 *   bindsym $mod+Ctrl+1 exec obsbot-gui --action preset 1
 */
class ControlService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.aaronsb.ObsbotControl")

public:
    static constexpr const char *kServiceName = "io.github.aaronsb.ObsbotControl";
    static constexpr const char *kObjectPath = "/Control";

    struct Actions {
        std::function<void(int)> recallPreset;          // slot 1-3
        std::function<bool(const QString &)> setMode;   // off, human, group, hand, desk, whiteboard
        std::function<void()> toggleTracking;
        std::function<void(double)> zoomBy;
        std::function<void(double)> zoomTo;
        std::function<void()> recentre;
        std::function<void(bool)> setAwake;
        std::function<void()> toggleSleep;
        std::function<void()> showWindow;
    };

    explicit ControlService(const Actions &actions, QObject *parent = nullptr);

    bool registerOnBus();

    // Runs one action against an already-running instance. Returns the
    // process exit code: 0 done, 1 bad arguments, 2 no running instance.
    static int sendAction(const QStringList &action);
    static QString usage();

public slots:
    Q_SCRIPTABLE void RecallPreset(int slot);
    Q_SCRIPTABLE bool SetMode(const QString &mode);
    Q_SCRIPTABLE void ToggleTracking();
    Q_SCRIPTABLE void ZoomBy(double delta);
    Q_SCRIPTABLE void ZoomTo(double zoom);
    Q_SCRIPTABLE void Recentre();
    Q_SCRIPTABLE void Sleep();
    Q_SCRIPTABLE void Wake();
    Q_SCRIPTABLE void ToggleSleep();
    Q_SCRIPTABLE void Show();

private:
    Actions m_actions;
};

#endif // CONTROLSERVICE_H
