#include "Tiny2CameraTab.h"
#include "ModeBar.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

constexpr int kZoomSendDelayMs = 120;
constexpr int kGimbalSettleMs = 1500;

QString describePosition(double yawDeg, double pitchDeg, double zoom)
{
    // Pitch is positive looking down; show it the way people read tilt.
    return QObject::tr("Pan %1°, tilt %2°, zoom %3×")
        .arg(yawDeg, 0, 'f', 1)
        .arg(-pitchDeg, 0, 'f', 1)
        .arg(zoom, 0, 'f', 1);
}

} // namespace

Tiny2CameraTab::Tiny2CameraTab(CameraController *controller, ModeBar *modeBar, QWidget *parent)
    : Tiny2Panel(controller, parent)
    , m_modeBar(modeBar)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(16);
    layout->addWidget(buildViewSection());
    layout->addWidget(buildPresetSection());
    layout->addWidget(buildStartupSection());
    layout->addWidget(buildZoneSection());
    layout->addStretch();

    m_zoomSendTimer = new QTimer(this);
    m_zoomSendTimer->setSingleShot(true);
    m_zoomSendTimer->setInterval(kZoomSendDelayMs);
    connect(m_zoomSendTimer, &QTimer::timeout, this, &Tiny2CameraTab::sendZoom);
}

QWidget *Tiny2CameraTab::buildViewSection()
{
    QGroupBox *group = new QGroupBox(tr("View"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(10);

    QHBoxLayout *zoomRow = new QHBoxLayout();
    zoomRow->addWidget(new QLabel(tr("Zoom"), group));
    m_zoomSlider = new QSlider(Qt::Horizontal, group);
    m_zoomSlider->setRange(100, 400);
    m_zoomSlider->setPageStep(25);
    m_zoomSlider->setToolTip(tr("Optical-quality zoom from 1× to 4×. Scroll on the preview to zoom too."));
    connect(m_zoomSlider, &QSlider::valueChanged, this, &Tiny2CameraTab::onZoomSliderChanged);
    zoomRow->addWidget(m_zoomSlider, 1);
    m_zoomValue = new QLabel(QStringLiteral("1.0×"), group);
    m_zoomValue->setMinimumWidth(m_zoomValue->fontMetrics().horizontalAdvance(QStringLiteral("4.00×")));
    zoomRow->addWidget(m_zoomValue);
    layout->addLayout(zoomRow);

    QHBoxLayout *viewRow = new QHBoxLayout();
    viewRow->addWidget(new QLabel(tr("Field of view"), group));
    m_viewGroup = new QButtonGroup(this);
    m_viewGroup->setExclusive(true);
    const struct { const char *label; int fov; } views[] = {
        {QT_TR_NOOP("Wide 86°"), 0},
        {QT_TR_NOOP("Medium 78°"), 1},
        {QT_TR_NOOP("Narrow 65°"), 2},
    };
    for (const auto &v : views) {
        QPushButton *button = new QPushButton(tr(v.label), group);
        button->setCheckable(true);
        m_viewGroup->addButton(button, v.fov);
        viewRow->addWidget(button);
    }
    viewRow->addStretch();
    connect(m_viewGroup, &QButtonGroup::idClicked, this, &Tiny2CameraTab::onViewClicked);
    layout->addLayout(viewRow);

    QHBoxLayout *positionRow = new QHBoxLayout();
    m_positionLabel = new QLabel(tr("Position unknown"), group);
    m_positionLabel->setToolTip(tr("Read from the gimbal motors, so it shows where the camera really points"));
    positionRow->addWidget(m_positionLabel, 1);
    QPushButton *centre = new QPushButton(tr("Re-centre"), group);
    centre->setToolTip(tr("Point the camera straight ahead (turn tracking off first)"));
    connect(centre, &QPushButton::clicked, this, [this]() {
        m_controller->resetGimbal();
        emit gimbalMoved();
    });
    positionRow->addWidget(centre);
    layout->addLayout(positionRow);

    return group;
}

QWidget *Tiny2CameraTab::buildPresetSection()
{
    QGroupBox *group = new QGroupBox(tr("Presets (stored on the camera)"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(8);

    for (int slot = 0; slot < CameraController::kMaxPresets; ++slot) {
        QHBoxLayout *row = new QHBoxLayout();
        m_presetButtons[slot] = new QPushButton(group);
        m_presetButtons[slot]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(m_presetButtons[slot], &QPushButton::clicked, this, [this, slot]() {
            if (presetInSlot(slot))
                recallPreset(slot);
            else
                onPresetMenu(slot);
        });
        row->addWidget(m_presetButtons[slot], 1);

        m_presetMenuButtons[slot] = new QPushButton(QStringLiteral("⋯"), group);
        m_presetMenuButtons[slot]->setToolTip(tr("Save, rename or delete this preset"));
        m_presetMenuButtons[slot]->setAccessibleName(tr("Preset %1 options").arg(slot + 1));
        connect(m_presetMenuButtons[slot], &QPushButton::clicked, this, [this, slot]() {
            onPresetMenu(slot);
        });
        row->addWidget(m_presetMenuButtons[slot]);
        layout->addLayout(row);
    }

    QLabel *note = new QLabel(tr("Presets recall the position, zoom and any working mode saved with them in OBSBOT Center."), group);
    note->setWordWrap(true);
    layout->addWidget(note);

    rebuildPresetButtons();
    return group;
}

QWidget *Tiny2CameraTab::buildStartupSection()
{
    QGroupBox *group = new QGroupBox(tr("Start-up position"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);
    m_startupLabel = new QLabel(tr("Not read yet"), group);
    m_startupLabel->setWordWrap(true);
    layout->addWidget(m_startupLabel);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *set = new QPushButton(tr("Use current view and mode"), group);
    set->setToolTip(tr("The camera returns to this position, zoom and AI mode whenever it powers on"));
    connect(set, &QPushButton::clicked, this, &Tiny2CameraTab::onSetStartup);
    buttons->addWidget(set);
    QPushButton *reset = new QPushButton(tr("Reset to default"), group);
    connect(reset, &QPushButton::clicked, this, &Tiny2CameraTab::onResetStartup);
    buttons->addWidget(reset);
    buttons->addStretch();
    layout->addLayout(buttons);
    return group;
}

QWidget *Tiny2CameraTab::buildZoneSection()
{
    QGroupBox *group = new QGroupBox(tr("Tracking zone"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QLabel *intro = new QLabel(tr("Keep tracking inside these limits. When the subject leaves, the camera "
                                  "goes back to its resting position instead of wandering."), group);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_zoneEnabled = new QCheckBox(tr("Limit tracking to this zone"), group);
    m_zoneEnabled->setToolTip(tr("The camera can't report this setting, so the box shows what was last set here"));
    connect(m_zoneEnabled, &QCheckBox::toggled, this, &Tiny2CameraTab::onZoneEnabledToggled);
    layout->addWidget(m_zoneEnabled);

    auto makeSpin = [group](double min, double max) {
        QDoubleSpinBox *spin = new QDoubleSpinBox(group);
        spin->setRange(min, max);
        spin->setDecimals(0);
        spin->setSuffix(QStringLiteral("°"));
        return spin;
    };
    QFormLayout *form = new QFormLayout();
    m_zoneYawMin = makeSpin(-120, 120);
    m_zoneYawMax = makeSpin(-120, 120);
    m_zonePitchMin = makeSpin(-90, 90);
    m_zonePitchMax = makeSpin(-90, 90);
    QHBoxLayout *yawRow = new QHBoxLayout();
    yawRow->addWidget(m_zoneYawMin);
    yawRow->addWidget(new QLabel(tr("to"), group));
    yawRow->addWidget(m_zoneYawMax);
    yawRow->addStretch();
    form->addRow(tr("Pan"), yawRow);
    QHBoxLayout *pitchRow = new QHBoxLayout();
    pitchRow->addWidget(m_zonePitchMin);
    pitchRow->addWidget(new QLabel(tr("to"), group));
    pitchRow->addWidget(m_zonePitchMax);
    pitchRow->addStretch();
    form->addRow(tr("Tilt"), pitchRow);
    layout->addLayout(form);

    m_zoneAutoSelect = new QCheckBox(tr("Pick up a new subject automatically"), group);
    layout->addWidget(m_zoneAutoSelect);

    QHBoxLayout *applyRow = new QHBoxLayout();
    QPushButton *apply = new QPushButton(tr("Apply limits"), group);
    connect(apply, &QPushButton::clicked, this, &Tiny2CameraTab::onApplyZone);
    applyRow->addWidget(apply);
    applyRow->addStretch();
    layout->addLayout(applyRow);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *rest = new QPushButton(tr("Use current view as resting position"), group);
    connect(rest, &QPushButton::clicked, this, [this]() { m_controller->setZoneStartFromCurrent(); });
    buttons->addWidget(rest);
    QPushButton *goRest = new QPushButton(tr("Go to resting position"), group);
    connect(goRest, &QPushButton::clicked, this, [this]() {
        m_controller->recallZoneStart();
        emit gimbalMoved();
    });
    buttons->addWidget(goRest);
    buttons->addStretch();
    layout->addLayout(buttons);
    return group;
}

const CameraController::CameraPreset *Tiny2CameraTab::presetInSlot(int slot) const
{
    for (const auto &preset : m_presets) {
        if (preset.id == slot)
            return &preset;
    }
    return nullptr;
}

void Tiny2CameraTab::rebuildPresetButtons()
{
    for (int slot = 0; slot < CameraController::kMaxPresets; ++slot) {
        const auto *preset = presetInSlot(slot);
        QPushButton *button = m_presetButtons[slot];
        if (preset) {
            const QString name = preset->name.isEmpty() ? tr("Preset %1").arg(slot + 1) : preset->name;
            button->setText(QStringLiteral("%1  ·  %2").arg(slot + 1).arg(name));
            button->setToolTip(describePosition(preset->yawDeg, preset->pitchDeg, preset->zoom));
        } else {
            button->setText(tr("%1  ·  Empty: save the current view").arg(slot + 1));
            button->setToolTip(tr("Saves the current position and zoom in this slot"));
        }
    }
}

void Tiny2CameraTab::refreshFromCamera()
{
    m_presets = m_controller->readPresets();
    rebuildPresetButtons();
    emit presetsChanged(m_presets);

    const auto startup = m_controller->readStartupPosition();
    m_startupLabel->setText(startup.id < 0
        ? tr("Couldn't read the start-up position")
        : describePosition(startup.yawDeg, startup.pitchDeg, startup.zoom));

    const auto zone = m_controller->readZoneLimits();
    if (zone.valid) {
        const QSignalBlocker b1(m_zoneYawMin), b2(m_zoneYawMax), b3(m_zonePitchMin), b4(m_zonePitchMax), b5(m_zoneAutoSelect);
        m_zoneYawMin->setValue(zone.yawMinDeg);
        m_zoneYawMax->setValue(zone.yawMaxDeg);
        // The SDK's pitch is positive looking down; the UI shows tilt up as positive.
        m_zonePitchMin->setValue(-zone.pitchMaxDeg);
        m_zonePitchMax->setValue(-zone.pitchMinDeg);
        m_zoneAutoSelect->setChecked(zone.autoSelect);
    }
}

void Tiny2CameraTab::recallPreset(int slot)
{
    if (!presetInSlot(slot))
        return;
    m_controller->recallPreset(slot);
    emit gimbalMoved();
}

void Tiny2CameraTab::onPresetMenu(int slot)
{
    const auto *preset = presetInSlot(slot);
    QMenu menu(this);
    QAction *save = menu.addAction(preset ? tr("Replace with the current view") : tr("Save the current view here"));
    QAction *rename = preset ? menu.addAction(tr("Rename…")) : nullptr;
    QAction *remove = preset ? menu.addAction(tr("Delete")) : nullptr;

    QAction *chosen = menu.exec(m_presetMenuButtons[slot]->mapToGlobal(QPoint(0, m_presetMenuButtons[slot]->height())));
    if (!chosen)
        return;

    if (chosen == save) {
        bool ok = true;
        QString name = preset ? preset->name : QString();
        if (!preset) {
            name = QInputDialog::getText(this, tr("Save preset %1").arg(slot + 1), tr("Name"),
                                         QLineEdit::Normal, tr("Preset %1").arg(slot + 1), &ok);
            if (!ok)
                return;
        }
        m_controller->savePreset(slot, name.trimmed());
    } else if (chosen == rename) {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Rename preset %1").arg(slot + 1), tr("Name"),
                                                   QLineEdit::Normal, preset->name, &ok);
        if (!ok)
            return;
        m_controller->renamePreset(slot, name.trimmed());
    } else if (chosen == remove) {
        const auto answer = QMessageBox::question(this, tr("Delete preset"),
            tr("Delete preset %1 from the camera?").arg(slot + 1));
        if (answer != QMessageBox::Yes)
            return;
        m_controller->deletePreset(slot);
    }

    QTimer::singleShot(300, this, &Tiny2CameraTab::refreshFromCamera);
}

void Tiny2CameraTab::onSetStartup()
{
    const int mode = m_modeBar ? m_modeBar->currentMode() : Device::AiWorkModeNone;
    const int framing = m_modeBar && mode == Device::AiWorkModeHuman ? m_modeBar->currentFraming() : 0;
    if (m_controller->setStartupFromCurrent(mode, framing))
        QTimer::singleShot(300, this, &Tiny2CameraTab::refreshFromCamera);
}

void Tiny2CameraTab::onResetStartup()
{
    if (m_controller->resetStartup())
        QTimer::singleShot(300, this, &Tiny2CameraTab::refreshFromCamera);
}

void Tiny2CameraTab::onApplyZone()
{
    CameraController::ZoneLimits limits;
    limits.yawMinDeg = qMin(m_zoneYawMin->value(), m_zoneYawMax->value());
    limits.yawMaxDeg = qMax(m_zoneYawMin->value(), m_zoneYawMax->value());
    limits.pitchMinDeg = -qMax(m_zonePitchMin->value(), m_zonePitchMax->value());
    limits.pitchMaxDeg = -qMin(m_zonePitchMin->value(), m_zonePitchMax->value());
    limits.autoSelect = m_zoneAutoSelect->isChecked();
    m_controller->setZoneLimits(limits);
    QTimer::singleShot(300, this, &Tiny2CameraTab::refreshFromCamera);
}

void Tiny2CameraTab::onZoneEnabledToggled(bool enabled)
{
    m_controller->setZoneTracking(enabled);
}

void Tiny2CameraTab::onZoomSliderChanged(int value)
{
    markUserChange(m_zoomSlider);
    m_zoomValue->setText(QStringLiteral("%1×").arg(value / 100.0, 0, 'f', 2));
    m_zoomSendTimer->start();
}

void Tiny2CameraTab::sendZoom()
{
    markUserChange(m_zoomSlider);
    m_controller->setZoom(m_zoomSlider->value() / 100.0);
}

void Tiny2CameraTab::stepZoom(double delta)
{
    m_zoomSlider->setValue(m_zoomSlider->value() + qRound(delta * 100.0));
}

void Tiny2CameraTab::onViewClicked(int fov)
{
    markUserChange(m_viewGroup);
    m_controller->setFOV(fov);
}

void Tiny2CameraTab::setGimbalAngles(const CameraController::GimbalAngles &angles)
{
    if (!angles.valid) {
        m_positionLabel->setText(tr("Position unknown"));
        return;
    }
    m_positionLabel->setText(tr("Pan %1°, tilt %2°")
        .arg(angles.yawDeg, 0, 'f', 1)
        .arg(-angles.pitchDeg, 0, 'f', 1));
}

void Tiny2CameraTab::updateFromState(const CameraController::CameraState &state)
{
    m_zoomSlider->setMaximum(qRound(m_controller->maxZoom() * 100.0));
    if (!isHeld(m_zoomSlider) && !m_zoomSlider->isSliderDown()) {
        const int value = qRound(state.zoom * 100.0);
        if (value != m_zoomSlider->value()) {
            QSignalBlocker blocker(m_zoomSlider);
            m_zoomSlider->setValue(value);
            m_zoomValue->setText(QStringLiteral("%1×").arg(state.zoom, 0, 'f', 2));
        }
    }

    if (!isHeld(m_viewGroup)) {
        if (QAbstractButton *button = m_viewGroup->button(state.fovMode)) {
            if (!button->isChecked())
                button->setChecked(true);
        }
    }
}
