#include "Tiny2DeviceTab.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

Tiny2DeviceTab::Tiny2DeviceTab(CameraController *controller, QWidget *parent)
    : Tiny2Panel(controller, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(16);
    layout->addWidget(buildPowerSection());
    layout->addWidget(buildGestureSection());
    layout->addWidget(buildVoiceSection());
    layout->addWidget(buildAudioSection());
    layout->addWidget(buildAboutSection());
    layout->addStretch();
}

QWidget *Tiny2DeviceTab::buildPowerSection()
{
    QGroupBox *group = new QGroupBox(tr("Sleep"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QHBoxLayout *stateRow = new QHBoxLayout();
    m_powerState = new QLabel(tr("Awake"), group);
    stateRow->addWidget(m_powerState, 1);
    m_sleepWake = new QPushButton(tr("Sleep now"), group);
    connect(m_sleepWake, &QPushButton::clicked, this, [this]() {
        markUserChange(m_sleepWake);
        m_asleep = !m_asleep;
        m_controller->setAwake(!m_asleep);
        m_powerState->setText(m_asleep ? tr("Asleep: the gimbal parks and the stream goes dark") : tr("Awake"));
        m_sleepWake->setText(m_asleep ? tr("Wake") : tr("Sleep now"));
    });
    stateRow->addWidget(m_sleepWake);
    layout->addLayout(stateRow);

    QFormLayout *form = new QFormLayout();
    m_autoSleep = new QComboBox(group);
    const struct { const char *label; int seconds; } options[] = {
        {QT_TR_NOOP("Never"), 0},
        {QT_TR_NOOP("After 1 minute"), 60},
        {QT_TR_NOOP("After 2 minutes"), 120},
        {QT_TR_NOOP("After 5 minutes"), 300},
        {QT_TR_NOOP("After 10 minutes"), 600},
        {QT_TR_NOOP("After 30 minutes"), 1800},
    };
    for (const auto &o : options)
        m_autoSleep->addItem(tr(o.label), o.seconds);
    m_autoSleep->setToolTip(tr("Sleep when no app has been streaming for this long"));
    connect(m_autoSleep, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        markUserChange(m_autoSleep);
        m_controller->setAutoSleep(m_autoSleep->currentData().toInt());
    });
    form->addRow(tr("Auto sleep"), m_autoSleep);
    layout->addLayout(form);

    m_micDuringSleep = new QCheckBox(tr("Keep the microphone on while asleep"), group);
    connect(m_micDuringSleep, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_micDuringSleep);
        m_controller->setMicDuringSleep(checked);
    });
    layout->addWidget(m_micDuringSleep);
    return group;
}

QWidget *Tiny2DeviceTab::buildGestureSection()
{
    QGroupBox *group = new QGroupBox(tr("Gesture control"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    const char *labels[4] = {
        QT_TR_NOOP("Open palm: start or stop tracking you"),
        QT_TR_NOOP("Pointing: zoom in or out"),
        QT_TR_NOOP("Two-hand “L”: dynamic zoom"),
        QT_TR_NOOP("Reverse the dynamic zoom direction"),
    };
    for (int i = 0; i < 4; ++i) {
        m_gestures[i] = new QCheckBox(tr(labels[i]), group);
        connect(m_gestures[i], &QCheckBox::toggled, this, [this, i](bool checked) {
            markUserChange(m_gestures[i]);
            m_controller->setGesture(i, checked);
        });
        layout->addWidget(m_gestures[i]);
    }
    m_gestureZoomNote = new QLabel(group);
    m_gestureZoomNote->setWordWrap(true);
    layout->addWidget(m_gestureZoomNote);
    return group;
}

QWidget *Tiny2DeviceTab::buildVoiceSection()
{
    QGroupBox *group = new QGroupBox(tr("Voice control"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QFormLayout *form = new QFormLayout();
    m_voiceLanguage = new QComboBox(group);
    m_voiceLanguage->addItem(tr("English"), Device::AudioCtrlLangEnUS);
    m_voiceLanguage->addItem(tr("Chinese (Mandarin)"), Device::AudioCtrlLangZhCN);
    connect(m_voiceLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        markUserChange(m_voiceLanguage);
        m_controller->setVoiceLanguage(m_voiceLanguage->currentData().toInt());
    });
    form->addRow(tr("Language"), m_voiceLanguage);
    layout->addLayout(form);

    // Order follows Device::AudioCtrlCmdType, whose values are the status bits.
    const char *commands[7] = {
        QT_TR_NOOP("“Hi, Tiny” wakes the camera"),
        QT_TR_NOOP("“Sleep, Tiny” puts it to sleep"),
        QT_TR_NOOP("“Track me” starts tracking"),
        QT_TR_NOOP("“Unlock me” stops tracking"),
        QT_TR_NOOP("“Zoom in closer”"),
        QT_TR_NOOP("“Zoom out further”"),
        QT_TR_NOOP("“Position one / two / three” recalls a preset"),
    };
    for (int i = 0; i < 7; ++i) {
        m_voiceCommands[i] = new QCheckBox(tr(commands[i]), group);
        connect(m_voiceCommands[i], &QCheckBox::toggled, this, [this, i](bool checked) {
            markUserChange(m_voiceCommands[i]);
            m_controller->setVoiceCommand(i, checked);
        });
        layout->addWidget(m_voiceCommands[i]);
    }

    QHBoxLayout *zoomRow = new QHBoxLayout();
    zoomRow->addWidget(new QLabel(tr("“Zoom in closer” goes to"), group));
    m_voiceZoom = new QSlider(Qt::Horizontal, group);
    m_voiceZoom->setRange(100, 400);
    m_voiceZoom->setPageStep(25);
    m_voiceZoom->setAccessibleName(tr("Voice zoom level"));
    zoomRow->addWidget(m_voiceZoom, 1);
    m_voiceZoomValue = new QLabel(QStringLiteral("2.0×"), group);
    zoomRow->addWidget(m_voiceZoomValue);
    layout->addLayout(zoomRow);

    m_voiceZoomTimer = new QTimer(this);
    m_voiceZoomTimer->setSingleShot(true);
    m_voiceZoomTimer->setInterval(200);
    connect(m_voiceZoomTimer, &QTimer::timeout, this, [this]() {
        markUserChange(m_voiceZoom);
        m_controller->setVoiceZoom(m_voiceZoom->value() / 100.0);
    });
    connect(m_voiceZoom, &QSlider::valueChanged, this, [this](int value) {
        markUserChange(m_voiceZoom);
        m_voiceZoomValue->setText(QStringLiteral("%1×").arg(value / 100.0, 0, 'f', 1));
        m_voiceZoomTimer->start();
    });
    return group;
}

QWidget *Tiny2DeviceTab::buildAudioSection()
{
    QGroupBox *group = new QGroupBox(tr("Microphone"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    m_autoGain = new QCheckBox(tr("Automatic gain"), group);
    connect(m_autoGain, &QCheckBox::toggled, this, [this](bool checked) {
        markUserChange(m_autoGain);
        m_controller->setAudioAutoGain(checked);
    });
    layout->addWidget(m_autoGain);
    return group;
}

QWidget *Tiny2DeviceTab::buildAboutSection()
{
    QGroupBox *group = new QGroupBox(tr("Device"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    m_about = new QLabel(group);
    m_about->setWordWrap(true);
    m_about->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_about);
    m_ledLevel = new QLabel(group);
    layout->addWidget(m_ledLevel);

    QLabel *missing = new QLabel(tr("Exposure compensation, ISO, LED brightness and microphone noise "
                                    "reduction can only be changed in OBSBOT Center: the SDK has no "
                                    "Tiny 2 commands for them."), group);
    missing->setWordWrap(true);
    layout->addWidget(missing);

    QHBoxLayout *resetRow = new QHBoxLayout();
    resetRow->addStretch();
    QPushButton *reset = new QPushButton(tr("Factory reset…"), group);
    connect(reset, &QPushButton::clicked, this, [this]() {
        const auto answer = QMessageBox::warning(this, tr("Factory reset"),
            tr("Reset the camera to its factory settings? This clears its presets, "
               "start-up position and every setting stored on the camera."),
            QMessageBox::Reset | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer == QMessageBox::Reset)
            m_controller->factoryReset();
    });
    resetRow->addWidget(reset);
    layout->addLayout(resetRow);
    return group;
}

void Tiny2DeviceTab::setCameraInfo(const CameraController::CameraInfo &info)
{
    m_about->setText(tr("%1\nSerial number %2\nFirmware %3")
        .arg(info.name, info.serialNumber, info.version));
}

void Tiny2DeviceTab::refreshFromCamera()
{
    const auto gestures = m_controller->readGestures();
    if (!gestures.valid)
        return;
    const bool values[4] = {gestures.targetSelect, gestures.zoom, gestures.dynamicZoom, gestures.dynamicZoomMirror};
    for (int i = 0; i < 4; ++i) {
        if (!isHeld(m_gestures[i]) && m_gestures[i]->isChecked() != values[i]) {
            QSignalBlocker blocker(m_gestures[i]);
            m_gestures[i]->setChecked(values[i]);
        }
    }
    m_gestureZoomNote->setText(tr("The zoom gesture goes to %1× (set in OBSBOT Center).")
        .arg(gestures.zoomFactor, 0, 'f', 1));
}

void Tiny2DeviceTab::updateFromState(const CameraController::CameraState &state)
{
    if (!isHeld(m_sleepWake)) {
        m_asleep = state.devStatus == Device::DevStatusSleep;
        m_powerState->setText(m_asleep ? tr("Asleep: the gimbal parks and the stream goes dark") : tr("Awake"));
        m_sleepWake->setText(m_asleep ? tr("Wake") : tr("Sleep now"));
    }

    if (!isHeld(m_autoSleep)) {
        int index = m_autoSleep->findData(state.autoSleepSeconds);
        if (index < 0) {
            m_autoSleep->addItem(tr("After %1 seconds").arg(state.autoSleepSeconds), state.autoSleepSeconds);
            index = m_autoSleep->count() - 1;
        }
        if (index != m_autoSleep->currentIndex()) {
            QSignalBlocker blocker(m_autoSleep);
            m_autoSleep->setCurrentIndex(index);
        }
    }

    auto syncCheck = [this](QCheckBox *box, bool value) {
        if (!isHeld(box) && box->isChecked() != value) {
            QSignalBlocker blocker(box);
            box->setChecked(value);
        }
    };
    syncCheck(m_micDuringSleep, state.micDuringSleep);
    for (int i = 0; i < 7; ++i)
        syncCheck(m_voiceCommands[i], state.voiceCommands & (1 << i));
    syncCheck(m_autoGain, state.audioAutoGainEnabled);

    if (!isHeld(m_voiceLanguage)) {
        const int index = m_voiceLanguage->findData(state.voiceLanguage);
        if (index >= 0 && index != m_voiceLanguage->currentIndex()) {
            QSignalBlocker blocker(m_voiceLanguage);
            m_voiceLanguage->setCurrentIndex(index);
        }
    }
    if (!isHeld(m_voiceZoom) && !m_voiceZoom->isSliderDown()) {
        QSignalBlocker blocker(m_voiceZoom);
        m_voiceZoom->setValue(qRound(state.voiceZoom * 100.0));
        m_voiceZoomValue->setText(QStringLiteral("%1×").arg(state.voiceZoom, 0, 'f', 1));
    }

    m_ledLevel->setText(state.ledLevel == 0 ? tr("Status light: off")
                                            : tr("Status light: brightness %1 of 3").arg(state.ledLevel));
}
