#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QMap>
#include <memory>
#include <functional>
#include <vector>
#include <dev/devs.hpp>
#include "Config.h"
#include "V4l2Backend.h"

/**
 * @brief Handles all camera communication and state management
 *
 * This class encapsulates the OBSBOT SDK and provides a clean Qt-friendly
 * interface for controlling the camera.
 */
class CameraController : public QObject
{
    Q_OBJECT

public:
    struct CameraInfo {
        QString name;
        QString serialNumber;
        QString version;
        int productType;
        bool connected;
    };

    struct CameraState {
        // Tracking
        bool autoFramingEnabled;
        int aiMode;
        int aiSubMode;
        bool autoZoomEnabled;
        int trackingMode;    // 0 standard, 1 headroom, 2 motion
        bool audioAutoGainEnabled;

        // PTZ
        double pan;
        double tilt;
        double zoom;

        // Image settings
        bool hdrEnabled;
        int fovMode;
        bool faceAEEnabled;
        bool faceFocusEnabled;
        bool autoFocusEnabled;
        int manualFocusValue;  // 0-100, motor position when in manual focus

        // Image controls
        bool brightnessAuto; // Auto mode for brightness
        int brightness;      // 0-255
        bool contrastAuto;   // Auto mode for contrast
        int contrast;        // 0-255
        bool saturationAuto; // Auto mode for saturation
        int saturation;      // 0-255
        int whiteBalance;    // 0=Auto, 1=Daylight, etc.
        int whiteBalanceKelvin; // Manual Kelvin value when white balance is manual

        // Status
        int zoomRatio;
        int devStatus;

        // Tiny 2 device state, decoded from the status block
        int autoSleepSeconds;   // 0 = never
        bool micDuringSleep;
        int voiceCommands;      // bit per Device::AudioCtrlCmdType (HiTiny..Preset)
        int voiceLanguage;      // Device::AudioCtrlLanguageType
        double voiceZoom;       // 1.0 - 4.0
        int antiFlicker;        // Device::PowerLineFreqType
        bool mirrored;
        int ledLevel;           // 0 off, 1-3 (read-only: no SDK setter for the Tiny 2)
        bool exposureAuto;
        int shutter;            // Device::DevShutterTimeType
        int sharpness;
        int hue;
    };

    struct GimbalAngles {
        double yawDeg = 0.0;    // positive = camera turns to its left (view moves left)
        double pitchDeg = 0.0;  // positive = looking down
        bool valid = false;
    };

    struct CameraPreset {
        int id = -1;
        QString name;
        double yawDeg = 0.0;
        double pitchDeg = 0.0;
        double zoom = 1.0;
    };

    struct GestureSettings {
        bool targetSelect = false;
        bool zoom = false;
        bool dynamicZoom = false;
        bool dynamicZoomMirror = false;
        double zoomFactor = 2.0;
        bool valid = false;
    };

    struct ZoneLimits {
        double yawMinDeg = -100.0;
        double yawMaxDeg = 100.0;
        double pitchMinDeg = -90.0;
        double pitchMaxDeg = 90.0;
        bool autoSelect = false;
        bool valid = false;
    };

    static constexpr int kMaxPresets = 3;

    struct ParamRange {
        int min = 0;
        int max = 0;
        int step = 1;
        int defaultValue = 0;
        bool valid = false;
    };

    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    // Connection
    bool isConnected() const { return m_connected; }
    bool isV4l2Only() const { return m_v4l2Only; }
    CameraInfo getCameraInfo() const { return m_cameraInfo; }
    void connectToCamera();
    void connectToCamera(const QString &devicePath);
    void disconnectFromCamera();
    QString getVideoDevicePath() const;
    QMap<QString, QString> getSerialsByDevicePath() const;

    // State
    CameraState getCurrentState();
    bool hasTiny2Capabilities() const;

    // Tracking controls
    bool enableAutoFraming(bool enabled);
    bool setAiMode(int mode, int subMode);
    bool setAutoZoom(bool enabled);
    bool setTrackingMode(int mode);
    bool setAudioAutoGain(bool enabled);

    // PTZ controls
    bool setPanTilt(double pan, double tilt);
    bool adjustPan(double delta);
    bool adjustTilt(double delta);
    bool setZoom(double zoom);
    bool centerView();
    double maxZoom() const;  // Highest optical zoom factor for the connected model

    // Camera settings
    bool setHDR(bool enabled);
    bool setFOV(int fovMode);  // 0=Wide, 1=Medium, 2=Narrow
    bool setFaceAE(bool enabled);
    bool setFaceFocus(bool enabled);
    bool setFocusAbsolute(int position, bool autoFocus);

    // Image controls
    void setBrightnessAuto(bool enabled) { m_currentState.brightnessAuto = enabled; }
    bool setBrightness(int value);  // 0-255
    void setContrastAuto(bool enabled) { m_currentState.contrastAuto = enabled; }
    bool setContrast(int value);    // 0-255
    void setSaturationAuto(bool enabled) { m_currentState.saturationAuto = enabled; }
    bool setSaturation(int value);  // 0-255
    bool setWhiteBalance(int mode); // 0=Auto, 1=Daylight, etc.
    bool setWhiteBalanceManual(int kelvin);

    // Tiny 2 gimbal (degrees, read from the motors rather than the UVC cache)
    GimbalAngles readGimbalAngles();
    bool setGimbalAngles(double yawDeg, double pitchDeg);
    bool setGimbalSpeed(double yawDegPerSec, double pitchDegPerSec);
    bool stopGimbal();  // retries: a dropped stop leaves the gimbal turning to its end stop
    bool resetGimbal();

    // Presets stored on the camera
    QList<CameraPreset> readPresets();
    bool recallPreset(int id);
    bool savePreset(int id, const QString &name);  // current position and zoom
    bool renamePreset(int id, const QString &name);
    bool deletePreset(int id);

    // Power-on position and AI mode
    CameraPreset readStartupPosition();
    bool setStartupFromCurrent(int aiMode, int aiSubMode);
    bool resetStartup();

    // Sleep
    bool setAwake(bool awake);
    bool setAutoSleep(int seconds);
    bool setMicDuringSleep(bool enabled);

    // Gestures and voice
    GestureSettings readGestures();
    bool setGesture(int gesture, bool enabled);  // 0 target, 1 zoom, 2 dynamic zoom, 3 dynamic zoom mirror
    bool setVoiceCommand(int command, bool enabled);
    bool setVoiceLanguage(int language);
    bool setVoiceZoom(double zoom);

    // Image controls with no V4L2 equivalent in the old UI
    bool setSharpness(int value);
    bool setHue(int value);
    bool setAntiFlicker(int mode);
    bool setExposure(bool autoExposure, int shutter);
    bool setMirror(bool mirrored);

    // Zone tracking
    ZoneLimits readZoneLimits();
    bool setZoneLimits(const ZoneLimits &limits);
    bool setZoneTracking(bool enabled);
    bool setZoneStartFromCurrent();
    bool recallZoneStart();

    bool factoryReset();
    bool isTiny2() const;

    // Configuration
    bool loadConfig(std::vector<Config::ValidationError> &errors);
    bool saveConfig();
    void applyConfigToCamera();  // Apply loaded config settings to camera
    void applyCurrentStateToCamera(const CameraState &uiState);  // Apply UI state to camera
    Config& getConfig() { return m_config; }

    // Settling state
    bool isSettling() const { return m_settlingTimer && m_settlingTimer->isActive(); }
    void beginSettling(int durationMs = 2000);  // Start settling period

    // Ranges
    ParamRange getBrightnessRange() const { return m_brightnessRange; }
    ParamRange getContrastRange() const { return m_contrastRange; }
    ParamRange getSaturationRange() const { return m_saturationRange; }
    ParamRange getWhiteBalanceKelvinRange() const { return m_whiteBalanceKelvinRange; }
    const std::vector<int>& getSupportedWhiteBalanceTypes() const { return m_supportedWhiteBalanceTypes; }

signals:
    void cameraConnected(const CameraInfo &info);
    void cameraDisconnected();
    void stateChanged(const CameraState &state);
    void commandFailed(const QString &description, int errorCode);
    void configLoaded();  // Emitted after config is successfully loaded

private:
    std::shared_ptr<Device> m_device;
    bool m_connected;
    QString m_selectedDevicePath;
    bool m_v4l2Only = false;
    V4l2Backend m_v4l2;
    QTimer *m_v4l2ScanTimer = nullptr;
    QString m_v4l2DevicePath;
    QString m_v4l2CardName;
    CameraInfo m_cameraInfo;
    CameraState m_currentState;
    CameraState m_cachedState;  // Cache intended state during settling
    Config m_config;
    QTimer *m_settlingTimer;  // Timer for settling period after config apply
    ParamRange m_brightnessRange;
    ParamRange m_contrastRange;
    ParamRange m_saturationRange;
    ParamRange m_whiteBalanceKelvinRange;
    std::vector<int> m_supportedWhiteBalanceTypes;
    int m_lastRequestedWhiteBalance;
    bool m_whiteBalanceFallbackActive;
    int m_fallbackWhiteBalanceMode;
    bool isTiny2Family() const;
    void tryV4l2Fallback();
    void connectV4l2(const std::string &devicePath);
    void refreshV4l2ControlRanges();
    void updateV4l2State();

    // Helper
    bool executeCommand(const QString &description, std::function<int32_t()> command);
    void updateState();
    void readGimbalPosition();
    void saveCurrentStateToConfig();  // Update config with current camera state
    void refreshControlRanges();
    void resetControlRanges();
    int clampToRange(int value, const ParamRange &range, int fallbackMin, int fallbackMax) const;
    int whiteBalancePresetToKelvin(int mode) const;
    bool applyManualWhiteBalance(int kelvin, int displayMode);
    bool isWhiteBalanceTypeSupported(int mode) const;
};

#endif // CAMERACONTROLLER_H
