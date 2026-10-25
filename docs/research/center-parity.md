# OBSBOT Center parity research: Tiny 2

Research date: 25 October 2026. Scope: the **OBSBOT Tiny 2** (not Tiny 2 Lite, Tiny SE, Tiny 4K or Tiny 3) as controlled by **OBSBOT Center** (formerly "OBSBOT WebCam"), plus a survey of how other webcam/PTZ companion apps lay out their controls.

Current versions at the time of writing (from the [Tiny 2 download page](https://obsbot.com/download/obsbot-tiny-2)): OBSBOT Center **2.0.15.186** (23 September 2026, Windows and macOS); Tiny 2 firmware **6.0.13.1** (1 June 2026). The installers are behind signed CDN URLs, so I could not inspect the Center binary itself; everything below comes from documents and reviews.

## Evidence key

Each item carries one or more tags saying where it was confirmed.

| Tag | Meaning |
|---|---|
| **[O]** | Official OBSBOT document: Tiny 2 user manual, OBSBOT news/release notes, download page, product page, FAQ, tutorial article, or the **OBSBOT Center OSC definition sheet** (which lists the commands Center itself accepts). |
| **[S]** | The OBSBOT SDK header shipped in this repo (`sdk/include/dev/dev.hpp`). Vendor source, but it describes what the *device* supports, not what Center's UI shows. |
| **[R]** | Third-party review or hands-on write-up showing or describing the Center UI with a Tiny 2. |
| **[M]** | Measured on the Tiny 2 attached to this desktop with `v4l2-ctl -L` (read-only control listing). |
| **[I]** | Inferred: not stated by any source in this form; reasoning given. |
| **[L]** | Documented for a sibling model (Tiny 2 Lite or Tail 2) using the same Center UI, not for Tiny 2 itself. |
| **[O-Insta360]**, **[O-Elgato]** | Part 2 only: the vendor's own manual or help-centre article for that app. |

Primary sources used repeatedly:

- Tiny 2 user manual v1.1 (EN PDF): <https://resource-cdn.obsbothk.com/download/obsbot-tiny-2/manual/OBSBOT%20Tiny%202%20User%20Manual_EN.pdf>
- OBSBOT Center OSC definition (8 May 2026 revision, XLSX), linked from <https://www.obsbot.com/explore/obsbot-center/osc>: <https://resource-cdn.obsbothk.com/download/obsbot-tail-2/osc/obsbot_center_osc_definition20260508.xlsx>
- Tiny 2 specs: <https://www.obsbot.com/obsbot-tiny-2-4k-webcam/specs>; FAQ: <https://www.obsbot.com/obsbot-tiny-2-4k-webcam/faq>; product page: <https://www.obsbot.com/obsbot-tiny-2-4k-webcam>
- TU Dresden ZIH test of the Tiny 2 (German; lists the Center menu tree from screenshots): <https://tu-dresden.de/zih/vcc/tests/zubehoer-1/obsbot-tiny-2>
- The Gadgeteer review, 23 Aug 2023 (describes the window layout): <https://the-gadgeteer.com/2023/08/23/obsbot-tiny-2-4k-webcam-and-smart-remote-2-review-do-ai-gestures-and-voice-control-make-a-better-experience/>
- PCGamesN review (top button bar, image controls): <https://www.pcgamesn.com/obsbot-tiny-2-webcam-review>

---

## Part 1: OBSBOT Center feature inventory for the Tiny 2

### 1.1 Overall structure of Center

Center has two UI modes since late 2023, both still present in 2026:

- **Pro Mode**: the full UI, "provides all functions of OBSBOT series products" [O] ([Lite Mode guide](https://www.obsbot.com/explore/obsbot-center/obsbot-center-lite-mode)).
- **Lite Mode**: reduced UI exposing "View and Gimbal", "Preset Positions", "Human Tracking" and "Beauty", with a "Preview" button [O]. Redesigned in **v2.0.14.53** (23 July 2026): preview opens automatically at maximum resolution, controls reordered "by usage frequency", plus new "Mobile Device Control" (QR code to a phone remote panel on the same Wi-Fi), "Tailored Image Control (Std. / Pro)", "AI Lock / Unlock" and "Launch Minimized" [O] ([news](https://www.obsbot.com/news/all-new-lite-mode-for-obsbot-center)).
- Switching: Pro → Lite via *Settings > General Settings > Software Mode*; Lite → Pro via *Settings > Switch to Pro Mode* [O].

Pro Mode window layout, as described for the Tiny 2:

| Region | Contents | Evidence |
|---|---|---|
| **Top bar** | Active camera name; buttons for video preview window, sleep/wake (power), physical remote on/off, global hotkeys on/off, virtual camera, OSC on/off, remote access | [R] Gadgeteer, PCGamesN |
| **Tabs** | **Console · Image · Beauty · More** | [R] Gadgeteer; TU Dresden (Konsole / Abbildung / Schönheit / Mehr); igorslab names the same Console / Image / More set for Tail 2 [L] |
| **Sections** | Collapsible sections within tabs (toggle arrows); window has a maximum height on macOS, so the More tab scrolls | [R] Gadgeteer |
| **Settings (gear, top left)** | Global Hotkeys, web/remote control, General Settings (Software Mode, language), logs, beginner guide, software version | [R] Gadgeteer; [O] Lite Mode guide |
| **Video Preview window** | Separate window; the resolution/frame-rate selector (e.g. "4K P30") sits in its lower-left corner and is the only place to change resolution. Since Nov 2024 the Beauty, Virtual Camera, Recording and Audio modules can be embedded in the preview window or opened separately. Double-clicking the preview selects the tracking target (Center 2.0.11.60, May 2025) | [R] Gadgeteer; [O] [Nov 2024 update](https://obsbot.com/news/november-update), [May 2025 update](https://www.obsbot.com/news/2025-may-update) |

"More Settings" in Center covers preview resolution, auto sleep, gesture control, voice control, microphone settings, firmware version management and factory reset; "available settings vary by product" [O] (Lite Mode guide).

### 1.2 Console tab: AI tracking and modes

**AI work modes.** Center's OSC sheet gives the flat list Center uses for Tiny 2 ("For Tiny 2, Tiny 2 Lite, Tiny SE"; Tiny 2 Lite and SE only have 0, 1, 2, 3, 9) [O]:

| OSC value | Mode | SDK equivalent [S] |
|---|---|---|
| 0 | No Tracking | `AiWorkModeNone` |
| 1 | Normal Tracking | `AiWorkModeHuman` + `AiSubModeNormal` |
| 2 | Upper Body | Human + `AiSubModeUpperBody` |
| 3 | Close-up | Human + `AiSubModeCloseUp` |
| 4 | Headless | Human + `AiSubModeHeadHide` |
| 5 | Lower Body | Human + `AiSubModeLowerBody` |
| 6 | Desk Mode | `AiWorkModeDesk` |
| 7 | Whiteboard | `AiWorkModeWhiteBoard` |
| 8 | Hand | `AiWorkModeHand` |
| 9 | Group | `AiWorkModeGroup` |

So Center presents **human tracking with five framings** (normal / upper body / close-up / headless / lower body) and **four other AI modes** (desk, whiteboard, hand, group). TU Dresden's screenshots show "AI-Portraitverfolgung" (human tracking) and a separate "AI Modus" group containing desk, whiteboard, hand and group [R]. XDA (31 Jul 2023) describes the framings as auto-zoom options for "upper or lower body, or a more close-up view" [R] ([XDA](https://www.xda-developers.com/obsbot-tiny-2-review/)).

Mode descriptions from the manual [O]:

- **Human tracking**: "automatically track and ensure that the figure is in the best composition position". Activated by gesture, voice or Center.
- **Group**: "the view will automatically adjust as participants join or leave the image".
- **Hand**: Center only; "automatically detect and track your hand… you can adjust the tracking area as needed". SDK adds a **left/right hand** choice and a per-hand tracking zone (yaw/pitch min/max) [S].
- **Whiteboard**: Center only; "intelligently recognize whiteboards… and enhance the display". The Smart Remote 2 adds a laser-pointer "laser whiteboard" close-up [O] (product page).
- **Desk**: Center only; "the camera will automatically tilt down 30 degrees to shoot the desk from above, and automatically correct the shot".

**Tracking mode (Standard vs Motion).** OSC `SetTrackingMode`: 0 Headroom, 1 Standard, 2 Motion; "Tiny 2/Tiny 2 Lite/Tiny SE only has 1,2" [O]. SDK `AiVerticalTrackType` and status byte `ai_tracker_speed` (0 normal, 2 motion) agree [S]. A Tiny 2 Lite comparison describes the options as "normal tracking and motion tracking presumably for fast motion" [R][L] ([Soltura](https://www.davesoltura.com/insta360-link-2-vs-obsbot-tiny-2-lite-not-sponsored/)). **Headroom mode is not offered on Tiny 2.**

**Tracking speed.** OSC `SetTrackingSpeed` (Slow/Standard/Fast) is "Only for Tiny 3, Tiny 3 Lite" [O]; the SDK's five-step `AiTrackSpeedType` is marked Tail Air [S]. **No separate tracking-speed control was found for Tiny 2**; Standard/Motion is the only speed-related choice.

**Auto Zoom.** The product page shows "AI Tracking with Auto Zoom" with Auto Zoom on/off [O]; SDK `aiSetAiAutoZoomR` is tagged "tiny2 series" [S]. Exact label/placement in Center not confirmed [I].

**"Smart composition".** OBSBOT's term is **"AI Composition Lines"**, added for Tiny 2 and Tiny 2 Lite in July 2024 ("keeps frames well composed during tracking") [O] ([July 2024 update](https://www.obsbot.com/news/obsbot-july-firmware-and-software-updates)). "Smart Composition" is **Insta360's** label for its auto-zoom framing toggle (see Part 2); I found no OBSBOT feature by that name.

**Zone tracking / non-tracking areas.** Manual: "zone tracking can be enabled through the app" [O]. Tiny 2 download page: "you can set a specific zone that the OBSBOT Tiny 2 would stop tracking when you are inside it, and re-start tracking when you get out of it" [O]. Jan 2024 Center notes for Tiny 2: "Improved Zone tracking, added vertical-screen support, added a tracking range setting, and added auto-tracking" [O] ([news](https://www.obsbot.com/news/obsbot-center)). SDK exposes two distinct things for "tiny2 series" [S]:
- **Zone presets** (`aiAddZonePresetR` etc.): named non-tracking areas. The Tiny 2 Lite page says "up to three non-tracking areas" [L].
- **Limited-zone tracking**: yaw min/max (−120…120°), pitch min/max (−90…90°), an initial position, and "auto-select" of a new target. Tiny 2 Lite marketing: "define 4 gimbal angles to limit the Tracking Zone… When the target goes out of the shooting range, [it] will return to the default initial shooting angle" [L].

**Target select / lock.** Center: double-click the preview to select the target (2.0.11.60) [O]; OSC `ToggleAILock` 1 lock / 0 unlock [O]; Lite Mode "AI Lock / Unlock" [O].

### 1.3 Console tab: view, gimbal, zoom

| Control | Options / range | Evidence |
|---|---|---|
| **View (FOV)** | Wide 86° / Medium 78° / Narrow 65° | [O] OSC `SetView`; [L] Lite preset guide ("Wide", "Medium", "Narrow"); [R] PCGamesN; [S] `FovType` |
| **Gimbal joystick** | On-screen virtual joystick / "digital stick" for pan and tilt | [R] PC Gamer via [Yahoo](https://tech.yahoo.com/general/articles/obsbot-tiny-2-review-172922481.html), [Capture Guide](https://www.captureguide.com/obsbot-tiny-2-review/) |
| **Gimbal speed** | Adjustable, with Coarse/Fine adjustment (Center, July 2024) | [O] |
| **Gimbal reset** | "Zero" returns gimbal to centre (in Initial Position setup); OSC `ResetGimbal` | [O] |
| **Pan/tilt range** | Controllable pan ±140°, tilt +30° to −70°; mechanical ±150° / ±90°; max controllable speed 120°/s | [O] manual, specs |
| **Absolute aim (OSC)** | `SetGimMotorDegree`: speed 0–90, pan −129…129°, pitch −59…59° | [O] |
| **Yaw direction reverse** | Mirror the joystick's pan direction | [S] `aiSetGimbalYawDirReverseR` (tiny2 series); [O] Nov 2023 notes add it for Tiny/Tiny 4K only |
| **Zoom** | 1x–4x digital; slider | [O] manual, specs; [R] Capture Guide |
| **Zoom (OSC)** | 0–100 % of range; `SetZoomSpeed` 0–11 (0 = default) explicitly lists Tiny 2 | [O] |
| **UVC ranges** | `zoom_absolute` 0–100; `pan_absolute` ±468000, `tilt_absolute` ±324000 (step 3600 arcsec); `pan_speed` ±80, `tilt_speed` ±120 | [M] |

How Center maps clicks on the preview to the gimbal (drag-to-aim) is **not documented** for Tiny 2. The only preview-click behaviours documented are double-click to select a tracking target [O] and, for Tail 2 in manual focus, left-click no longer triggering autofocus [L].

### 1.4 Presets

- **Count: 3** ("Preset Position 1/2/3"). OSC `TriggerPreset` 0–2 and `GetPresetPositionInfo` returns exists + name for each of the three [O]; voice commands "Position One/Two/Three" [O]; Smart Remote 2 has three preset buttons [R].
- **Stores** (Preset Positions, original): view (FOV) and gimbal/zoom. SDK `PresetPosInfo` holds pitch, yaw, zoom and a name of up to 64 bytes [S].
- **Preset Mode** (Center **2.0.11.60** + Tiny 2 firmware **6.0.11.1**, May 2025): each preset can also store **focus, exposure, white balance and colour saturation**; reached via "More Settings" under each preset [O] ([news](https://www.obsbot.com/news/2025-may-update)). The Tiny 2 Lite version also stores tracking mode, image parameters and Bokeh [L].
- **UI** (documented for Tiny 2 Lite, same Center screen) [L] ([guide](https://www.obsbot.com/explore/obsbot-tiny-2-lite/preset-modes)): "Add" saves the current position as the next preset; a drop-down per preset offers "Rename" and "Delete"; "Update PTZ" overwrites position; "As Initial State" makes it the power-on state; "More Settings" edits the preset's other parameters.
- Presets live **on the camera**, so they survive moving the camera to another computer [O] (FAQ: "the adjustment will be stored in the OBSBOT Tiny 2 camera").
- Voice preset recall needs presets to have been added in Center first [O].

### 1.5 Initial (boot) position and state

Introduced in Center 2.0.6.2 / firmware 6.0.3.1 (Aug 2023) [O] ([news](https://obsbot.com/news/obsbot-webcam-software-august-update)). Procedure [O] ([tutorial](https://www.obsbot.com/explore/obsbot-tiny-2/how-to-set-tiny-2-initial-working-state)):

- *More → Initial Position* (renamed from "Gimbal Initial Position" in a later build [O]).
- Sets **gimbal initial position** (with "Zero" to centre), **zoom initial position**, and **initial working mode** (default standard / no tracking; alternatives include normal tracking, whiteboard, desk, group).
- "Restore" reverts to the previous initial state; "Done" saves.
- On reconnect the camera restores gimbal, zoom and working mode; pressing the reset button restores only gimbal and zoom.
- SDK: `aiSetGimbalBootPosR`, `aiTrgGimbalBootPosR`, `aiRstGimbalBootPosR`, `cameraSetBootModeU(mode, sub_mode)` [S].

### 1.6 Sleep and privacy

| Control | Options | Evidence |
|---|---|---|
| **Manual sleep** | Point the lens straight down; LED off = asleep | [O] manual |
| **Sleep/wake button** | Top bar; OSC `WakeSleep` 0/1 | [R] Gadgeteer; [O] OSC |
| **Device Auto Sleep** | Must be switched on in Center ("To activate Device Auto Sleep feature, you need to turn on the corresponding button in the OBSBOT Center control software"); sleep time configurable. The selectable durations are **not documented** | [O] manual, download page |
| **Auto sleep (SDK)** | `cameraSetSuspendTimeU` seconds; status `auto_sleep_time` (0 = never) | [S] |
| **Sleep background** ("Privacy Protection 2.0") | While asleep, stream shows a custom photo, an OBSBOT image, or black. SDK: image *or video*, up to 3 user slots, optional mirror | [R] TU Dresden; [O] product page; [S] `cameraSetResourceActionU`, `sleep_bg_type`, `bg_img_mirror` |
| **Microphone while asleep** | On/off | [S] `cameraSetMicrophoneDuringSleepU` |
| **Sleep without stream** | Option to disable auto-sleep when no stream is open | [S] `cameraSetDisableSleepWithoutStreamU` |
| **Privacy state** | `DevStatusPrivacy` (no stream fetched) | [S] |
| **Sleep position** | **No configurable sleep position found.** The camera parks pointing down | [I] from manual's manual-sleep description |
| **Firmware behaviour** | 6.0.4.1 (Oct 2023): camera keeps the selected working mode after waking | [O] ([news](https://www.obsbot.com/news/obsbot-tiny-2-receives-new-firmware-update-to-enhance-streaming)) |

### 1.7 Gesture control ("Gesture Control 2.0")

Gestures (manual) [O]:

| Gesture | Effect | LED |
|---|---|---|
| Open palm | Toggle human tracking | Flashes twice, then steady blue (on) / green (off) |
| Pointing (zoom gesture) | Zoom to 2x (default) / cancel | Flashes twice |
| Two-hand "L" held until LED flashes, then hands apart/together | Dynamic zoom | Flashing |

Settings in Center (More tab → Gesture control):

| Setting | Options | Evidence |
|---|---|---|
| Gesture control on/off | Can be disabled in Center | [O] FAQ |
| Gesture zoom factor | 1x–4x (default 2x) | [O] manual; [S] `gesture_zoom_factor` 1.0–4.0 |
| Individual toggles: **target select**, **gesture zoom**, **dynamic zoom**, **dynamic-zoom direction (mirror)** | On/off each | [S] `aiSetGestureCtrlIndividualR` gestures 0–3; `AiStatus.gesture_target/zoom/dynamic_zoom/mirror`. That Center shows each as a separate switch is [I] |

### 1.8 Voice control

Commands (English and Mandarin) [O] (manual):

| Command | Action |
|---|---|
| "Hi, Tiny" | Wake |
| "Sleep, Tiny" | Sleep |
| "Track Me" | Human tracking on |
| "Unlock Me" | Human tracking off |
| "Zoom in Closer" | Zoom to 2x (default) |
| "Zoom out Further" | Back to 1x |
| "Position One/Two/Three" | Recall preset 1/2/3 |

Settings: voice zoom factor 1x–4x [O] (manual); per-command enable bits for each of the seven command groups, language Chinese/English [S] (`AudioCtrlCmdType`, `cameraSetAudioCtrlStateU`; status `voice_ctrl`, `voice_ctrl_language`, `voice_ctrl_zoom` 0–100). TU Dresden lists the voice-control page under "Mehr" [R]. A one-second silence is needed before a command [O] (product page).

### 1.9 Image tab

Grouping seen in Tiny 2 screenshots: **"Focus and Exposure"** and **"White Balance and Image Content"** (TU Dresden: "Fokus und Belichtung", "Weissabgleich und Bildinhalt einstellen") [R]. Reviews add HDR and anti-flicker on the same tab [R] (Capture Guide; [PCGamesN](https://www.pcgamesn.com/obsbot-tiny-2-webcam-review)).

| Control | Options / range | Evidence |
|---|---|---|
| **HDR** | On/off, default off (introduced Center 2.0.5.8 / fw 6.0.2.1, July 2023). SDK warns HDR interacts with 60 fps (`kEvtInfoHDRWith60Fps`) and exposes `hdr_support` per mode | [O] manual, [July 2023 news](https://www.obsbot.com/news/obsbot-july-update-for-obsbot-webcam-and-tiny-2-firmware); [S] |
| **Focus** | Auto / Manual; manual 0–100 | [O] manual, OSC `SetAutoFocus`, `SetManualFocus`; [M] `focus_absolute` 0–100 |
| **Face focus** (face-priority AF) | On/off | [S] `cameraSetFaceFocusR`, `face_auto_focus`. Center label not confirmed [I] |
| **Exposure mode** | Auto / Manual | [O] OSC `SetAutoExposure` |
| **Auto-exposure metering** | Global vs face-compensated ("Face AE") | [R] PCGamesN; [S] `cameraSetFaceAER` |
| **Exposure compensation** | −3.0 to +3.0 EV in ⅓ steps (19 values: −30, −27, −23 … 27, 30 in OSC units) | [O] OSC |
| **Shutter** (manual) | 1/2.5 to 1/6400 s: 6400, 5000, 3200, 2500, 2000, 1600, 1250, 1000, 800, 640, 500, 400, 320, 240, 200, 160, 120, 100, 80, 60, 50, 40, 30, 25, 20, 15, 12.5, 10, 8, 6.25, 5, 4, 3, 2.5 | [O] OSC; manual shutter added in fw 6.0.2.1 |
| **ISO** (manual) | 100–6400 | [O] OSC |
| **Anti-flicker** | Off / 50 Hz / 60 Hz (SDK also has Auto) | [S] `PowerLineFreqType`; [M] UVC `power_line_frequency` Disabled/50/60; [R] PCGamesN "flicker removal" |
| **White balance** | Auto / Manual (Kelvin) | [O] OSC `SetAutoWhiteBalance` |
| **Colour temperature** | 2000–10000 K | [O] OSC ("within the range supported by the corresponding webcam"); [M] UVC 2000–10000, step 100, default 5000 |
| **WB presets** | SDK lists Auto, Daylight, Fluorescent, Tungsten, Flash, Cloudy, Shade and others for Tiny 2. Which presets Center shows for Tiny 2 is **not confirmed**; igorslab's Tail 2 review shows daylight/incandescent/fluorescent [L] | [S] `DevWhiteBalanceType`; [I] |
| **WB shift (B–A, G–M) and R/B gain** | **Not for Tiny 2** (OSC: Tiny 3, Tiny 3 Lite, Meet Flip only) | [O] |
| **Brightness / contrast / saturation / sharpness / hue** | Sliders. UVC: brightness 0–100, others 1–100, default 50 | [R] Capture Guide ("contrast, saturation, sharpness, and hue"); [S] `cameraSetImage*R`; [M] |
| **Backlight compensation** | UVC 0–18 (default 9). Not mentioned in any Center source | [M]; Center exposure [I] |
| **Gain** | UVC 1–64. Not mentioned in any Center source | [M] |
| **Mirror (horizontal flip)** | On/off | [O] OSC `SetMirror`; [S] `cameraSetImageFlipHorizonU` |
| **Orientation** | Landscape, portrait, downward, upside-down, detected automatically from mounting; portrait switch restarts the device | [O] manual; [S] `cameraSetVerticalModeU` |
| **Low light** | Automatic dual-native-ISO switching; **no user control** found | [O] product page |
| **Style / colour presets** | `DevImageStyle` (Standard, Text, Landscape…) is **Tail Air only**. For Tiny 2 the only "looks" are Beauty-tab colour filters | [S]; [R] |
| **Lite Mode image levels** | "Tailored Image Control (Std. / Pro)" | [O] |

### 1.10 Beauty tab (host-side processing)

All Beauty effects are rendered on the PC and reach other apps only through the **OBSBOT Virtual Camera** [R] (XDA, Capture Guide, TU Dresden). 4K beauty and background effects need an NVIDIA RTX 2060+ (Windows) or Apple M1+ (macOS) [O] (specs).

- Beauty presets "Natural", "Classic", "Men", each with face / body / make-up / basics sub-settings [R] (TU Dresden, PCGamesN, XDA).
- Retouch: face features (eyes, nose, mouth, cheekbones), body shape (leggy, legs, waist, shoulder, head) [R].
- Filters: black-and-white, clear, crystal, sheer [R] (PCGamesN).
- Background blur with adjustable level; replacement is **not** supported on Tiny 2 [O] (FAQ).

### 1.11 More tab and device settings

| Setting | Options | Evidence |
|---|---|---|
| **Microphone noise reduction** | Off (default) / weak / medium / strong | [R] TU Dresden; [S] `noise_cancellation` |
| **Audio auto gain** | On/off | [O] specs; [S] `cameraSetAudioAutoGainU` |
| **Pickup distance** | Near / standard / far | [O] Jan 2024 notes ("radio distance"); [S] `audio_opt.distance` |
| **Disable microphone (UAC)** | On/off | [O] Jan 2024 notes; [S] `audio_opt.uac_enabled` |
| **LED brightness** | Off, 1–3 | [O] Nov 2023 notes; [S] `led_brightness_level` |
| **Firmware update** | Center prompts on connect | [O] manual |
| **Factory reset** | Yes | [O] Lite Mode guide; [S] `cameraSetRestoreFactorySettingsR` |
| **Resolution / frame rate** | Video Preview window lower-left. Device modes: MJPEG and H.264 4K ≤30, 1080p/720p ≤60; YUV 4K 15, 1080p ≤30, 720p ≤30 | [R] Gadgeteer; [O] specs |
| **Switch 2 Mode** | For Nintendo Switch 2; max 1080p60, no high-fps recording (Center 2.0.13.26, fw 6.0.12.2) | [O] [Nov 2025 news](https://www.obsbot.com/news/center-november-update) |
| **"Camera Setting"** | Added to Center for Tiny 2 (contents not described) | [O] download page notes |
| **Recording / snapshot** | PC recording, snapshot, recording format/bitrate, hardware vs software encoding, ISO recording (raw stream) | [O] Nov 2023, Nov 2024, May 2025 notes; OSC `SetPCRecording`, `PCSnapshot` |
| **Virtual camera** | Top-bar toggle; enabling it disables the "OBSBOT Tiny 2 StreamCamera" device | [R] Gadgeteer, TU Dresden |
| **Global Hotkeys** | Customisable shortcuts for common operations; settings page under the gear; on/off in the top bar. **Default key list not published.** "Smart Hotkeys" as a name was **not found**; OBSBOT calls it "Global Hotkey" | [O] product page; [R] Gadgeteer, Capture Guide |
| **Remote control** | Smart Remote 2 (USB dongle, relayed by Center): pan/tilt, zoom, 3 presets, Track / Close Up / Hand / Desk toggles, laser pointer, page up/down | [R] Gadgeteer, PCGamesN |
| **OSC / Stream Deck / SDK / RTC** | OSC server in Center (must be running); Stream Deck plugin; mobile/RTC remote | [O] OSC page, product page, Center explore page |

### 1.12 Relevant Center and Tiny 2 firmware history

| Date | Version | Tiny 2–relevant change | Source |
|---|---|---|---|
| 18 Jul 2023 | Center 2.0.5.8, fw 6.0.2.1 | HDR; recording resolution; shutter adjustment | [O] |
| 21 Aug 2023 | Center 2.0.6.2, fw 6.0.3.1 | Initial working mode at power-on | [O] |
| 16 Oct 2023 | fw 6.0.4.1 | Audio NR; portrait tracking; keeps mode after wake | [O] |
| 15 Nov 2023 | Center (no version) | Adjustable LED brightness | [O] |
| 2 Jan 2024 | Center (rename from WebCam) | Zone tracking improvements, tracking range, mic distance / disable | [O] |
| 25 Jul 2024 | Center | Gimbal speed with Coarse/Fine; AI Composition Lines | [O] |
| 5 Nov 2024 | Center 2.0.10.18 | Preview-embedded modules; HW/SW encoding | [O] |
| 22 May 2025 | Center 2.0.11.60, fw 6.0.11.1 | Preset Mode (per-preset focus/exposure/WB/saturation); double-click preview selects target | [O] |
| 26 Nov 2025 | Center 2.0.13.26, fw 6.0.12.2 | Switch 2 Mode | [O] |
| 1 Jun 2026 | fw 6.0.13.1 | "Fixed known bugs" | [O] |
| 23 Jul 2026 | Center 2.0.14.53 | Redesigned Lite Mode; global hotkeys switch PiP | [O] |
| 23 Sep 2026 | Center 2.0.15.186 | "Optimized known issues" | [O] |

### 1.13 Items asked about but not found

- A Tiny 2 **tracking-speed** slider (only Standard/Motion exists).
- **"Smart composition"** under that name (OBSBOT's is "AI Composition Lines").
- Configurable **sleep position**.
- The list of **auto-sleep durations**.
- The **default global-hotkey bindings**.
- Center-side **drag-on-preview** gimbal control for Tiny 2.
- Which **white-balance presets** Center lists for Tiny 2.
- Reddit threads: reddit.com was not reachable from my search tooling, so no Reddit material is included.

---

## Part 2: Webcam control software with well-regarded UI

### 2.1 Which apps are praised

| App | Reception | Evidence |
|---|---|---|
| **Insta360 Link Controller** (Link, Link 2/2C/2 Pro) | Most consistently praised PTZ/AI-webcam app: "well-designed companion app" (Tom's Guide, Link 2), "user-friendly interface ensures that even novice users can get the most out of their camera" (CGMagazine, Feb 2025), "polished software program" (T3). In a head-to-head with a Tiny 2 Lite, it was judged faster and more responsive than OBSBOT Center. One dissent: Tom's Hardware Italia called it not very intuitive at first use | [Tom's Guide](https://tomsguide.com/computing/peripherals/insta360-link-2-webcam-review), [CGMagazine](https://www.cgmagonline.com/review/hardware/insta360-link-2-webcam-review), [T3](https://www.t3.com/tech/webcams/insta360-link-2-review), [Soltura](https://www.davesoltura.com/insta360-link-2-vs-obsbot-tiny-2-lite-not-sponsored/), [Tom's HW Italia](https://www.tomshw.it/hardware/insta360-link-2-pro-webcam-professionale-con-gimbal) |
| **Elgato Camera Hub** (Facecam family) | Praised for DSLR-style image controls and simplicity: "the easiest-to-use software I've encountered for a webcam" (PCWorld); "DSLR-like fine-tuning" (Expert Reviews). Criticised for a single on-camera profile and (early) missing hue/tint | [PCWorld](https://www.pcworld.com/article/3624639/elgato-facecam-review.html), [Expert Reviews](https://www.expertreviews.co.uk/technology/accessories/elgato-facecam-review) |
| **OBSBOT Center** | Mixed to positive: "excellent utility software and configurability" (PCWorld), "comprehensive software" (T3), "reasonably fast and compact with no extra adverts" (PCGamesN); but "becomes cumbersome… key features are spread across tabs and require scrolling" (Gadgeteer); "the Obsbot app isn't that optimized… tends to lag more" (Soltura) | [PCWorld](https://www.pcworld.com/article/2013947/obsbot-tiny-2-4k-webcam-review.html), [T3](https://www.t3.com/reviews/obsbot-tiny-2-review-webcam), PCGamesN, Gadgeteer |
| **Logi Tune / Options+ / G Hub** | Reviews praise "lots of control with Logi Tune app" (Tom's Guide, MX Brio) but say little about layout. Logitech splits webcam control across three apps and warns that settings conflict between them (e.g. RightSight in Tune makes zoom changes in G Hub temporary) | [Tom's Guide](https://www.tomsguide.com/computing/peripherals/logitech-mx-brio-705-for-business-webcam-review), [Logitech support](https://support.logi.com/hc/en-ca/articles/21848986694935) |
| **Razer Synapse** (Kiyo Pro / Pro Ultra) | Negative example: Tom's Hardware called Synapse 3 "one of the worst software programs we've ever seen", with settings that "may take several tries to save"; CGMagazine called it buggy; no macOS version | [Tom's Hardware](https://www.tomshardware.com/reviews/razer-kiyo-pro-ultra-webcam), [CGMagazine](https://www.cgmagonline.com/review/hardware/razer-kiyo-pro-ultra-webcam) |
| **Opal Composer** (C1, Tadpole) | Reviewers mention it positively but describe little of its UI; the main complaint is that it is macOS-only | [Serious Insights](https://seriousinsights.net/opal-tadpole-review) |

The two best-documented and best-regarded designs are **Insta360 Link Controller** (the closest analogue to OBSBOT Center: gimbal, AI modes, presets) and **Elgato Camera Hub** (image controls and on-camera persistence). OBSBOT Center itself is described below as the baseline.

### 2.2 Insta360 Link Controller

**Main layout**
- A compact **control bar** docks at the bottom of the screen after install; a button at its left opens the full window with a live preview [R] ([GIGAZINE](https://gigazine.net/gsc_news/en/20230321-insta360-link-controller), Mar 2023).
- Full window: **preview on the left**, a **right-hand panel with three tabs**: *Gimbal Control*, *Image Settings*, *More Settings* [R] ([9to5Toys](https://9to5toys.com/2022/08/02/insta360-link-review/); CGMagazine Link review).
- **Mode buttons along the bottom** of the preview: Tracking, Whiteboard, Overhead, DeskView [R] (GIGAZINE).

**PTZ**
- **Drag inside the preview** to pan/tilt, **mouse wheel** to zoom, or use direction buttons and a zoom bar on the right [R] (GIGAZINE); the Link 2 manual also documents "hold the left mouse button and drag the preview, or use the mouse wheel" and a "View control panel" [O-Insta360] ([manual](https://onlinemanual.insta360.com/link2/en-us/operating-tutorials/track/ai-track)).
- Gimbal reset from the Controller, the phone remote, or a double-tap of the touch key [O-Insta360] ([gimbal reset](https://onlinemanual.insta360.com/link2/en-us/operating-tutorials/step/gimbal-reset)).
- Phone/tablet remote via a QR code shown in the desktop app [O-Insta360] ([blog](https://insta360.com/blog/news/new-insta360-link-update-remote-control.html)).

**AI tracking**
- An **"AI Tracking" button in the bottom bar** toggles tracking; a **"^" next to it opens a pull-up menu** to pick *Single Tracking* or *Group Tracking* [O-Insta360].
- **"Smart Composition"** (reached from AI Tracking) turns on auto-zoom framing; framing presets head / upper body / full body [O-Insta360]; [R] Tom's Guide.
- **Tracking speed**: slow / ordinary / quick [R] (Soltura).
- **Pause-Track Area** (tracking pauses and zooms to the area when someone enters; up to six on Link 2) and **Tracking Area** (tracking only inside the zone) [O-Insta360]; [R] CGMagazine.
- Gesture enable/disable per gesture, with an on-screen cheat sheet of what each gesture does, in *More Settings* [R] (CGMagazine Link review, 9to5Toys).

**Presets**
- Saved in the *Gimbal Control* tab; recalled by **mouse click or an assigned hotkey** [R] (CGMagazine, 9to5Toys).
- With the Controller running, Stream Deck presets can include image and effect settings; without it, only hardware state (gimbal, zoom, AI mode) [O-Insta360] ([Stream Deck page](https://onlinemanual.insta360.com/link2pro/en-us/tutorial/ecosystem-integration/elgato-streamdeck)).

**Image settings**
- *Image Settings* tab: brightness, contrast, saturation, sharpness; auto/manual exposure with shutter and ISO; **exposure curve** (highlights/shadows); auto/manual white balance [R] (CGMagazine, PCMag via search).
- A **"Save as preset"** checkbox reapplies image settings at next start-up [R] (GIGAZINE).
- Less-used toggles (anti-flicker, HDR, mirror, autofocus, AI zoom, streamer/portrait mode, firmware) sit in *More Settings* [R] (9to5Toys).

**What reviewers praised and criticised**
- Praised: tracking "incredibly snappy" vs OBSBOT; more natural software bokeh; stability ("more dependable and crash-free over two months of daily use", in a Meet 2 vs Link 2C comparison); novice-friendly [R] (Soltura ×2, CGMagazine).
- Criticised: not intuitive at first use (Tom's HW Italia); a learning curve shared by AI webcams (CGMagazine Link review); bokeh not in 4K, make-up filters ≤1080p [R].

### 2.3 Elgato Camera Hub

**Main layout** ([official settings overview](https://help.elgato.com/hc/en-us/articles/4405055113357), updated Oct 2026)
- Preview plus a **single scrolling settings column divided into titled sections**: *Effects · Device · Zoom · Picture · Exposure · White Balance · Processing* [O-Elgato].
- **Compact Mode** (icon at top left) shrinks the window to the settings only, so image controls can be adjusted while another app (OBS, Zoom) owns the camera [O-Elgato] ([Compact Mode](https://help.elgato.com/hc/en-us/articles/4405171109773)).

**PTZ** (Facecam Pro, digital) ([PTZ article](https://help.elgato.com/hc/en-us/articles/10290605046669))
- *Zoom / FOV* slider (shown as a percentage, with FOV in degrees as the secondary value while dragging, from Camera Hub 1.5) [O-Elgato].
- Once zoomed in, **drag a frame-rectangle over the preview** to choose the crop; arrow buttons for pan/tilt [O-Elgato].

**Presets**
- **Four frame presets, A–D**: **press and hold** a letter to save (a save icon appears), click to recall; presets also exposed to Stream Deck [O-Elgato].
- **"Save" button in the Device section** writes all current settings to the camera's flash; it is greyed out when nothing has changed and shows a check mark after saving [O-Elgato] ([save article](https://help.elgato.com/hc/en-us/articles/25051865823117)). Only one on-camera profile exists, which TechPowerUp criticised [R].

**Image settings and progressive disclosure**
- *Exposure*: Auto or Manual. Auto shows **metering** (centre-weighted / average) and **compensation**; Manual shows **shutter** and **ISO**, with a live **ISO readout** at top right of the section, and in-app guidance (shutter of 1/64 or faster keeps 60 fps; stay under ISO 1000) [O-Elgato].
- In automatic mode, the current shutter and white-balance values are displayed (Camera Hub 1.1 release notes, quoted in a search result; I did not open the page) [O-Elgato].
- *White Balance*: Auto / Manual, with tint added later; auto-WB can be locked at any time (1.9.1) [O-Elgato].
- *Processing*: noise reduction, anti-flicker 50/60 Hz, with a note that anti-flicker applies only in auto exposure [O-Elgato].
- LUTs with an intensity slider and hover previews (2.0–2.1) [O-Elgato].

**What reviewers praised and criticised**
- Praised: easiest-to-use webcam software; "mimics DSLR/mirrorless camera controls"; works while another program uses the feed; smooth, gradual slider response [R] (PCWorld, TechPowerUp via search).
- Criticised: single on-camera profile overwrites the default (TechPowerUp); missing hue/colour-temperature control at launch (PCWorld) [R].

### 2.4 OBSBOT Center (baseline, for comparison)

- **Layout**: top icon bar plus four tabs (Console / Image / Beauty / More) with collapsible sections; preview is a separate window; settings behind a gear. Lite Mode offers a second, reduced layout with preview-first design [R][O] (see 1.1).
- **PTZ**: on-screen joystick, zoom slider, FOV buttons (Wide/Medium/Narrow), gimbal speed coarse/fine [R][O]. No documented drag-on-preview aiming.
- **AI modes**: human tracking with framing sub-modes, plus desk / whiteboard / hand / group as separate mode buttons; tracking mode Standard/Motion [R][O].
- **Presets**: three, with Add / Rename / Delete / Update PTZ / As Initial State / More Settings [L].
- **Image settings**: grouped Focus & Exposure and White Balance & Image Content, with auto/manual switches per group; Lite Mode adds a Std./Pro level [R][O].
- **Praised**: depth of configuration, fast and compact, no adverts [R] (PCWorld, T3, PCGamesN).
- **Criticised**: features spread across tabs needing scrolling; no customisable tab; window cannot be freely resized; lag compared with Insta360's app; beauty defaults and "Beauty" naming; heavy CPU/GPU load for effects [R] (Gadgeteer, Soltura, PCGamesN, TU Dresden).

### 2.5 Recurring patterns across the well-reviewed apps

Observed in the sources above, not prescriptions:

1. **Direct manipulation on the preview** for framing: Insta360 (drag to pan/tilt, wheel to zoom) and Elgato (drag the crop rectangle). OBSBOT relies on a separate joystick.
2. **Mode switching sits next to the preview**, not inside a settings tab: Insta360's bottom bar with a pull-up for sub-modes (Single/Group, Smart Composition).
3. **Presets as a small fixed row of slots** with an explicit save gesture: Elgato's press-and-hold A–D; Insta360's click or hotkey recall; OBSBOT's three slots with a menu.
4. **Auto/manual switches that change which controls are shown**: Elgato's exposure section (metering + compensation in auto; shutter + ISO in manual) with live readouts of what auto chose.
5. **A compact or "lite" mode** for use while another app owns the stream: Elgato Compact Mode, OBSBOT Lite Mode, Insta360's docked control bar.
6. **Explicit "save to camera"** with dirty-state feedback: Elgato's Save button greys out when nothing changed.
7. **Reviewers penalise scrolling and tab-hopping** for frequently used controls (Gadgeteer on OBSBOT) and **unreliable saves** (Tom's Hardware on Razer Synapse).
