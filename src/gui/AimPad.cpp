#include "AimPad.h"
#include <QDateTime>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {

// OBSBOT Center's limits for the Tiny 2 (OSC SetGimMotorDegree).
constexpr double kYawLimitDeg = 129.0;
constexpr double kPitchLimitDeg = 59.0;
constexpr qint64 kDragSendIntervalMs = 150;
constexpr double kKeyStepDeg = 2.0;

} // namespace

AimPad::AimPad(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    setToolTip(tr("Click or drag to aim the camera. Arrow keys nudge it by 2°."));
}

void AimPad::setActual(const CameraController::GimbalAngles &angles)
{
    m_actual = angles;
    // Once the gimbal has arrived, drop the target marker.
    if (m_hasTarget && !m_dragging && angles.valid
        && qAbs(angles.yawDeg - m_target.x()) < 1.0 && qAbs(angles.pitchDeg - m_target.y()) < 1.0) {
        m_hasTarget = false;
    }
    update();
}

void AimPad::setPresets(const QList<CameraController::CameraPreset> &presets)
{
    m_presets = presets;
    update();
}

void AimPad::setMessage(const QString &message)
{
    m_message = message;
    update();
}

QRectF AimPad::fieldRect() const
{
    const QRectF area = QRectF(rect()).adjusted(12, 12, -12, -12);
    // Keep the field at the gimbal's aspect ratio (yaw range : pitch range).
    const double aspect = kYawLimitDeg / kPitchLimitDeg;
    double w = area.width();
    double h = w / aspect;
    if (h > area.height()) {
        h = area.height();
        w = h * aspect;
    }
    return QRectF(area.center().x() - w / 2, area.center().y() - h / 2, w, h);
}

QPointF AimPad::toPoint(double yawDeg, double pitchDeg) const
{
    // Positive yaw turns the view left, positive pitch looks down.
    const QRectF f = fieldRect();
    const double x = f.center().x() - yawDeg / kYawLimitDeg * f.width() / 2;
    const double y = f.center().y() + pitchDeg / kPitchLimitDeg * f.height() / 2;
    return QPointF(x, y);
}

QPointF AimPad::toAngles(const QPointF &point) const
{
    const QRectF f = fieldRect();
    const double yaw = -(point.x() - f.center().x()) / (f.width() / 2) * kYawLimitDeg;
    const double pitch = (point.y() - f.center().y()) / (f.height() / 2) * kPitchLimitDeg;
    return QPointF(qBound(-kYawLimitDeg, yaw, kYawLimitDeg), qBound(-kPitchLimitDeg, pitch, kPitchLimitDeg));
}

void AimPad::requestAim(const QPointF &angles)
{
    m_target = angles;
    m_hasTarget = true;
    m_lastSentMs = QDateTime::currentMSecsSinceEpoch();
    emit aimRequested(angles.x(), angles.y());
    update();
}

void AimPad::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette pal = palette();
    const QRectF f = fieldRect();

    p.fillRect(rect(), pal.color(QPalette::Base));
    p.setPen(QPen(pal.color(QPalette::Mid), 1));
    p.drawRoundedRect(f, 6, 6);

    // Centre lines and 30-degree grid
    QPen grid(pal.color(QPalette::Mid), 1, Qt::DotLine);
    p.setPen(grid);
    for (int yaw = -120; yaw <= 120; yaw += 30) {
        const QPointF top = toPoint(yaw, -kPitchLimitDeg);
        p.drawLine(QPointF(top.x(), f.top()), QPointF(top.x(), f.bottom()));
    }
    for (int pitch = -30; pitch <= 30; pitch += 30) {
        const QPointF left = toPoint(kYawLimitDeg, pitch);
        p.drawLine(QPointF(f.left(), left.y()), QPointF(f.right(), left.y()));
    }

    QFont labelFont = font();
    p.setFont(labelFont);
    const QFontMetrics fm(labelFont);

    // Presets
    for (const auto &preset : m_presets) {
        const QPointF pt = toPoint(preset.yawDeg, preset.pitchDeg);
        p.setPen(QPen(pal.color(QPalette::Text), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(pt.x() - 5, pt.y() - 5, 10, 10));
        const QString label = preset.name.isEmpty() ? QString::number(preset.id + 1) : preset.name;
        p.drawText(QPointF(pt.x() + 9, pt.y() + fm.ascent() / 2.0 - 1), label);
    }

    // Target being sent
    if (m_hasTarget) {
        const QPointF pt = toPoint(m_target.x(), m_target.y());
        p.setPen(QPen(pal.color(QPalette::Highlight), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pt, 11, 11);
    }

    // Actual position
    if (m_actual.valid) {
        const QPointF pt = toPoint(m_actual.yawDeg, m_actual.pitchDeg);
        p.setPen(QPen(pal.color(QPalette::Base), 2));
        p.setBrush(pal.color(QPalette::Highlight));
        p.drawEllipse(pt, 8, 8);
    }

    if (!m_message.isEmpty()) {
        p.setPen(pal.color(QPalette::Text));
        const QRectF textRect = f.adjusted(10, 8, -10, -8);
        p.drawText(textRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, m_message);
    }
}

void AimPad::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_dragging = true;
    requestAim(toAngles(event->position()));
}

void AimPad::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    m_target = toAngles(event->position());
    m_hasTarget = true;
    if (QDateTime::currentMSecsSinceEpoch() - m_lastSentMs >= kDragSendIntervalMs)
        requestAim(m_target);
    update();
}

void AimPad::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_dragging)
        return;
    m_dragging = false;
    requestAim(toAngles(event->position()));
}

void AimPad::keyPressEvent(QKeyEvent *event)
{
    QPointF from = m_hasTarget ? m_target
                 : m_actual.valid ? QPointF(m_actual.yawDeg, m_actual.pitchDeg)
                 : QPointF(0, 0);
    switch (event->key()) {
    case Qt::Key_Left:  from.rx() += kKeyStepDeg; break;
    case Qt::Key_Right: from.rx() -= kKeyStepDeg; break;
    case Qt::Key_Up:    from.ry() -= kKeyStepDeg; break;
    case Qt::Key_Down:  from.ry() += kKeyStepDeg; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    from.setX(qBound(-kYawLimitDeg, from.x(), kYawLimitDeg));
    from.setY(qBound(-kPitchLimitDeg, from.y(), kPitchLimitDeg));
    requestAim(from);
}
