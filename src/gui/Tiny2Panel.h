#ifndef TINY2PANEL_H
#define TINY2PANEL_H

#include <QElapsedTimer>
#include <QHash>
#include <QWidget>
#include "CameraController.h"

/**
 * @brief Base for the Tiny 2 panels.
 *
 * The camera's status block lags writes by 1.5-2.5 s, so a control the user
 * just changed would be snapped back by the next poll. Each panel marks a
 * control when the user changes it and leaves it alone until the camera has
 * had time to report the new value.
 */
class Tiny2Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Tiny2Panel(CameraController *controller, QWidget *parent = nullptr)
        : QWidget(parent), m_controller(controller) {}

    virtual void updateFromState(const CameraController::CameraState &state) = 0;

protected:
    static constexpr qint64 kStatusLagMs = 3000;

    void markUserChange(const QObject *control)
    {
        m_changed[control].start();
    }

    bool isHeld(const QObject *control) const
    {
        auto it = m_changed.constFind(control);
        return it != m_changed.constEnd() && it->isValid() && it->elapsed() < kStatusLagMs;
    }

    CameraController *m_controller;

private:
    QHash<const QObject *, QElapsedTimer> m_changed;
};

#endif // TINY2PANEL_H
