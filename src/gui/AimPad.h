#ifndef AIMPAD_H
#define AIMPAD_H

#include <QList>
#include <QPointF>
#include <QWidget>
#include "CameraController.h"

/**
 * @brief Map of the gimbal's whole range with the camera's real position.
 *
 * Stands in for the preview when another app has the camera. Click or drag
 * to aim; the dot shows where the motors actually are (read back from the
 * gimbal, not the commanded value), and stored presets are marked.
 */
class AimPad : public QWidget
{
    Q_OBJECT

public:
    explicit AimPad(QWidget *parent = nullptr);

    void setActual(const CameraController::GimbalAngles &angles);
    void setPresets(const QList<CameraController::CameraPreset> &presets);
    void setMessage(const QString &message);

    QSize sizeHint() const override { return QSize(480, 240); }
    QSize minimumSizeHint() const override { return QSize(240, 140); }

signals:
    void aimRequested(double yawDeg, double pitchDeg);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QRectF fieldRect() const;
    QPointF toPoint(double yawDeg, double pitchDeg) const;
    QPointF toAngles(const QPointF &point) const;  // x = yaw, y = pitch
    void requestAim(const QPointF &angles);

    CameraController::GimbalAngles m_actual;
    QList<CameraController::CameraPreset> m_presets;
    QString m_message;
    QPointF m_target;
    bool m_hasTarget = false;
    bool m_dragging = false;
    qint64 m_lastSentMs = 0;
};

#endif // AIMPAD_H
