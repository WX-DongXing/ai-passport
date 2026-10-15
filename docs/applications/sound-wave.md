<p align="right"><a href="sound-wave.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Sound Wave

Sound Wave 1.0.0 is an offline microphone visualizer for the AI Passport's
240 × 320 display. `PROJECT_VER` in the root CMake configuration sets the
embedded application version.
Startup shows a dedicated Chinese title and moving curve, then opens the
live sine view automatically after at least 1.4 seconds. The baseline test menu
is not part of this application's startup or navigation.

## Controls

| Button | Action |
| --- | --- |
| UP click | Cycle sine, mirror, raw PCM waveform |
| DOWN click | Toggle portrait/landscape orientation |
| OK click | Pause/resume the curve and readings |
| OK long press | Enter full screen in landscape / return to ordinary portrait |

Full screen shows only the wave and small Hz/dBFS readings. Battery, title,
status, and hints are hidden. Shape, orientation, and pause controls remain available.
The microphone continues to be
drained during pause so resume shows current audio, not queued old samples.
There are no confirmation dialogs or calibration screens. The compact on-device
labels use verified 14/22 px Chinese font subsets.

## Readings and visual mapping

Capture uses fixed BSP input gain, 16 kHz mono 16-bit PCM, and 512 samples per
analysis block (32 ms). A Hann-windowed FFT estimates the strongest spectral
peak with a bin interval of 31.25 Hz and sub-bin interpolation. This is not a
fundamental-frequency detector: mixed sounds can produce different peaks.
Silence or a spectrum without a pronounced peak displays `-- Hz`. The lowest
reported peak is about 47 Hz after interpolation; the upper analysis boundary is below 8 kHz.

dBFS is `20 log10(RMS / 32768)` after DC removal, floored at -96 dBFS.
A full-scale sine reads approximately -3.01 dBFS. This is digital input level,
not calibrated acoustic SPL or dB(A). No ambient-noise accuracy is claimed.
Near-full-scale PCM displays CLIP in ordinary mode. Input failure displays
MIC ERR and unavailable readings, and capture retries once per second.

Sine is a stylized single curve whose visible cycle count follows the dominant
frequency; it is not a literal oscilloscope trace. Mirror draws absolute local
RMS symmetrically. Raw waveform retains the signed peak PCM sample in each
display bucket. Neither uses per-frame normalization. Fixed 16x visual gain
makes height proportional to input amplitude before screen clipping; an input
increase of 6.02 dB doubles height. The ordinary readings use the same frame as
the curve. No automatic visual gain or calibration dialog is applied.

Each style has its own solid background: tilted purple orbital paths with a lilac
sine, mauve symmetric hourglass paths with a pink mirror, and purple perspective mesh
with a coral raw waveform. There are no gradients, layered waves, or blur.
Direction/style controls leave measured Hz/dBFS unchanged.
Display updates wait 33 ms after rendering; real frame rate, latency, microphone
noise floor, battery cost, and frequency accuracy require board acceptance.

The application uses one permanent LVGL screen, a latest-frame queue, a bounded
button queue, and one audio worker. It does not record to storage, play audio,
enable networking, or allocate a full-screen bitmap. Only the UI owner accesses
LVGL and holds the BSP lock. Firmware uses the repository's default partitions.
The plot is rendered as RGB565 strips through LVGL 9.5 custom decoder callbacks,
with one aligned 5120-byte buffer and one software draw unit. It does not
allocate a complete image or an offscreen ARGB layer. Host tests compare whole
and strip rendering, verify pixel-level amplitude ratios and clipped writes.

## Validation and device acceptance

Run the shared complete gate with ESP-IDF 5.5.3 activated:

```bash
./tools/validate.sh
```

Host tests exercise FFT tones, amplitude ratios, DC rejection, clipping,
low-level input, controls, symmetry, and curve bounds. On the device, check
startup, all three shapes, single and long OK press separation, pause/resume,
full-screen minimal readings, orientation, Chinese glyphs, quiet-room behavior, microphone response to a
reference tone, clipping, and sustained concurrent capture/display stability.
Use the matching archived firmware and ELF; follow the
[flashing/data policy](../development/engineering/firmware-layout.md#flashing-and-stored-data).

Capture explicitly yields after each analysis; display updates never spin to
catch up missed deadlines. Curve generation uses trig recurrence instead of
per-point trig calls. Capture and display logs report timing and free heap for
bounded device acceptance.

The release uses direct latest-frame display and fixed phase animation speed.
There is no smoothing filter, speed control, settings panel, or long-UP action.
The orbital sine and hourglass mirror backgrounds remain selected.

The restored application previously passed the complete host/build/archive gate
and a 60-second device run without watchdog, panic, missing-glyph, or
microphone-unavailable errors. Reported free heap remained 212000 bytes, with
approximately 13.1–13.2 refreshes/s in ordinary portrait sine. Logs recorded
full-screen and style transitions. This validates the restored behavior before
the version metadata update; the exact 1.0.0 artifact still requires on-device
acceptance. Physical appearance, acoustic accuracy, all controls, and extended
stability remain board/instrument acceptance items. Firmware binaries and local
serial logs are not committed.
