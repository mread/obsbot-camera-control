#ifndef CAMERAHOLDERS_H
#define CAMERAHOLDERS_H

#include <QList>
#include <QString>

/**
 * @brief Processes that have a video device open, and which are streaming.
 *
 * Linux lets any number of processes open a camera; only one can stream.
 * A browser often keeps the device open after a call ends, and that does
 * not block anyone, so only processes with the device memory-mapped (the
 * buffers of an active stream) count as blocking the preview.
 */
struct CameraHolder {
    qint64 pid = 0;
    QString name;       // executable name, e.g. "zen" or "ffmpeg"
    bool streaming = false;
    QString userUnit;   // systemd --user unit the process runs in, if any
};

QList<CameraHolder> cameraHolders(const QString &devicePath);
QList<CameraHolder> streamingHolders(const QString &devicePath);

// Human-readable description, e.g. "zen (PID 4143665)" or
// "ffmpeg (PID 1395500) in go2rtc.service".
QString describeHolder(const CameraHolder &holder);

#endif // CAMERAHOLDERS_H
