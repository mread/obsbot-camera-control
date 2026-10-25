#include "CameraHolders.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>

namespace {

QString processName(qint64 pid, const QString &fallback)
{
    const QFileInfo exe(QStringLiteral("/proc/%1/exe").arg(pid));
    const QString target = exe.symLinkTarget();
    return target.isEmpty() ? fallback : QFileInfo(target).fileName();
}

qint64 parentPid(qint64 pid)
{
    QFile stat(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!stat.open(QIODevice::ReadOnly))
        return 0;
    // pid (comm) state ppid ...; comm may contain spaces, so split after ')'
    const QString text = QString::fromUtf8(stat.readAll());
    return text.mid(text.lastIndexOf(QLatin1Char(')')) + 2).section(QLatin1Char(' '), 1, 1).toLongLong();
}

QString userUnitOf(qint64 pid, const QString &name)
{
    // cgroup v2: 0::/user.slice/user-1000.slice/user@1000.service/app.slice/go2rtc.service
    QFile cgroup(QStringLiteral("/proc/%1/cgroup").arg(pid));
    if (!cgroup.open(QIODevice::ReadOnly))
        return QString();
    const QString path = QString::fromUtf8(cgroup.readAll()).trimmed().section(QLatin1Char(':'), 2);
    if (!path.contains(QLatin1String("/user@")))
        return QString();
    const QString leaf = path.section(QLatin1Char('/'), -1);
    if (!leaf.endsWith(QLatin1String(".service")))
        return QString();

    // Only name the unit when it is the process's own service: its name
    // matches the process or its parent (go2rtc.service -> go2rtc -> ffmpeg).
    // A browser started from a terminal multiplexer also lands in that
    // tool's unit, which would be misleading to report.
    const QString stem = leaf.chopped(int(sizeof(".service")) - 1);
    const QString parent = processName(parentPid(pid), QString());
    return (stem == name || stem == parent) ? leaf : QString();
}

} // namespace

QList<CameraHolder> cameraHolders(const QString &devicePath)
{
    QProcess lsof;
    lsof.start(QStringLiteral("lsof"), {QStringLiteral("-w"), devicePath});
    lsof.waitForFinished(2000);
    const QStringList lines = QString::fromUtf8(lsof.readAllStandardOutput()).split(QLatin1Char('\n'));

    // COMMAND PID USER FD TYPE DEVICE SIZE/OFF NODE NAME; FD "mem" = mapped buffers
    const qint64 ourPid = QCoreApplication::applicationPid();
    QMap<qint64, CameraHolder> byPid;
    for (int i = 1; i < lines.size(); ++i) {
        const QStringList parts = lines[i].trimmed().split(QRegularExpression(QStringLiteral("\\s+")));
        if (parts.size() < 4)
            continue;
        const qint64 pid = parts[1].toLongLong();
        if (pid <= 0 || pid == ourPid)
            continue;
        CameraHolder &holder = byPid[pid];
        if (holder.pid == 0) {
            holder.pid = pid;
            holder.name = processName(pid, parts[0]);
            holder.userUnit = userUnitOf(pid, holder.name);
        }
        if (parts[3] == QLatin1String("mem"))
            holder.streaming = true;
    }
    return byPid.values();
}

QList<CameraHolder> streamingHolders(const QString &devicePath)
{
    QList<CameraHolder> streaming;
    for (const auto &holder : cameraHolders(devicePath)) {
        if (holder.streaming)
            streaming.append(holder);
    }
    return streaming;
}

QString describeHolder(const CameraHolder &holder)
{
    QString text = QStringLiteral("%1 (PID %2)").arg(holder.name).arg(holder.pid);
    if (!holder.userUnit.isEmpty())
        text += QCoreApplication::translate("CameraHolders", " in %1").arg(holder.userUnit);
    return text;
}
