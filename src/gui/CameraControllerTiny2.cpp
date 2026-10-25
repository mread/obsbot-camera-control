// Tiny 2 features beyond the original controls. Every SDK call here was
// checked against a Tiny 2 (firmware 6.0.11.1) by reading the result back,
// either from the status block or from the matching getter.
#include "CameraController.h"
#include <QDebug>
#include <algorithm>
#include <cstring>

namespace {

// Gimbal ranges OBSBOT Center allows for the Tiny 2 (OSC SetGimMotorDegree).
constexpr double kYawLimitDeg = 129.0;
constexpr double kPitchLimitDeg = 59.0;

QString presetName(const Device::DevDataArray &array)
{
    const int len = std::clamp(array.len, 0, 63);
    return QString::fromUtf8(reinterpret_cast<const char *>(array.data_uint8), len).trimmed();
}

} // namespace

bool CameraController::isTiny2() const
{
    return m_connected && !m_v4l2Only && m_device && m_cameraInfo.productType == ObsbotProdTiny2;
}

CameraController::GimbalAngles CameraController::readGimbalAngles()
{
    GimbalAngles angles;
    if (!isTiny2())
        return angles;

    // The camera drops the odd request (seen as -3 here), more often while
    // other programs are also talking to it. One retry is normally enough.
    constexpr int kAttempts = 3;
    float rollPitchYaw[3] = {0.0f, 0.0f, 0.0f};
    int attempt = 0;
    while (m_device->gimbalGetAttitudeInfoR(rollPitchYaw) != 0) {
        if (++attempt == kAttempts)
            return angles;
    }

    angles.pitchDeg = rollPitchYaw[1];
    angles.yawDeg = rollPitchYaw[2];
    angles.valid = true;
    return angles;
}

bool CameraController::setGimbalAngles(double yawDeg, double pitchDeg)
{
    if (!isTiny2()) return false;

    yawDeg = qBound(-kYawLimitDeg, yawDeg, kYawLimitDeg);
    pitchDeg = qBound(-kPitchLimitDeg, pitchDeg, kPitchLimitDeg);
    return executeCommand("Move Gimbal", [this, yawDeg, pitchDeg]() {
        return m_device->aiSetGimbalMotorAngleR(static_cast<float>(pitchDeg), static_cast<float>(yawDeg));
    });
}

bool CameraController::setGimbalSpeed(double yawDegPerSec, double pitchDegPerSec)
{
    if (!isTiny2()) return false;

    // Continuous motion for drag-to-aim and the aiming pad. Failures are not
    // reported through commandFailed: these are sent many times a second.
    return m_device->gimbalSpeedCtrlR(pitchDegPerSec, yawDegPerSec) == 0;
}

bool CameraController::stopGimbal()
{
    if (!isTiny2()) return false;

    constexpr int kAttempts = 3;
    for (int attempt = 0; attempt < kAttempts; ++attempt) {
        if (m_device->gimbalSpeedCtrlR(0.0, 0.0) == 0)
            return true;
    }
    emit commandFailed("Stop Gimbal", -1);
    return false;
}

bool CameraController::resetGimbal()
{
    if (!isTiny2()) return false;
    return executeCommand("Reset Gimbal", [this]() {
        return m_device->gimbalRstPosR();
    });
}

QList<CameraController::CameraPreset> CameraController::readPresets()
{
    QList<CameraPreset> presets;
    if (!isTiny2()) return presets;

    Device::DevDataArray ids{};
    if (m_device->aiGetGimbalPresetListR(&ids) != 0)
        return presets;

    // Preset ids come back as int32 values, not bytes.
    const int count = std::clamp(ids.len, 0, 16);
    for (int i = 0; i < count; ++i) {
        const int id = ids.data_int32[i];
        Device::PresetPosInfo info{};
        if (m_device->aiGetGimbalPresetInfoWithIdR(&info, id) != 0)
            continue;

        CameraPreset preset;
        preset.id = id;
        preset.yawDeg = info.yaw;
        preset.pitchDeg = info.pitch;
        preset.zoom = info.zoom;
        // PresetPosInfo.name comes back empty; the name has its own getter.
        Device::DevDataArray name{};
        if (m_device->aiGetGimbalPresetNameWithIdR(&name, id) == 0)
            preset.name = presetName(name);
        presets.append(preset);
    }

    std::sort(presets.begin(), presets.end(), [](const CameraPreset &a, const CameraPreset &b) {
        return a.id < b.id;
    });
    return presets;
}

bool CameraController::recallPreset(int id)
{
    if (!isTiny2()) return false;
    return executeCommand("Recall Preset", [this, id]() {
        return m_device->aiTrgGimbalPresetR(id);
    });
}

bool CameraController::savePreset(int id, const QString &name)
{
    if (!isTiny2() || id < 0 || id >= kMaxPresets) return false;

    const GimbalAngles angles = readGimbalAngles();
    if (!angles.valid) {
        emit commandFailed("Save Preset (reading the gimbal position)", -1);
        return false;
    }

    Device::PresetPosInfo info{};
    info.id = id;
    info.yaw = static_cast<float>(angles.yawDeg);
    info.pitch = static_cast<float>(angles.pitchDeg);
    info.zoom = static_cast<float>(getCurrentState().zoom);
    const QByteArray utf8 = name.toUtf8().left(63);
    info.name_len = utf8.size();
    std::memcpy(info.name, utf8.constData(), utf8.size());

    // aiAddGimbalPresetR also updates an existing preset with the same id.
    return executeCommand("Save Preset", [this, &info]() {
        return m_device->aiAddGimbalPresetR(&info);
    });
}

bool CameraController::renamePreset(int id, const QString &name)
{
    if (!isTiny2()) return false;
    const std::string utf8 = name.toUtf8().left(63).toStdString();
    return executeCommand("Rename Preset", [this, id, utf8]() {
        return m_device->aiSetGimbalPresetNameWithIdR(utf8, id);
    });
}

bool CameraController::deletePreset(int id)
{
    if (!isTiny2()) return false;
    return executeCommand("Delete Preset", [this, id]() {
        return m_device->aiDelGimbalPresetR(id);
    });
}

CameraController::CameraPreset CameraController::readStartupPosition()
{
    CameraPreset startup;
    if (!isTiny2()) return startup;

    Device::PresetPosInfo info{};
    if (m_device->aiGetGimbalBootPosR(&info) != 0)
        return startup;

    startup.id = 0;
    startup.yawDeg = info.yaw;
    startup.pitchDeg = info.pitch;
    startup.zoom = info.zoom;
    return startup;
}

bool CameraController::setStartupFromCurrent(int aiMode, int aiSubMode)
{
    if (!isTiny2()) return false;

    const GimbalAngles angles = readGimbalAngles();
    if (!angles.valid) {
        emit commandFailed("Set Start-up Position (reading the gimbal position)", -1);
        return false;
    }

    Device::PresetPosInfo info{};
    info.yaw = static_cast<float>(angles.yawDeg);
    info.pitch = static_cast<float>(angles.pitchDeg);
    info.zoom = static_cast<float>(getCurrentState().zoom);
    if (!executeCommand("Set Start-up Position", [this, &info]() {
            return m_device->aiSetGimbalBootPosR(info);
        })) {
        return false;
    }

    return executeCommand("Set Start-up Mode", [this, aiMode, aiSubMode]() {
        return m_device->cameraSetBootModeU(static_cast<Device::AiWorkModeType>(aiMode),
                                            static_cast<Device::AiSubModeType>(aiSubMode));
    });
}

bool CameraController::resetStartup()
{
    if (!isTiny2()) return false;
    return executeCommand("Reset Start-up Position", [this]() {
        return m_device->aiRstGimbalBootPosR();
    });
}

bool CameraController::setAwake(bool awake)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand(awake ? "Wake Camera" : "Sleep Camera", [this, awake]() {
        return m_device->cameraSetDevRunStatusR(awake ? Device::DevStatusRun : Device::DevStatusSleep);
    });
    if (success) {
        m_currentState.devStatus = awake ? Device::DevStatusRun : Device::DevStatusSleep;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setAutoSleep(int seconds)
{
    if (!isTiny2()) return false;
    bool success = executeCommand("Set Auto Sleep", [this, seconds]() {
        return m_device->cameraSetSuspendTimeU(seconds);
    });
    if (success) {
        m_currentState.autoSleepSeconds = seconds;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setMicDuringSleep(bool enabled)
{
    if (!isTiny2()) return false;
    bool success = executeCommand("Set Microphone During Sleep", [this, enabled]() {
        return m_device->cameraSetMicrophoneDuringSleepU(enabled ? 1 : 0);
    });
    if (success) {
        m_currentState.micDuringSleep = enabled;
        emit stateChanged(m_currentState);
    }
    return success;
}

CameraController::GestureSettings CameraController::readGestures()
{
    GestureSettings gestures;
    if (!isTiny2()) return gestures;

    Device::AiStatus status{};
    if (m_device->aiGetAiStatusR(&status) != 0)
        return gestures;

    gestures.targetSelect = status.gesture_target;
    gestures.zoom = status.gesture_zoom;
    gestures.dynamicZoom = status.gesture_dynamic_zoom;
    gestures.dynamicZoomMirror = status.gesture_mirror;
    gestures.zoomFactor = status.gesture_zoom_factor;
    gestures.valid = true;
    return gestures;
}

bool CameraController::setGesture(int gesture, bool enabled)
{
    if (!isTiny2()) return false;
    return executeCommand("Set Gesture Control", [this, gesture, enabled]() {
        return m_device->aiSetGestureCtrlIndividualR(gesture, enabled);
    });
}

bool CameraController::setVoiceCommand(int command, bool enabled)
{
    if (!isTiny2()) return false;
    bool success = executeCommand("Set Voice Command", [this, command, enabled]() {
        return m_device->cameraSetAudioCtrlStateU(static_cast<Device::AudioCtrlCmdType>(command), enabled ? 1 : 0);
    });
    if (success) {
        if (enabled)
            m_currentState.voiceCommands |= (1 << command);
        else
            m_currentState.voiceCommands &= ~(1 << command);
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setVoiceLanguage(int language)
{
    if (!isTiny2()) return false;
    bool success = executeCommand("Set Voice Language", [this, language]() {
        return m_device->cameraSetAudioCtrlStateU(Device::AudioCtrlLanguage, language);
    });
    if (success) {
        m_currentState.voiceLanguage = language;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setVoiceZoom(double zoom)
{
    if (!isTiny2()) return false;
    zoom = qBound(1.0, zoom, maxZoom());
    // The command takes 0-100 across the camera's zoom range, like the status block.
    const int level = qRound((zoom - 1.0) / (maxZoom() - 1.0) * 100.0);
    bool success = executeCommand("Set Voice Zoom", [this, level]() {
        return m_device->cameraSetAudioCtrlStateU(Device::AudioCtrlZoomFactor, level);
    });
    if (success) {
        m_currentState.voiceZoom = zoom;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setSharpness(int value)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand("Set Sharpness", [this, value]() {
        return m_device->cameraSetImageSharpR(value);
    });
    if (success) {
        m_currentState.sharpness = value;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setHue(int value)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand("Set Hue", [this, value]() {
        return m_device->cameraSetImageHueR(value);
    });
    if (success) {
        m_currentState.hue = value;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setAntiFlicker(int mode)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand("Set Anti-flicker", [this, mode]() {
        return m_device->cameraSetAntiFlickR(mode);
    });
    if (success) {
        m_currentState.antiFlicker = mode;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setExposure(bool autoExposure, int shutter)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand("Set Exposure", [this, autoExposure, shutter]() {
        return m_device->cameraSetExposureAbsolute(shutter, autoExposure);
    });
    if (success) {
        m_currentState.exposureAuto = autoExposure;
        m_currentState.shutter = shutter;
        emit stateChanged(m_currentState);
    }
    return success;
}

bool CameraController::setMirror(bool mirrored)
{
    if (!m_connected || m_v4l2Only) return false;
    bool success = executeCommand("Set Mirror", [this, mirrored]() {
        return m_device->cameraSetImageFlipHorizonU(mirrored ? 1 : 0);
    });
    if (success) {
        m_currentState.mirrored = mirrored;
        emit stateChanged(m_currentState);
    }
    return success;
}

CameraController::ZoneLimits CameraController::readZoneLimits()
{
    ZoneLimits limits;
    if (!isTiny2()) return limits;

    float deg = 0.0f;
    bool ok = true;
    ok &= m_device->aiGetLimitedZoneTrackYawMinR(deg, 0) == 0;   limits.yawMinDeg = deg;
    ok &= m_device->aiGetLimitedZoneTrackYawMaxR(deg, 0) == 0;   limits.yawMaxDeg = deg;
    ok &= m_device->aiGetLimitedZoneTrackPitchMinR(deg, 0) == 0; limits.pitchMinDeg = deg;
    ok &= m_device->aiGetLimitedZoneTrackPitchMaxR(deg, 0) == 0; limits.pitchMaxDeg = deg;
    bool autoSelect = false;
    if (m_device->aiGetLimitedZoneTrackAutoSelectR(autoSelect) == 0)
        limits.autoSelect = autoSelect;
    limits.valid = ok;
    return limits;
}

bool CameraController::setZoneLimits(const ZoneLimits &limits)
{
    if (!isTiny2()) return false;
    return executeCommand("Set Tracking Zone", [this, limits]() {
        int32_t ret = m_device->aiSetLimitedZoneTrackYawMinR(static_cast<float>(limits.yawMinDeg));
        if (ret == 0) ret = m_device->aiSetLimitedZoneTrackYawMaxR(static_cast<float>(limits.yawMaxDeg));
        if (ret == 0) ret = m_device->aiSetLimitedZoneTrackPitchMinR(static_cast<float>(limits.pitchMinDeg));
        if (ret == 0) ret = m_device->aiSetLimitedZoneTrackPitchMaxR(static_cast<float>(limits.pitchMaxDeg));
        if (ret == 0) ret = m_device->aiSetLimitedZoneTrackAutoSelectR(limits.autoSelect);
        return ret;
    });
}

bool CameraController::setZoneTracking(bool enabled)
{
    if (!isTiny2()) return false;
    // aiGetLimitedZoneTrackEnabledR returns an error on the Tiny 2, so this
    // state can't be read back; the UI keeps what it last set.
    return executeCommand(enabled ? "Enable Tracking Zone" : "Disable Tracking Zone", [this, enabled]() {
        return m_device->aiSetLimitedZoneTrackEnabledR(enabled);
    });
}

bool CameraController::setZoneStartFromCurrent()
{
    if (!isTiny2()) return false;

    const GimbalAngles angles = readGimbalAngles();
    if (!angles.valid) {
        emit commandFailed("Set Zone Resting Position (reading the gimbal position)", -1);
        return false;
    }

    Device::PresetPosInfo info{};
    info.yaw = static_cast<float>(angles.yawDeg);
    info.pitch = static_cast<float>(angles.pitchDeg);
    info.zoom = static_cast<float>(getCurrentState().zoom);
    return executeCommand("Set Zone Resting Position", [this, &info]() {
        return m_device->aiSetLimitedZoneTrackInitPosR(&info);
    });
}

bool CameraController::recallZoneStart()
{
    if (!isTiny2()) return false;
    return executeCommand("Go to Zone Resting Position", [this]() {
        return m_device->aiTrgLimitedZoneTrackInitPosR();
    });
}

bool CameraController::factoryReset()
{
    if (!m_connected || m_v4l2Only) return false;
    return executeCommand("Factory Reset", [this]() {
        return m_device->cameraSetRestoreFactorySettingsR();
    });
}
