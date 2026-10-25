#include "ModeBar.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

ModeBar::ModeBar(CameraController *controller, QWidget *parent)
    : Tiny2Panel(controller, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QHBoxLayout *modeRow = new QHBoxLayout();
    modeRow->setSpacing(6);
    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    const struct { const char *label; int mode; const char *tip; } modes[] = {
        {QT_TR_NOOP("Off"), Device::AiWorkModeNone, QT_TR_NOOP("No tracking: the gimbal stays where you put it")},
        {QT_TR_NOOP("Human"), Device::AiWorkModeHuman, QT_TR_NOOP("Follow one person, framed as chosen below")},
        {QT_TR_NOOP("Group"), Device::AiWorkModeGroup, QT_TR_NOOP("Keep everyone in shot as people join or leave")},
        {QT_TR_NOOP("Hand"), Device::AiWorkModeHand, QT_TR_NOOP("Follow your hand")},
        {QT_TR_NOOP("Desk"), Device::AiWorkModeDesk, QT_TR_NOOP("Tilt down 30° to show the desk from above")},
        {QT_TR_NOOP("Whiteboard"), Device::AiWorkModeWhiteBoard, QT_TR_NOOP("Find and straighten a whiteboard")},
    };
    for (const auto &m : modes) {
        QPushButton *button = new QPushButton(tr(m.label), this);
        button->setCheckable(true);
        button->setToolTip(tr(m.tip));
        m_modeGroup->addButton(button, m.mode);
        modeRow->addWidget(button);
    }
    modeRow->addStretch();
    layout->addLayout(modeRow);
    connect(m_modeGroup, &QButtonGroup::idClicked, this, &ModeBar::onModeClicked);

    QHBoxLayout *optionsRow = new QHBoxLayout();
    optionsRow->setSpacing(10);

    m_framingLabel = new QLabel(tr("Framing"), this);
    optionsRow->addWidget(m_framingLabel);
    m_framingCombo = new QComboBox(this);
    m_framingCombo->addItem(tr("Normal"), Device::AiSubModeNormal);
    m_framingCombo->addItem(tr("Upper body"), Device::AiSubModeUpperBody);
    m_framingCombo->addItem(tr("Close-up"), Device::AiSubModeCloseUp);
    m_framingCombo->addItem(tr("Headless"), Device::AiSubModeHeadHide);
    m_framingCombo->addItem(tr("Lower body"), Device::AiSubModeLowerBody);
    connect(m_framingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ModeBar::onFramingChanged);
    optionsRow->addWidget(m_framingCombo);

    optionsRow->addSpacing(12);
    QLabel *trackingLabel = new QLabel(tr("Tracking"), this);
    optionsRow->addWidget(trackingLabel);
    m_trackingGroup = new QButtonGroup(this);
    m_trackingGroup->setExclusive(true);
    QPushButton *standard = new QPushButton(tr("Standard"), this);
    standard->setCheckable(true);
    standard->setToolTip(tr("Smooth tracking for people sitting or moving slowly"));
    QPushButton *motion = new QPushButton(tr("Motion"), this);
    motion->setCheckable(true);
    motion->setToolTip(tr("Faster tracking for people moving about"));
    m_trackingGroup->addButton(standard, Device::AiVTrackStandard);
    m_trackingGroup->addButton(motion, Device::AiVTrackMotion);
    optionsRow->addWidget(standard);
    optionsRow->addWidget(motion);
    connect(m_trackingGroup, &QButtonGroup::idClicked, this, &ModeBar::onTrackingModeClicked);

    optionsRow->addSpacing(12);
    m_autoZoomCheckBox = new QCheckBox(tr("Auto zoom"), this);
    m_autoZoomCheckBox->setToolTip(tr("Let tracking zoom to keep the subject framed"));
    connect(m_autoZoomCheckBox, &QCheckBox::toggled, this, &ModeBar::onAutoZoomToggled);
    optionsRow->addWidget(m_autoZoomCheckBox);
    optionsRow->addStretch();
    layout->addLayout(optionsRow);
}

int ModeBar::currentMode() const
{
    const int mode = m_modeGroup->checkedId();
    return mode < 0 ? Device::AiWorkModeNone : mode;
}

int ModeBar::currentFraming() const
{
    return m_framingCombo->currentData().toInt();
}

void ModeBar::selectMode(int mode)
{
    if (QAbstractButton *button = m_modeGroup->button(mode)) {
        button->setChecked(true);
        onModeClicked(mode);
    }
}

void ModeBar::onModeClicked(int mode)
{
    markUserChange(m_modeGroup);
    applyMode(mode);
}

void ModeBar::onFramingChanged(int)
{
    markUserChange(m_framingCombo);
    if (currentMode() == Device::AiWorkModeHuman)
        applyMode(Device::AiWorkModeHuman);
}

void ModeBar::applyMode(int mode)
{
    const int framing = mode == Device::AiWorkModeHuman ? currentFraming() : 0;
    m_controller->setAiMode(mode, framing);
    m_framingCombo->setEnabled(mode == Device::AiWorkModeHuman);
    m_framingLabel->setEnabled(mode == Device::AiWorkModeHuman);
}

void ModeBar::onTrackingModeClicked(int mode)
{
    markUserChange(m_trackingGroup);
    m_controller->setTrackingMode(mode);
}

void ModeBar::onAutoZoomToggled(bool checked)
{
    // The status block has no auto-zoom field, so this stays as last set.
    m_controller->setAutoZoom(checked);
}

void ModeBar::updateFromState(const CameraController::CameraState &state)
{
    if (!isHeld(m_modeGroup)) {
        if (QAbstractButton *button = m_modeGroup->button(state.aiMode)) {
            if (!button->isChecked())
                button->setChecked(true);
        }
    }

    if (!isHeld(m_framingCombo) && !isHeld(m_modeGroup) && state.aiMode == Device::AiWorkModeHuman) {
        const int index = m_framingCombo->findData(state.aiSubMode);
        if (index >= 0 && index != m_framingCombo->currentIndex()) {
            QSignalBlocker blocker(m_framingCombo);
            m_framingCombo->setCurrentIndex(index);
        }
    }

    const bool human = currentMode() == Device::AiWorkModeHuman;
    m_framingCombo->setEnabled(human);
    m_framingLabel->setEnabled(human);

    if (!isHeld(m_trackingGroup)) {
        if (QAbstractButton *button = m_trackingGroup->button(state.trackingMode)) {
            if (!button->isChecked())
                button->setChecked(true);
        }
    }
}
