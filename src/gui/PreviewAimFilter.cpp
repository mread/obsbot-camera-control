#include "PreviewAimFilter.h"
#include "CameraController.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>
#include <QtMath>

namespace {

constexpr double kMaxSpeedDegPerSec = 60.0;
constexpr qint64 kSendIntervalMs = 100;
constexpr double kDeadZonePx = 6.0;
constexpr double kWheelZoomStep = 0.1;

// Horizontal field of view at 1x, measured on a Tiny 2 by turning the gimbal
// a known angle and tracking the image shift. The 86/78/65 degrees OBSBOT
// quotes are diagonal. The FOV setting doesn't change 1080p or 1440p frames
// (only 4K ones, which this doesn't allow for), and the 4:3 modes add height,
// not width, so one figure covers them.
constexpr double kHorizontalFovDeg = 68.6;

} // namespace

PreviewAimFilter::PreviewAimFilter(CameraController *controller, QWidget *target, QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_target(target)
{
    target->installEventFilter(this);
    target->setToolTip(tr("Drag to turn the camera, scroll to zoom, double-click to centre on a point"));
}

bool PreviewAimFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_target || !m_controller->isTiny2())
        return QObject::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton)
            break;
        m_pressPos = mouse->position();
        m_dragging = true;
        if (beforeManualAim)
            beforeManualAim();
        m_target->setCursor(Qt::SizeAllCursor);
        return true;
    }
    case QEvent::MouseMove: {
        if (!m_dragging)
            break;
        auto *mouse = static_cast<QMouseEvent *>(event);
        // The release went somewhere else (a popup, another window): stop
        // rather than keep turning towards wherever the pointer went.
        if (!(mouse->buttons() & Qt::LeftButton)) {
            endDrag();
            return true;
        }
        sendSpeed(mouse->position(), false);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton || !m_dragging)
            break;
        endDrag();
        return true;
    }
    case QEvent::MouseButtonDblClick: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            // The second click's press may have started a drag.
            endDrag();
            if (beforeManualAim)
                beforeManualAim();
            centreOn(mouse->position());
            return true;
        }
        break;
    }
    case QEvent::Hide:
    case QEvent::FocusOut:
        endDrag();
        break;
    case QEvent::Wheel: {
        auto *wheel = static_cast<QWheelEvent *>(event);
        const double notches = wheel->angleDelta().y() / 120.0;
        if (zoomBy && notches != 0.0)
            zoomBy(notches * kWheelZoomStep);
        return true;
    }
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void PreviewAimFilter::endDrag()
{
    if (!m_dragging)
        return;
    m_dragging = false;
    m_target->unsetCursor();
    m_controller->stopGimbal();
    emit gimbalMoved();
}

void PreviewAimFilter::sendSpeed(const QPointF &pos, bool force)
{
    if (!force && m_lastSend.isValid() && m_lastSend.elapsed() < kSendIntervalMs)
        return;
    m_lastSend.start();

    const QPointF offset = pos - m_pressPos;
    const double halfW = qMax(1.0, m_target->width() / 2.0);
    const double halfH = qMax(1.0, m_target->height() / 2.0);
    auto scale = [](double px, double half) {
        if (qAbs(px) < kDeadZonePx)
            return 0.0;
        return qBound(-1.0, px / half, 1.0) * kMaxSpeedDegPerSec;
    };
    // Joystick sense: pointer right turns the view right (negative yaw),
    // pointer down tilts the view down (positive pitch).
    m_controller->setGimbalSpeed(-scale(offset.x(), halfW), scale(offset.y(), halfH));
}

void PreviewAimFilter::centreOn(const QPointF &pos)
{
    const QRectF picture = pictureRect ? pictureRect() : QRectF(m_target->rect());
    if (picture.isEmpty() || !picture.contains(pos))
        return;

    const auto angles = m_controller->readGimbalAngles();
    if (!angles.valid)
        return;

    const double zoom = qMax(1.0, m_controller->getCurrentState().zoom);
    // Field of view narrows with zoom as atan(tan(fov/2) / zoom).
    const double halfH = qAtan(qTan(qDegreesToRadians(kHorizontalFovDeg / 2.0)) / zoom);
    const double halfV = qAtan(qTan(halfH) * picture.height() / picture.width());

    const double fx = (pos.x() - picture.left()) / picture.width() * 2.0 - 1.0;  // -1 left .. 1 right
    const double fy = (pos.y() - picture.top()) / picture.height() * 2.0 - 1.0;  // -1 top .. 1 bottom
    const double yawOffset = qRadiansToDegrees(qAtan(fx * qTan(halfH)));
    const double pitchOffset = qRadiansToDegrees(qAtan(fy * qTan(halfV)));

    m_controller->setGimbalAngles(angles.yawDeg - yawOffset, angles.pitchDeg + pitchOffset);
    emit gimbalMoved();
}
