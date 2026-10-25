#ifndef MODEBAR_H
#define MODEBAR_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include "Tiny2Panel.h"

/**
 * @brief AI work mode, framing and tracking mode, shown under the preview.
 *
 * Mirrors OBSBOT Center's Console: human tracking with five framings, plus
 * group, hand, desk and whiteboard modes, and Standard/Motion tracking.
 */
class ModeBar : public Tiny2Panel
{
    Q_OBJECT

public:
    explicit ModeBar(CameraController *controller, QWidget *parent = nullptr);

    void updateFromState(const CameraController::CameraState &state) override;

    int currentMode() const;
    int currentFraming() const;

public slots:
    void selectMode(int mode);  // Device::AiWorkModeType

private slots:
    void onModeClicked(int mode);
    void onFramingChanged(int index);
    void onTrackingModeClicked(int mode);
    void onAutoZoomToggled(bool checked);

private:
    void applyMode(int mode);

    QButtonGroup *m_modeGroup;
    QLabel *m_framingLabel;
    QComboBox *m_framingCombo;
    QButtonGroup *m_trackingGroup;
    QCheckBox *m_autoZoomCheckBox;
};

#endif // MODEBAR_H
