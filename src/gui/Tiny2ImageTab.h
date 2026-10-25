#ifndef TINY2IMAGETAB_H
#define TINY2IMAGETAB_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <functional>
#include "Tiny2Panel.h"

/**
 * @brief Exposure, white balance, focus and picture controls.
 *
 * Auto/manual choices change which controls are shown, as Elgato Camera Hub
 * does: auto exposure offers face metering, manual offers the shutter.
 */
class Tiny2ImageTab : public Tiny2Panel
{
    Q_OBJECT

public:
    explicit Tiny2ImageTab(CameraController *controller, QWidget *parent = nullptr);

    void updateFromState(const CameraController::CameraState &state) override;

private:
    struct SliderRow {
        QSlider *slider = nullptr;
        QLabel *value = nullptr;
        QTimer *sendTimer = nullptr;
    };

    QWidget *buildExposureSection();
    QWidget *buildWhiteBalanceSection();
    QWidget *buildFocusSection();
    QWidget *buildPictureSection();
    SliderRow addSlider(QWidget *parent, class QFormLayout *form, const QString &label,
                        int min, int max, int step, std::function<void(int)> send,
                        const QString &suffix = QString());
    void syncSlider(SliderRow &row, int value, const QString &suffix = QString());
    void updateExposureVisibility();
    void updateWhiteBalanceVisibility();
    void updateFocusVisibility();

    QButtonGroup *m_exposureGroup;
    QCheckBox *m_faceAE;
    QLabel *m_shutterLabel;
    QComboBox *m_shutterCombo;
    QComboBox *m_antiFlickerCombo;
    QCheckBox *m_hdr;

    QButtonGroup *m_wbGroup;
    SliderRow m_kelvin;

    QButtonGroup *m_focusGroup;
    QCheckBox *m_faceFocus;
    SliderRow m_focus;

    SliderRow m_brightness;
    SliderRow m_contrast;
    SliderRow m_saturation;
    SliderRow m_sharpness;
    SliderRow m_hue;
    QCheckBox *m_mirror;
};

#endif // TINY2IMAGETAB_H
