#include "ControlService.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QTextStream>

ControlService::ControlService(const Actions &actions, QObject *parent)
    : QObject(parent)
    , m_actions(actions)
{
}

bool ControlService::registerOnBus()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return false;
    if (!bus.registerService(kServiceName)) {
        qWarning() << "ControlService: another instance already owns" << kServiceName;
        return false;
    }
    return bus.registerObject(kObjectPath, this, QDBusConnection::ExportScriptableSlots);
}

void ControlService::RecallPreset(int slot) { if (m_actions.recallPreset) m_actions.recallPreset(slot); }
bool ControlService::SetMode(const QString &mode) { return m_actions.setMode && m_actions.setMode(mode); }
void ControlService::ToggleTracking() { if (m_actions.toggleTracking) m_actions.toggleTracking(); }
void ControlService::ZoomBy(double delta) { if (m_actions.zoomBy) m_actions.zoomBy(delta); }
void ControlService::ZoomTo(double zoom) { if (m_actions.zoomTo) m_actions.zoomTo(zoom); }
void ControlService::Recentre() { if (m_actions.recentre) m_actions.recentre(); }
void ControlService::Sleep() { if (m_actions.setAwake) m_actions.setAwake(false); }
void ControlService::Wake() { if (m_actions.setAwake) m_actions.setAwake(true); }
void ControlService::ToggleSleep() { if (m_actions.toggleSleep) m_actions.toggleSleep(); }
void ControlService::Show() { if (m_actions.showWindow) m_actions.showWindow(); }

QString ControlService::usage()
{
    return QStringLiteral(
        "Actions (sent to the running obsbot-gui):\n"
        "  preset <1-3>        recall a preset stored on the camera\n"
        "  mode <name>         off, human, group, hand, desk or whiteboard\n"
        "  track               toggle human tracking\n"
        "  zoom <factor>       zoom to a factor, e.g. 2.5\n"
        "  zoom-in | zoom-out  zoom by 0.25\n"
        "  centre              point the camera straight ahead\n"
        "  sleep | wake | toggle-sleep\n"
        "  show                bring the window forward\n");
}

int ControlService::sendAction(const QStringList &action)
{
    QTextStream err(stderr);
    if (action.isEmpty()) {
        err << usage();
        return 1;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected() || !bus.interface()->isServiceRegistered(kServiceName)) {
        err << "obsbot-gui is not running\n";
        return 2;
    }
    QDBusInterface control(kServiceName, kObjectPath, kServiceName, bus);

    const QString name = action.first();
    const QString arg = action.value(1);
    bool ok = true;
    QDBusMessage reply;
    if (name == "preset") {
        const int slot = arg.toInt(&ok);
        if (!ok || slot < 1 || slot > 3) { err << "preset needs 1, 2 or 3\n"; return 1; }
        reply = control.call("RecallPreset", slot);
    } else if (name == "mode") {
        reply = control.call("SetMode", arg);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().value(0).toBool()) {
            err << "unknown mode: " << arg << "\n";
            return 1;
        }
    } else if (name == "track") {
        reply = control.call("ToggleTracking");
    } else if (name == "zoom") {
        const double zoom = arg.toDouble(&ok);
        if (!ok) { err << "zoom needs a factor, e.g. 2.5\n"; return 1; }
        reply = control.call("ZoomTo", zoom);
    } else if (name == "zoom-in") {
        reply = control.call("ZoomBy", 0.25);
    } else if (name == "zoom-out") {
        reply = control.call("ZoomBy", -0.25);
    } else if (name == "centre" || name == "center") {
        reply = control.call("Recentre");
    } else if (name == "sleep") {
        reply = control.call("Sleep");
    } else if (name == "wake") {
        reply = control.call("Wake");
    } else if (name == "toggle-sleep") {
        reply = control.call("ToggleSleep");
    } else if (name == "show") {
        reply = control.call("Show");
    } else {
        err << "unknown action: " << name << "\n" << usage();
        return 1;
    }

    if (reply.type() == QDBusMessage::ErrorMessage) {
        err << reply.errorMessage() << "\n";
        return 2;
    }
    return 0;
}
