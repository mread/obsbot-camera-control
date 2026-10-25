#ifndef TINY2DEVICETAB_H
#define TINY2DEVICETAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include "Tiny2Panel.h"

/**
 * @brief Sleep, gestures, voice control, audio and device information.
 */
class Tiny2DeviceTab : public Tiny2Panel
{
    Q_OBJECT

public:
    explicit Tiny2DeviceTab(CameraController *controller, QWidget *parent = nullptr);

    void updateFromState(const CameraController::CameraState &state) override;
    void refreshFromCamera();  // gesture settings (not in the status block)
    void setCameraInfo(const CameraController::CameraInfo &info);

private:
    QWidget *buildPowerSection();
    QWidget *buildGestureSection();
    QWidget *buildVoiceSection();
    QWidget *buildAudioSection();
    QWidget *buildAboutSection();

    QLabel *m_powerState;
    QPushButton *m_sleepWake;
    QComboBox *m_autoSleep;
    QCheckBox *m_micDuringSleep;
    bool m_asleep = false;

    QCheckBox *m_gestures[4];
    QLabel *m_gestureZoomNote;

    QComboBox *m_voiceLanguage;
    QCheckBox *m_voiceCommands[7];
    QSlider *m_voiceZoom;
    QLabel *m_voiceZoomValue;
    QTimer *m_voiceZoomTimer;

    QCheckBox *m_autoGain;

    QLabel *m_about;
    QLabel *m_ledLevel;
};

#endif // TINY2DEVICETAB_H
