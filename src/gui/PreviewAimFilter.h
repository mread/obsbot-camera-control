#ifndef PREVIEWAIMFILTER_H
#define PREVIEWAIMFILTER_H

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <functional>

class CameraController;
class QWidget;

/**
 * @brief Aim the camera from the live preview.
 *
 * Drag works like a joystick: the further the pointer moves from where it
 * was pressed, the faster the gimbal turns, and releasing stops it. The
 * wheel zooms. Double-click turns the camera to centre the clicked point.
 */
class PreviewAimFilter : public QObject
{
    Q_OBJECT

public:
    PreviewAimFilter(CameraController *controller, QWidget *target, QObject *parent = nullptr);

    // Applies a zoom step; wired to the Camera tab's zoom slider.
    std::function<void(double)> zoomBy;
    // Called before any gimbal move, so tracking can be switched off first.
    std::function<void()> beforeManualAim;

signals:
    void gimbalMoved();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void sendSpeed(const QPointF &pos, bool force);
    void centreOn(const QPointF &pos);

    CameraController *m_controller;
    QWidget *m_target;
    QPointF m_pressPos;
    bool m_dragging = false;
    QElapsedTimer m_lastSend;
};

#endif // PREVIEWAIMFILTER_H
