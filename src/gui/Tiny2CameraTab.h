#ifndef TINY2CAMERATAB_H
#define TINY2CAMERATAB_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include "Tiny2Panel.h"

class ModeBar;

/**
 * @brief Zoom, view, position, presets, start-up position and tracking zone.
 */
class Tiny2CameraTab : public Tiny2Panel
{
    Q_OBJECT

public:
    explicit Tiny2CameraTab(CameraController *controller, ModeBar *modeBar, QWidget *parent = nullptr);

    void updateFromState(const CameraController::CameraState &state) override;
    void setGimbalAngles(const CameraController::GimbalAngles &angles);
    void refreshFromCamera();  // presets, start-up position, zone limits

    const QList<CameraController::CameraPreset> &presets() const { return m_presets; }

public slots:
    void recallPreset(int slot);
    void stepZoom(double delta);
    void setZoom(double zoom);

signals:
    void presetsChanged(const QList<CameraController::CameraPreset> &presets);
    void gimbalMoved();  // ask for a fresh position read once the move settles

private slots:
    void onZoomSliderChanged(int value);
    void onViewClicked(int fov);
    void onPresetMenu(int slot);
    void onSetStartup();
    void onResetStartup();
    void onApplyZone();
    void onZoneEnabledToggled(bool enabled);

private:
    QWidget *buildViewSection();
    QWidget *buildPresetSection();
    QWidget *buildStartupSection();
    QWidget *buildZoneSection();
    void rebuildPresetButtons();
    const CameraController::CameraPreset *presetInSlot(int slot) const;
    void sendZoom();

    ModeBar *m_modeBar;

    QSlider *m_zoomSlider;
    QLabel *m_zoomValue;
    QTimer *m_zoomSendTimer;
    QButtonGroup *m_viewGroup;
    QLabel *m_positionLabel;

    QList<CameraController::CameraPreset> m_presets;
    QPushButton *m_presetButtons[CameraController::kMaxPresets];
    QPushButton *m_presetMenuButtons[CameraController::kMaxPresets];

    QLabel *m_startupLabel;

    QCheckBox *m_zoneEnabled;
    QCheckBox *m_zoneAutoSelect;
    QDoubleSpinBox *m_zoneYawMin;
    QDoubleSpinBox *m_zoneYawMax;
    QDoubleSpinBox *m_zonePitchMin;
    QDoubleSpinBox *m_zonePitchMax;
};

#endif // TINY2CAMERATAB_H
