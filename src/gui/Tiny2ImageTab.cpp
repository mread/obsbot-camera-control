#include "Tiny2ImageTab.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

constexpr int kSliderSendDelayMs = 150;
constexpr int kDefaultShutter = 33;  // 1/30 s, the camera's default

// Device::DevShutterTimeType values the Tiny 2 accepts (10-42 per
// cameraGetRangeExposureAbsolute), with their exposure times.
const struct { int value; const char *label; } kShutterTimes[] = {
    {10, "1/6400"}, {11, "1/5000"}, {12, "1/4000"}, {13, "1/3200"}, {14, "1/2500"},
    {15, "1/2000"}, {16, "1/1600"}, {17, "1/1250"}, {18, "1/1000"}, {19, "1/800"},
    {20, "1/640"},  {21, "1/500"},  {22, "1/400"},  {23, "1/320"},  {24, "1/240"},
    {25, "1/200"},  {26, "1/160"},  {27, "1/120"},  {28, "1/100"},  {29, "1/80"},
    {30, "1/60"},   {31, "1/50"},   {32, "1/40"},   {33, "1/30"},   {34, "1/25"},
    {35, "1/20"},   {36, "1/15"},   {37, "1/12.5"}, {38, "1/10"},   {39, "1/8"},
    {40, "1/6.25"}, {41, "1/5"},    {42, "1/4"},
};

enum { ModeAuto = 0, ModeManual = 1 };

} // namespace

Tiny2ImageTab::Tiny2ImageTab(CameraController *controller, QWidget *parent)
    : Tiny2Panel(controller, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(16);
    layout->addWidget(buildExposureSection());
    layout->addWidget(buildWhiteBalanceSection());
    layout->addWidget(buildFocusSection());
    layout->addWidget(buildPictureSection());
    layout->addStretch();
}

Tiny2ImageTab::SliderRow Tiny2ImageTab::addSlider(QWidget *parent, QFormLayout *form, const QString &label,
                                                  int min, int max, int step, std::function<void(int)> send,
                                                  const QString &suffix)
{
    SliderRow row;
    QHBoxLayout *line = new QHBoxLayout();
    row.slider = new QSlider(Qt::Horizontal, parent);
    row.slider->setRange(min, max);
    row.slider->setSingleStep(step);
    row.slider->setPageStep(step * 10);
    row.slider->setAccessibleName(label);
    row.value = new QLabel(parent);
    row.value->setMinimumWidth(row.value->fontMetrics().horizontalAdvance(QStringLiteral("10000 K")));
    row.value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    line->addWidget(row.slider, 1);
    line->addWidget(row.value);
    form->addRow(label, line);

    row.sendTimer = new QTimer(this);
    row.sendTimer->setSingleShot(true);
    row.sendTimer->setInterval(kSliderSendDelayMs);
    QSlider *slider = row.slider;
    QLabel *valueLabel = row.value;
    connect(row.sendTimer, &QTimer::timeout, this, [this, slider, send]() {
        markUserChange(slider);
        send(slider->value());
    });
    QTimer *timer = row.sendTimer;
    connect(row.slider, &QSlider::valueChanged, this, [this, slider, valueLabel, timer, suffix](int value) {
        markUserChange(slider);
        valueLabel->setText(QString::number(value) + suffix);
        timer->start();
    });
    return row;
}

void Tiny2ImageTab::syncSlider(SliderRow &row, int value, const QString &suffix)
{
    if (isHeld(row.slider) || row.slider->isSliderDown() || row.sendTimer->isActive())
        return;
    if (row.slider->value() != value) {
        QSignalBlocker blocker(row.slider);
        row.slider->setValue(value);
    }
    row.value->setText(QString::number(row.slider->value()) + suffix);
}

QWidget *Tiny2ImageTab::buildExposureSection()
{
    QGroupBox *group = new QGroupBox(tr("Exposure"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QHBoxLayout *modeRow = new QHBoxLayout();
    m_exposureGroup = new QButtonGroup(this);
    QPushButton *autoButton = new QPushButton(tr("Auto"), group);
    QPushButton *manualButton = new QPushButton(tr("Manual"), group);
    autoButton->setCheckable(true);
    manualButton->setCheckable(true);
    m_exposureGroup->addButton(autoButton, ModeAuto);
    m_exposureGroup->addButton(manualButton, ModeManual);
    modeRow->addWidget(autoButton);
    modeRow->addWidget(manualButton);
    modeRow->addStretch();
    layout->addLayout(modeRow);

    m_faceAE = new QCheckBox(tr("Expose for faces"), group);
    m_faceAE->setToolTip(tr("Meter on faces rather than the whole frame"));
    connect(m_faceAE, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_faceAE);
        m_controller->setFaceAE(checked);
    });
    layout->addWidget(m_faceAE);

    QHBoxLayout *shutterRow = new QHBoxLayout();
    m_shutterLabel = new QLabel(tr("Shutter"), group);
    shutterRow->addWidget(m_shutterLabel);
    m_shutterCombo = new QComboBox(group);
    for (const auto &s : kShutterTimes)
        m_shutterCombo->addItem(QStringLiteral("%1 s").arg(QLatin1String(s.label)), s.value);
    m_shutterCombo->setToolTip(tr("1/30 s or faster keeps 30 fps; slower shutters lower the frame rate"));
    shutterRow->addWidget(m_shutterCombo);
    shutterRow->addStretch();
    layout->addLayout(shutterRow);

    connect(m_exposureGroup, &QButtonGroup::idClicked, this, [this](int mode) {
        markUserChange(m_exposureGroup);
        const int shutter = m_shutterCombo->currentData().toInt();
        m_controller->setExposure(mode == ModeAuto, shutter > 0 ? shutter : kDefaultShutter);
        updateExposureVisibility();
    });
    connect(m_shutterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        markUserChange(m_shutterCombo);
        m_controller->setExposure(false, m_shutterCombo->currentData().toInt());
    });

    QFormLayout *form = new QFormLayout();
    m_antiFlickerCombo = new QComboBox(group);
    m_antiFlickerCombo->addItem(tr("Off"), Device::PowerLineFreqOff);
    m_antiFlickerCombo->addItem(tr("50 Hz (UK, Europe)"), Device::PowerLineFreq50);
    m_antiFlickerCombo->addItem(tr("60 Hz (North America)"), Device::PowerLineFreq60);
    connect(m_antiFlickerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        markUserChange(m_antiFlickerCombo);
        m_controller->setAntiFlicker(m_antiFlickerCombo->currentData().toInt());
    });
    form->addRow(tr("Anti-flicker"), m_antiFlickerCombo);
    layout->addLayout(form);

    m_hdr = new QCheckBox(tr("HDR"), group);
    m_hdr->setToolTip(tr("Wider dynamic range for bright windows behind you"));
    connect(m_hdr, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_hdr);
        m_controller->setHDR(checked);
    });
    layout->addWidget(m_hdr);

    updateExposureVisibility();
    return group;
}

QWidget *Tiny2ImageTab::buildWhiteBalanceSection()
{
    QGroupBox *group = new QGroupBox(tr("White balance"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QHBoxLayout *modeRow = new QHBoxLayout();
    m_wbGroup = new QButtonGroup(this);
    QPushButton *autoButton = new QPushButton(tr("Auto"), group);
    QPushButton *manualButton = new QPushButton(tr("Manual"), group);
    autoButton->setCheckable(true);
    manualButton->setCheckable(true);
    m_wbGroup->addButton(autoButton, ModeAuto);
    m_wbGroup->addButton(manualButton, ModeManual);
    modeRow->addWidget(autoButton);
    modeRow->addWidget(manualButton);
    modeRow->addStretch();
    layout->addLayout(modeRow);

    QWidget *kelvinBox = new QWidget(group);
    QFormLayout *form = new QFormLayout(kelvinBox);
    form->setContentsMargins(0, 0, 0, 0);
    m_kelvin = addSlider(kelvinBox, form, tr("Colour temperature"), 2000, 10000, 100,
                         [this](int kelvin) { m_controller->setWhiteBalanceManual(kelvin); },
                         QStringLiteral(" K"));
    layout->addWidget(kelvinBox);

    connect(m_wbGroup, &QButtonGroup::idClicked, this, [this](int mode) {
        markUserChange(m_wbGroup);
        if (mode == ModeAuto)
            m_controller->setWhiteBalance(Device::DevWhiteBalanceAuto);
        else
            m_controller->setWhiteBalanceManual(m_kelvin.slider->value());
        updateWhiteBalanceVisibility();
    });

    updateWhiteBalanceVisibility();
    return group;
}

QWidget *Tiny2ImageTab::buildFocusSection()
{
    QGroupBox *group = new QGroupBox(tr("Focus"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QHBoxLayout *modeRow = new QHBoxLayout();
    m_focusGroup = new QButtonGroup(this);
    QPushButton *autoButton = new QPushButton(tr("Auto"), group);
    QPushButton *manualButton = new QPushButton(tr("Manual"), group);
    autoButton->setCheckable(true);
    manualButton->setCheckable(true);
    m_focusGroup->addButton(autoButton, ModeAuto);
    m_focusGroup->addButton(manualButton, ModeManual);
    modeRow->addWidget(autoButton);
    modeRow->addWidget(manualButton);
    modeRow->addStretch();
    layout->addLayout(modeRow);

    m_faceFocus = new QCheckBox(tr("Focus on faces"), group);
    connect(m_faceFocus, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_faceFocus);
        m_controller->setFaceFocus(checked);
    });
    layout->addWidget(m_faceFocus);

    QWidget *focusBox = new QWidget(group);
    QFormLayout *form = new QFormLayout(focusBox);
    form->setContentsMargins(0, 0, 0, 0);
    m_focus = addSlider(focusBox, form, tr("Distance"), 0, 100, 1,
                        [this](int position) { m_controller->setFocusAbsolute(position, false); });
    layout->addWidget(focusBox);

    connect(m_focusGroup, &QButtonGroup::idClicked, this, [this](int mode) {
        markUserChange(m_focusGroup);
        m_controller->setFocusAbsolute(m_focus.slider->value(), mode == ModeAuto);
        updateFocusVisibility();
    });

    updateFocusVisibility();
    return group;
}

QWidget *Tiny2ImageTab::buildPictureSection()
{
    QGroupBox *group = new QGroupBox(tr("Picture"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    QFormLayout *form = new QFormLayout();
    layout->addLayout(form);

    m_brightness = addSlider(group, form, tr("Brightness"), 0, 100, 1,
                             [this](int v) { m_controller->setBrightness(v); });
    m_contrast = addSlider(group, form, tr("Contrast"), 0, 100, 1,
                           [this](int v) { m_controller->setContrast(v); });
    m_saturation = addSlider(group, form, tr("Saturation"), 0, 100, 1,
                             [this](int v) { m_controller->setSaturation(v); });
    m_sharpness = addSlider(group, form, tr("Sharpness"), 0, 100, 1,
                            [this](int v) { m_controller->setSharpness(v); });
    m_hue = addSlider(group, form, tr("Hue"), 0, 100, 1,
                      [this](int v) { m_controller->setHue(v); });

    QHBoxLayout *bottom = new QHBoxLayout();
    m_mirror = new QCheckBox(tr("Mirror image"), group);
    connect(m_mirror, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_mirror);
        m_controller->setMirror(checked);
    });
    bottom->addWidget(m_mirror);
    bottom->addStretch();
    QPushButton *defaults = new QPushButton(tr("Reset picture"), group);
    defaults->setToolTip(tr("Set brightness, contrast, saturation, sharpness and hue back to 50"));
    connect(defaults, &QPushButton::clicked, this, [this]() {
        for (SliderRow *row : {&m_brightness, &m_contrast, &m_saturation, &m_sharpness, &m_hue})
            row->slider->setValue(50);
    });
    bottom->addWidget(defaults);
    layout->addLayout(bottom);
    return group;
}

void Tiny2ImageTab::updateExposureVisibility()
{
    const bool manual = m_exposureGroup->checkedId() == ModeManual;
    m_faceAE->setVisible(!manual);
    m_shutterLabel->setVisible(manual);
    m_shutterCombo->setVisible(manual);
}

void Tiny2ImageTab::updateWhiteBalanceVisibility()
{
    const bool manual = m_wbGroup->checkedId() == ModeManual;
    m_kelvin.slider->parentWidget()->setVisible(manual);
}

void Tiny2ImageTab::updateFocusVisibility()
{
    const bool manual = m_focusGroup->checkedId() == ModeManual;
    m_faceFocus->setVisible(!manual);
    m_focus.slider->parentWidget()->setVisible(manual);
}

void Tiny2ImageTab::updateFromState(const CameraController::CameraState &state)
{
    auto syncCheck = [this](QCheckBox *box, bool value) {
        if (!isHeld(box) && box->isChecked() != value) {
            QSignalBlocker blocker(box);
            box->setChecked(value);
        }
    };
    auto syncGroup = [this](QButtonGroup *group, int id) {
        if (isHeld(group))
            return;
        if (QAbstractButton *button = group->button(id)) {
            if (!button->isChecked())
                button->setChecked(true);
        }
    };
    auto syncCombo = [this](QComboBox *combo, int data) {
        if (isHeld(combo))
            return;
        const int index = combo->findData(data);
        if (index >= 0 && index != combo->currentIndex()) {
            QSignalBlocker blocker(combo);
            combo->setCurrentIndex(index);
        }
    };

    syncGroup(m_exposureGroup, state.exposureAuto ? ModeAuto : ModeManual);
    syncCheck(m_faceAE, state.faceAEEnabled);
    syncCombo(m_shutterCombo, state.shutter);
    syncCombo(m_antiFlickerCombo, state.antiFlicker);
    syncCheck(m_hdr, state.hdrEnabled);
    updateExposureVisibility();

    const bool wbAuto = state.whiteBalance == static_cast<int>(Device::DevWhiteBalanceAuto);
    syncGroup(m_wbGroup, wbAuto ? ModeAuto : ModeManual);
    syncSlider(m_kelvin, state.whiteBalanceKelvin, QStringLiteral(" K"));
    updateWhiteBalanceVisibility();

    syncGroup(m_focusGroup, state.autoFocusEnabled ? ModeAuto : ModeManual);
    syncCheck(m_faceFocus, state.faceFocusEnabled);
    syncSlider(m_focus, state.manualFocusValue);
    updateFocusVisibility();

    syncSlider(m_brightness, state.brightness);
    syncSlider(m_contrast, state.contrast);
    syncSlider(m_saturation, state.saturation);
    syncSlider(m_sharpness, state.sharpness);
    syncSlider(m_hue, state.hue);
    syncCheck(m_mirror, state.mirrored);
}
