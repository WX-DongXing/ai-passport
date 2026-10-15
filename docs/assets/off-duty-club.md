**English** · [简体中文](off-duty-club.zh_CN.md)

# Off Duty Club

This application replaces the hardware-test menu with a 240 × 320 companion.
The display has eight moods, five nonrepeating break scenes, daily progress,
an animated final-minute launch countdown, and a celebration page. Audio is
synthesized locally and honors the volume setting, including mute.

The home companion breathes, blinks, looks around and occasionally waves; first-run pages include the same living pet.

## First-run home

The home guides new users through network or phone time synchronization and shift setup.
OK opens the next task; UP opens network settings and DOWN opens shift settings.
Saving either shift time confirms the displayed start/end pair and persists a
completion flag. Saving brightness alone does not complete shift setup.
Previously saved preferences without this flag require one shift confirmation.
Disconnected operation with a valid clock anchor and confirmed shift keeps the
normal home; an unsynchronized reboot returns to connection/time guidance.

## Controls and settings

On the work screen, UP changes mood, a short DOWN press starts a three-second
break scene on release, and OK opens Settings. A press of at least 0.5 seconds
starts the hold flow and never triggers a break scene when released.
Hold DOWN continuously for five seconds
to enter a full-screen 5–4–3–2–1 countdown and go straight to early celebration;
releasing early cancels. On early celebration, UP/DOWN releases confetti, short OK opens the
off-duty home, and long OK opens Settings. This home shows elapsed free time and a relaxed companion;
DOWN starts a full-screen pet relaxation scene and OK opens Settings. UP opens the continue-work/overtime choice. Resuming uses
the current calibrated time, not the moment the break began. Early exit is a
temporary state in RAM and does not change or persist the scheduled end time.
The regular final-minute launch is suppressed while already in early exit.

Settings displays the application version and contains start/end/waiting times, brightness, volume, one six-choice screen-off selector,
automatic break scenes, Wi-Fi setup,
manual synchronization, synchronization status, Forget Network, and Return.
UP/DOWN selects a row. OK enters editing; UP/DOWN adjusts and OK saves.
Shift and waiting times change in 15-minute steps, brightness and volume by five percent.
Holding OK cancels an unfinished edit and returns. Forget Network requires
another OK; UP/DOWN cancels the confirmation. Existing unrelated NVS data is
preserved. Start and end cannot be equal; overnight shifts are supported.

Default hours are 09:00–18:00 China time, brightness 75%, volume 50%, screen-off
disabled, automatic
breaks enabled with a fifteen-minute cooldown. Screen-off disables only the
backlight. One key action wakes without performing its normal action. The
last scheduled minute forces the screen on and leaves celebration visible.
Keeping Wi-Fi/timekeeping active consumes more power than deep sleep; actual
battery life has not been measured.

## Connect and synchronize

Select Connect Wi-Fi. The device shows a Wi-Fi QR code for a unique temporary `OFFDUTY-XXXX`
WPA2 hotspot with a random password. Scan with a compatible phone camera and
keep the connection even if there is no Internet. The Wi-Fi QR, hotspot name, `http://192.168.4.1` and connection status
share one screen. Joining the hotspot changes only the status text. Open that address, select a scanned 2.4 GHz network and enter its password. Only the AP interface accepts configuration.
The form uses a random token; credentials are not echoed or logged. The
hotspot closes after five minutes, OK, or a long OK return. Join succeeds
before replacement credentials are committed. A failed attempt preserves
the saved credentials for the next boot. Forget Network deletes only this
application's network key. Open and WPA/WPA2 personal networks are supported;
5 GHz, enterprise login and captive portals are not supported.

Credentials and preferences are stored in the device's default NVS, without
claiming encrypted storage. They are not sent to a cloud service. After obtaining
an IP, SNTP queries `ntp.aliyun.com` and `time.cloudflare.com`; the default
ESP-IDF hourly synchronization interval applies. Each successful sample anchors
UTC to `esp_timer_get_time()`. UI refresh delays do not accumulate countdown
error. Disconnected operation retains this anchor until reboot. An unsynchronized
boot shows a waiting state, never a fabricated countdown. Offline drift and network
time accuracy require measurements; this is not a submillisecond timing guarantee.

## Continue work and overtime

From the off-duty home, UP opens a two-option page. Continue work adjusts the daily end time in 15-minute grid steps and saves it to device settings. Overtime starts a separate 30-minute to 8-hour session, adjusted in half-hour steps; it does not change daily start/end settings. The picker displays duration and expected end time, including the next-day label.

The overtime home shows its own remaining time and progress (below 100% until completion). The existing final-minute animation and mood-themed completion receipt apply to overtime. Short DOWN keeps the three-second break; holding DOWN for five seconds finishes the session early. Short OK on completion opens the off-duty home; long OK still opens settings.

A separate `overtime` NVS record stores session start/end. Wi-Fi loss preserves counting; after a reboot, fresh network or phone calibration restores the session. A completed session retains its receipt until the next scheduled shift begins. Continuing daily work clears the overtime record together with the settings save. Daily schedule values and partitions are retained. Physical reboot/resume, buttons, sound and display acceptance remain pending.

## Waiting for work and completed-page retention

The waiting transition follows shift order: current start, current end, waiting, next start. The default is one hour before the next start (09:00–18:00 defaults to next-day 08:00; 22:00–06:00 defaults to 21:00). Changing either daily start or end recalculates waiting. When the rest interval is shorter than an hour, the default is the shift end; the completion state is observed before waiting starts. Manual one-minute adjustment only offers times from the current end up to, but excluding, the next start. Device settings group all three time rows first; the phone form groups them before display/audio preferences and updates the wait field immediately on start/end changes. Invalid phone submissions are rejected server-side as well.

An active overnight shift or overtime session is not interrupted. Explicit continue-work changes update daily waiting, while independent overtime does not alter the daily plan. Legacy 12/14-byte settings retain other preferences; an absent `waitv` marker recalculates the new default on load, and future saves mark this migration complete. The settings blob size remains 14 bytes and partitions remain unchanged. Cached phone pages must be refreshed to load the new linkage script.

## Three-second startup

Power-on shows only the approved startup design for 3 seconds: cream background, the OFF DUTY CLUB brand and Chinese subtitle, the application title, animated coral question mark, and the existing pet sliding in, waving and hopping. Layout and keyframe timings follow the interaction preview; small caption/question font subsets and the existing Flash pet image keep RAM bounded. The LVGL task continues rendering while the application waits in short scheduler delays.

After the startup interval, the application initializes its services and displays the appropriate setup, waiting, work, or completed page. Buttons are registered after the splash; any key still held during initialization is consumed until release to prevent a queued action. Screen-off wake does not replay the splash; a cold start or reboot does.

## Offline time recovery

A previously calibrated device continues counting down when Wi-Fi disconnects;
reconnection retries do not clear its monotonic clock anchor. After a power-off
or restart, the anchor is deliberately invalid until a fresh sample arrives.
Saved shift preferences alone cannot establish the current time.

The time guidance screen opens the existing private hotspot for both Wi-Fi setup
and phone calibration. The phone page has a "Use phone time" form: a small
hash-authorized local script samples `Date.now()` at submission, without an
Internet request. The AP-only, session-token-protected `/time` endpoint validates
an exact millisecond timestamp in 2025–2099 and queues calibration in the network
worker. Queue waiting time is included when advancing the clock; transport
latency and the phone's own clock error remain unmeasured. Time source appears
in Settings and phone status; the work home shows offline continuation.

Wi-Fi SNTP samples replace phone calibration when available. Completion effects
are guarded by the latest celebrated end timestamp, and final-second tones only
advance, so backward calibration cannot replay a completed deadline. No BLE,
new NVS fields, partition changes or fabricated saved-time recovery is added.
The screen-off feature only turns off backlight; it does not discard the anchor.

## Assets and validation

UI coordinates and rendering are in `main/offduty_ui.c`; portable timing,
settings validation, form decoding and hold timing are in `main/offduty_model.c`.
The app reuses the BSP without modifying pins or drivers. Networking and audio
have worker tasks; button callbacks enqueue timestamped events only. Bluetooth
is disabled. LVGL has a 48 KB allocation pool and never allocates a full-screen
canvas. The renderer's test uses the same 48 KB pool and RGB565 output.

See [font assets](../../assets/README.md#off-duty-club-fonts) for licensing and
generation. UI-only subsets explicitly exclude arbitrary user text: the device
does not render a user-supplied SSID. The phone handles network-name entry.

Run `./tools/validate.sh` with ESP-IDF 5.5.3 activated. For actual LVGL software
rendering and glyph coverage, run:

```sh
cmake -S tests/offduty_render -B build/offduty-render -DCMAKE_BUILD_TYPE=Debug
cmake --build build/offduty-render -j 8
mkdir -p build/offduty-render/images
cd build/offduty-render/images
../offduty_render ../../../assets/fonts/offduty/characters.txt
```

The renderer emits PPM frames for all moods, break scenes, setting selections,
early exit, off-duty home, countdown boundaries and celebration. It checks every
inventory code point and printable ASCII using the selected LVGL fonts.
These checks do not prove LCD appearance, refresh rate, button timing, audio,
RF connectivity, NVS recovery or battery life. Device acceptance must test
all pages, five-second hold/cancellation, resume, overnight end, incorrect
credentials, reconnect, reboot-before-sync, screen-off wake, mute, final-minute
override and sustained simultaneous rendering/network/audio.

Implementation and host validation are complete only after the current gate
passes. Device acceptance is tracked below; flashing alone does not prove visible behavior. Do not flash
an application-only image at offset zero; use the exact verified bundle and
the [firmware data policy](../development/engineering/firmware-layout.md#flashing-and-stored-data).

The phone page also edits shift times, brightness, volume, one six-choice screen-off selector
and automatic scenes. It shows connection
and synchronization state and offers manual sync and explicit network forgetting.
Settings are validated before being handed to the main application task for NVS
saving. An independent status frame refreshes every three seconds and reports the save result without resetting edits. Access remains limited to the
private hotspot, which expires after five minutes. Device setting labels and
numeric values use the same font and are centered by their visible ink within each row.
The portal has no visible waiting countdown; the five-minute hotspot lifetime remains.

## Themes, resume and phone provisioning

The completion ticket and home mood tag share eight accent/ink palettes. Receipt paper, ticket, seal and pet expression follow the mood; the pet remains above the lower teeth. UP/DOWN on celebration release ten randomly distributed fragments in front of all text, falling out of the screen bottom over about two seconds. Short OK opens the off-duty home; long OK opens Settings without a visible long-press hint.

The resume picker changes end time in 15-minute grid steps. OK persists the chosen time to the device/phone setting before starting the new session from calibrated time; failed saves stay on the picker. Long OK cancels. Screen-off offers always on and 1, 2, 5, 10 or 30 idle minutes. Stored settings retain the old blob layout; legacy idle values round upward, capped at 30 minutes, and scheduled sleep becomes always on.

The phone portal lists up to 16 supported nearby networks ordered by signal strength, supports rescanning and retains a manual form for hidden networks. Its worker owns scan/connection operations; failed scans preserve previous results and do not interrupt credential attempts. SSIDs are escaped and requests require the private AP interface and session token. Network names/passwords are not logged.

## Full-screen breaks and pet relaxation

DOWN on the off-duty home now starts one of six full-screen pet motions: stretch, sofa flop, dance, snack, bubbles and nap. Titles, captions, layout and three-second return rail follow the approved preview. Pet colors/expressions follow the selected mood; consecutive choices do not repeat. UP cancels into the end-time picker; short/long OK opens settings. Work breaks retain the original five scenes/captions but use the same full-screen structure and three-second return, with five-second DOWN early exit kept separate.

Pet images are cached in Flash to avoid per-frame transparent-layer allocations within the 48 KB LVGL pool. Host rendering covers all 48 mood/action combinations, Chinese glyphs, partial/full frame equality and animation cleanup. Button timing and six-way selection have pure host checks. Confetti uses a fresh hardware random seed per burst for position, entry delay, shape and color; fragments are about 15 percent smaller. Existing saved settings and partitions are unchanged. Physical animation, glyph alignment, sound and button behavior require acceptance of this new build.

## Phone settings layout and inline feedback

The phone portal now separates offline phone clock sync and online Wi-Fi setup into mutually exclusive views without numbered steps. Both share the daily schedule and preferences. Compact status content replaces the full-height status iframe; native time fields have bounded widths for narrow phones. Form submissions remain on the current page, show a loading indicator, then poll the device for saved, failed, connected, or newly calibrated state. HTTP acceptance alone does not report a completed save or clock sync. Scan results update only the Wi-Fi panel; preference edits remain intact. Missing connectivity times out with an unconfirmed-result message. CSP permits only same-origin requests and the exact script hash; AP access and token validation remain required.

Resuming work clears active break animations and restarts the automatic egg cooldown. Short OK clears break state before opening settings. Both resume paths were verified in the browser; the reported unexpected-page scenario was not reproduced there. iPhone Safari layout and physical button behavior require device acceptance.

## Device confirmation for phone operations

Phone preference saves now show a three-second device footer confirming success or failure after the application storage result. Each applied phone-time request advances a revision, so repeated calibration also shows a fresh three-second confirmation. These operations wake an idle screen and retain the current page and running countdown. The footer replaces bottom helper content temporarily, preserving the QR code and portal address. Rendering tests verify the QR/address region is unchanged and dismissal restores the full original frame. Wi-Fi options now show only escaped SSID names; signal sorting and connection validation remain unchanged.

## Phone password visibility

Scanned and manual Wi-Fi forms mask passwords by default. Independent show/hide buttons change only the input type and never submit the form. Delegated handlers keep the buttons working after scan results replace the network panel.

## Not open yet waiting page

The accepted poster design replaces the circular charging page: two-line heading, a five-degree diagonal countdown band, pet at the right, and aligned charging labels, ten-segment progress, start time and button hints below. UP retains the eight-mood cycle and now changes the waiting palette, pet body/expression and dedicated pre-work caption together. Work-page palettes and captions remain separate. Long waits use a 24 px HH:MM:SS clock; ordinary waits use 45 px MM:SS. The 29 px heading has its own glyph subset, and the 8 px mood header uses a verified Chinese font. Small text layers rotate within the 48 KB LVGL pool; overlapping background triangles avoid a visible seam without allocating a full-screen layer.

## Version and last flashed validation

The application version is `1.0.2`, defined once in `main/offduty_version.h`. The ESP-IDF application descriptor, device settings header and phone settings header use this value. The version is read-only and does not change saved preferences.

Build: PASS. Host tests: PASS, including Chinese glyph coverage, 40 waiting-page frames, long countdowns, palette/pet matching, settings alignment, phone version output and partial/full rendering. Verified archive: `build/firmware/979ea53ed367a0857c7e392235e5ed747ec4b1c81150f2cbe348b72f23605fbd/`; ELF SHA-256 `97ca16a07f5bae5ee50b730ece620146f4e35ab49f69a7ce424e062431c1ac8b`. The application is 2,800,032 bytes and fits the installed 3 MiB partition.

Device tests: PASS for segmented bootloader/application flashing, unchanged-partition verification and a 35-second startup check. The running application reports version `1.0.0` and matching ELF prefix `97ca16a07`; the splash lasted 3,087 ms, with no errors or crash markers. The serial monitor is closed. Unverified: physical text alignment, eight-mood switching, animation smoothness, phone Safari layout and automatic work-start transition. Use verified bootloader/app components at `0x0`/`0x10000` when retaining the installed partition table and settings; do not flash the merged image for this workflow.

## Settings and refresh optimization

Device and phone labels use preparation time, start time and end time. All three support one-minute adjustment; overtime duration retains half-hour steps. Preparation defaults to one hour before the next start and follows schedule edits. New installations default to 09:00–18:00, 75% brightness, 50% volume and always-on display; stored preferences are retained.

A completed resumed shift now expires at the next valid preparation window instead of masking it until the next start. Wi-Fi connection success waits for the network-configuration storage result; persistence failure is reported separately from connectivity. Waiting-page redraws skip unchanged frames, update the central 120 rows for pet/countdown changes, and redraw the small charging region only on percentage changes. Host frames are compared against complete redraws. The unused 47 px clock font is removed and the 8 px subset is limited to the waiting header and mood names; this saves Flash, not runtime RAM.

Optimization validation: Build PASS; Host tests PASS, including minute-level forms, preserved preferences, resumed-shift preparation boundary, storage-failure status and LVGL partial/full frame equality. Archive `build/firmware/3daee2b5683a884377e106aeadf36bc2aba47c4561daa6a0ef62e72164386fe8/`, ELF SHA-256 `00ea0f17ec1a3783bd664b14decfcf03795f1d66ea3a07df3e2084f65d7d31a8`. Application size 2,781,168 bytes, 18,864 bytes smaller than the last flashed build. Device tests: segmented write/hash verification, unchanged installed partition table and 35-second startup observation PASS. The running ELF prefix is `00ea0f17e`, version `1.0.0`, splash 3,003 ms, no error/crash markers; monitor closed. Physical UI, animation smoothness, one-minute button editing, phone saving and Wi-Fi persistence remain unverified.

The phone footer uses compact, left-aligned helper text and a quieter brand signature. The visible show/hide controls need no duplicate explanatory text; saved passwords are still not returned to the browser.

Footer update validation: Build PASS; Host tests PASS; browser layout PASS at 390 px. Verified archive `build/firmware/8376b8aacaf4e434d9630d83ccb7a15bf17e69e3c1b0cfe4c0bb77c8c1df830f/`, ELF SHA-256 `e80dde1d8ad6d0c6d3d5ba31416f0895f911348d28a9c381374ab9e106240422`. Final binary content matches the simplified footer. Device tests: segmented write/hash checks, unchanged installed partition table and 35-second startup observation PASS. Running ELF prefix `e80dde1d8`, version `1.0.0`, splash 3,066 ms, no error/crash markers; monitor closed. Physical phone footer layout remains unverified.

## Mood-aware screens

The eight moods use three independent palette tables in `main/offduty_theme.h`. Working/overtime, preparation, final countdowns, completion stamps, off-duty home, breaks, relaxation, pickers, settings, provisioning and save/time feedback follow the selected theme and mood. Startup retains its cream background, coral question/rail and independent mint artwork. Clock setup retains its lavender background and mint pet; schedule setup retains cream. Native rendering validates all theme/mood combinations, Chinese glyphs, contrast and partial/full redraw equality. Physical colors and readability still require device acceptance.

The validation gate exports the product-named `Off-Duty-Club-1.0.2-full.bin` with matching ELF, MAP and checksums under `build/products/<full-sha>/`. CI releases only the merged image. Renaming the export preserves its bytes and its link to the verified archive.

Bright-theme/product-export validation: Build PASS; Host tests PASS, including product byte/hash identity, collision rejection and unsafe-name/corrupt-source rejection. Verified archive `build/firmware/49ce912a50a28fd6c4f5506569d97230487d10b9fc00b2d1e673a9b6bc210e7e/`, ELF SHA-256 `13793883a249fd9443a73540db106ff700773145054d4343fc7a011f1d2128ad`, application 2,801,840 bytes. Authorized segmented flashing to `/dev/cu.usbmodem1201` verified both written segments and retained the installed partition table byte-for-byte. A 35-second observation matched ELF prefix `13793883a`, version `1.0.0` and 3,048 ms splash, with no crash/reboot markers; the serial monitor was closed. The strict error-free startup check FAILed: four I2S already-disabled errors occurred during the existing audio wake/sleep flow, which handles `ESP_ERR_INVALID_STATE` as success. Codec open and low-power register checks subsequently succeeded. Physical colors, readability, interaction and audible output remain unverified.

Confetti uses five independent celebration colors (coral, yellow, green, blue and purple), randomly mixed for every burst; it does not inherit the selected mood palette.

Five-color confetti device check: verified archive `ba09b2d0646fbb7e2f469f5edf7020515c5139ce2fa300556137579cf4dd4865`, ELF SHA-256 `d103dbf7ed221a738d27fc1a71fbff8c95bff6deaa2358c33bda526be7971f5f`. Authorized segmented writes to `/dev/cu.usbmodem1201` passed hash verification; the installed partition table remained byte-identical. The 35-second startup window matched ELF prefix `d103dbf7e`, version `1.0.0` and a 3,028 ms splash, with no error/crash markers; monitor closed. Build and host/render checks PASS. Device write/startup checks PASS; physical confetti colors/randomness and sound remain unverified. This startup window does not establish that the previously observed audio wake/sleep duplicate-disable logging is fixed.

Onboarding design restoration: original cream/mint startup artwork and lavender clock setup are restored; the interactive preview adds a direct clock-setup button. Restored archive `cb2daf8281afcce2c49d23f870472bfad1cbb6e1225522d4711580ae3f7005d1` passes the complete build, host and rendering gates. Authorized segmented flashing passed both written hashes and byte-identical installed partition verification. The 35-second startup check matched ELF prefix `77e455467`, version `1.0.0`, and a 3,013 ms splash, with no error/crash markers; monitor closed. Physical display and restored-design acceptance remain unverified.

## Selectable bright themes

Settings > Color Theme selects Electric Pop, Retro Print or Jelly Light; Retro Print is the default. Each has an independent eight-mood palette, including pet and stamp artwork. Work/overtime, preparation, final countdowns, completion, off-duty home, breaks, relaxation, pickers, settings, hotspot connection and save feedback follow the selected theme. Startup and clock setup retain their original fixed designs; confetti keeps its independent five colors. The phone form exposes the same choices and retains loading/result feedback. Editing the theme no longer changes the Forget Network confirmation text: only that row can arm deletion, and only OK confirms it.

The sixteen-byte settings blob persists the theme. Twelve- and fourteen-byte legacy configurations retain their schedule, brightness and sound and default to Retro Print. New phone schedule choices use a 15-minute list; device adjustments advance to the adjacent quarter-hour, including midnight wrap. Existing off-grid stored times remain unchanged until edited. Phone submission rejects off-grid times; overtime duration retains its 30-minute step.

Theme artwork is generated from native LVGL reference rendering. Transparent pet margins are cropped with matching placement and rotation pivots. Smaller RGB565A8 frames and stamps use the bundled lossless LZ4 decoder; tall crown/halo pet frames remain uncompressed to limit transform-time memory. All 48 pet frames and 24 stamps total 457,327 Flash bytes. The LVGL pool remains 48 KB. Enable both reference targets in `tests/offduty_render`, render to a local output directory, then run `python3 tools/generate_offduty_theme_images.py <renders>` and turn reference modes off before validation. The generator verifies compression by exact round-trip comparison.

Host tests and native render checks PASS for quarter-hour boundaries, legacy configuration migration, phone validation, all 24 theme/mood combinations, all relaxation modes, menu-confirmation isolation, Chinese glyphs, and unchanged startup/clock-setup pixels. New-theme device acceptance remains unverified.

Bright-theme/menu/quarter-hour build: complete gate PASS; native glyph, layout, 144 relaxation-combination centering and menu-isolation checks PASS. Verified archive `50ef3f4828804788a7402da8ce97a3fd48be0d5693eeb0f194c305c0badbb608`; application 2,879,920 bytes, compatible with the installed 3 MB factory partition. Browser simulation confirms theme OK leaves Forget Network unarmed, and schedule lists contain 96 quarter-hour options. Authorized segmented flashing to `/dev/cu.usbmodem1201` passed both written hashes and byte-identical installed partition verification. The 35-second startup observation matched ELF prefix `de523779a`, version `1.0.0` and a 3,002 ms splash, with zero error or crash markers; the monitor was closed. Device write/startup checks PASS; actual animation timing, audio, persisted selection and Wi-Fi retention remain unverified.

Theme replacement: each family contains eight independent mood palettes, with separate foregrounds for pages, tickets, stamps and pets. Jelly Light uses native LVGL gradients on the main background and pets without adding fullscreen images or enlarging the memory pool; gradient smoothness on the 16-bit display still needs device acceptance. New settings and legacy blobs without a theme field default to Retro Print. Existing sixteen-byte settings retain their numeric selection (0 Electric Pop, 1 Retro Print, 2 Jelly Light), schedule, network, brightness and volume. Startup, clock setup and independent five-color confetti retain their designs.

Replacement-theme validation: complete Build and Host gates PASS; native render checks PASS for all 24 palettes and animation redraws, including 144 relaxation combinations across the three themes. Fixed startup and clock-setup frames remain identical across themes. Verified archive `03748f88544bd4c99bd8aceb13541f5371d3062243d2ca84ad6238ccbbddb67f`; application 2,883,680 bytes. Authorized segmented flashing to `/dev/cu.usbmodem1201` passed both written hashes. The current installed minimal partition table (NVS, PHY, factory at 0x10000 with size 0x7f0000) remained byte-identical. A 35-second startup observation matched ELF prefix `dc9029361`, version `1.0.0` and a 3,068 ms splash, with no error/crash markers; monitor closed. Device write/startup checks PASS. Physical colors, gradient smoothness, persisted settings and sound await acceptance.

The author accepted the three-theme design on the previously flashed 1.0.0 build. Version 1.0.1 retains those themes and the Retro Print default; only the version descriptor and displayed version change. Device acceptance of the rebuilt 1.0.1 image remains pending.

Version 1.0.1 validation: Build and Host tests PASS, including native Chinese glyph/layout, theme contrast and all animation redraw checks. Verified archive `70866a45f0608fab2b6a2c37221dbc7cb11baecbf8c943f28ef8f3e5e4d09e4b`, ELF SHA-256 `96d7afe04b5846218a961401849f712a0869fcb22171ad84f854107ded75eb6e`; application 2,883,680 bytes. The app descriptor and product package both identify 1.0.1. Device tests NOT RUN for this rebuilt version.

Theme discoverability: Color Theme is the first device settings row, selected when opening settings from the normal home screen. The phone preferences form also places theme selection before the work schedule. Shared named menu indices keep time edits, synchronization and Forget Network bound to their original actions.

Theme-first validation: complete Build and Host gates PASS; native layout, Chinese glyphs, theme contrast and animation checks PASS. Verified 1.0.1 archive `cb5c8111f6f39d8540871a945d7515ba87ee992f22118d05a63ff038a47ff539`; application 2,883,680 bytes. Browser settings open with Color Theme selected in the first row. Authorized segmented flashing to `/dev/cu.usbmodem1201` passed both write hashes and byte-identical partition verification. The 35-second startup observation matched ELF prefix `131eff4f8`, version `1.0.1` and a 3,072 ms splash, with no errors/crash markers; monitor closed. Device write/startup checks PASS; physical button routing, save persistence and network retention await verification.


## Landscape display

Open Settings with OK. Theme remains the first row; Screen direction and Landscape style follow it. Choose Landscape, confirm with OK to save, and hold OK to leave Settings. The phone preferences page offers the same controls. Landscape uses a 320 × 240 layout; rotating back to portrait keeps the same countdown, mood and active overtime session. The saved direction and style are restored after restart. Only the three-second boot page retains its original portrait layout. Initial time guidance and the hotspot QR page reflow to the saved direction and keep their original colors.

Classic Landscape uses the selected theme and mood for the entire page. One-line Wander and Freedom Cinema are independent styles with a curved route or a poster arc; their surfaces also respond to mood, and the companion keeps its selected mood. Work, preparation, final minute, celebration, free-time home, three-second breaks, relaxation, early exit, continue-work/overtime pickers, settings and save feedback have landscape layouts. Existing button behavior is preserved: UP changes mood at work and opens continue-work/overtime after work; short DOWN starts a break or relaxation. No automatic orientation sensor or focus timer is introduced.

The settings blob appends direction and landscape style to the existing 16-byte format (18 bytes total). Existing 12/14/16-byte preferences migrate to portrait and Classic Landscape without changing saved hours, theme, brightness or volume. Hardware rotation uses the existing panel driver with the 9,600-pixel partial buffer; rounded clipping follows the logical screen dimensions. Native LVGL rendering checks include 3 styles × 3 themes × 8 moods, all break/relaxation variants, partial/full repaint equivalence, menu rows and rotation transitions. Actual panel orientation, performance, button access, audio and persistence still require device acceptance.


Landscape delivery validation: full Build and Host gates PASS; native LVGL glyph, layout and partial/full repaint checks PASS. Verified full-image SHA-256 `95c5aeb42e6b9dfd9671563f7231837c2b9e140e7dba5cd7e475ac7d4aace89f`, matching ELF SHA-256 `34988badcb6afc9db17fae4557536c8caf10ec2c737388c3d035d7911b88c2cb`, application 2,924,368 bytes. With explicit authorization, bootloader at `0x0` and application at `0x10000` were written to `/dev/cu.usbmodem1201`; NVS and the partition table were not written. Initial tooling tried to reload a resident stub before any write; after reusing it, writing completed but the old stub connection could not read after the tool returned to ROM. A fresh read-only connection verified the installed application digest and the unchanged partition table. A bounded 35-second startup matched ELF prefix `34988badc` and version `1.0.1`; the boot page completed in 3,061 ms with no crash markers. The monitor is closed. Device write/startup checks PASS; visual orientation, actual animation performance, keys, saved display choices and audio await user acceptance.


Global shortcuts: hold UP for 0.5 seconds to switch and save screen direction on every interactive page, including settings, calibration and QR connection. Long-press shortcuts remain available without on-screen instructional labels; existing short-press legends retain their alignment. Only direction is saved: pending shift/theme edits remain pending. Save failure retains the current direction and displays an error. Hold OK opens settings with Screen direction selected, even during the final minute or a break. Holding OK inside settings still cancels an unfinished edit and returns. From a continue-work/overtime picker, this shortcut cancels the unstarted choice and opens settings; leaving settings returns to the off-duty home. Short-press actions and the five-second DOWN early-exit gesture remain unchanged; a sleeping device consumes the first action to wake. Boot has no interactive controls.


Global-shortcut build: complete Build and Host gates PASS, including the actual application dispatcher test for every interactive stage, edit isolation, wake and save failure. Native glyph/layout and rotation rendering checks PASS. Verified image `715ec2b8ceae77f1550413425d82882aed60bf5a9d9a6d266183d9cae25db2cf`, ELF `d3697ec7e0e50c319a75f9169ec5f19cbce9ce984bf5cc5c45078b5f63d32c37`, app 2,925,712 bytes. This build has not been flashed; physical long presses, orientation persistence and layout need device acceptance.


Global-shortcut device handoff: with explicit authorization, build `715ec2b8ceae77f1550413425d82882aed60bf5a9d9a6d266183d9cae25db2cf` was written to `/dev/cu.usbmodem1201` at bootloader `0x0` and app `0x10000`. Both write hashes passed; a fresh connection verified the installed application digest and all 4,096 partition-table-region bytes against the pre-write region. NVS was outside the write ranges. The 35-second startup window matched `1.0.1` and ELF prefix `d3697ec7e`; the boot page lasted 3,027 ms with no crash markers. The monitor is closed. Device write/startup checks PASS; physical shortcuts on each page, direction persistence, rendering and audio await user acceptance.


Landscape alignment and key legends: the author confirmed that the physical buttons are on the right, ordered OK, DOWN, UP from top to bottom. The interactive preview follows that order. Landscape legends use left/right arrows for the same logical UP/DOWN actions, and the bottom action legend is ordered OK, right arrow, left arrow. Portrait hints keep the original order. Menu labels and values, continue-work/overtime time cards, the QR address and save feedback are vertically centered from actual glyph bounds instead of a fixed offset. Pixel tests check menu rows and time-card centers; the existing three-theme/eight-mood glyph and rendering checks remain in place.


Alignment/key-order revision: full Build and Host gates PASS; actual glyph coverage, all menu-row centers, time-card centers and landscape animation checks PASS. Browser coordinates confirm the OK legend is leftmost and the bezel keys are OK/right/left from top to bottom, with portrait order restored when switched back. Verified firmware `6edea06f3e1db2b6a394f23791576dfd4dbc2e852fed2e3db14d9adef9453a2c`, app 2,929,152 bytes. This revision has not been flashed; physical alignment and direction hints require acceptance.


Alignment/key-order device handoff: with explicit authorization, verified build `6edea06f` was written to `/dev/cu.usbmodem1201` at bootloader `0x0` and application `0x10000`. Write hashes and a fresh-connection application digest matched; all 4,096 partition-table-region bytes were unchanged. NVS remained outside the write ranges. The 35-second startup window matched version `1.0.1` and ELF prefix `44ec7c514`, with a 3,062 ms splash and no crash markers. The serial monitor is closed. Device write/startup checks PASS; visual centering, physical key legends/actions, persistence and audio await user acceptance.


Version 1.0.2 includes the landscape layouts, global orientation/settings shortcuts and glyph-based vertical alignment. Both long-press hints now share a compact bottom line; short-press actions sit above it, with OK first in landscape. Neighboring progress rails and labels leave clearance. Complete Build and Host gates and native rendering PASS. Verified merged SHA-256 `ad167e283caac93b78c480798262a2c50f548b18264c5463d41e0700e8ab81c3`, matching ELF SHA-256 `364dda900067356163f54581a3774e63712f14ccdce568500fc8b5f503cbc8cc`; application 2,928,848 bytes. This 1.0.2 artifact has not been flashed. Device tests: NOT RUN; physical bottom-hint readability, keys, persistence and audio remain pending.


Long-press label removal: all on-screen Hold OK and orientation-switch instructional labels are removed, including continue-work/overtime pickers. Shortcut behavior is unchanged. Existing short-press legends, landscape OK-left order and glyph-centered settings/time values retain their coordinates. Native rendering checks PASS across three styles, three themes and eight moods; inspected portrait/landscape home, settings, time pickers and break images show no remaining label overlap.


Landscape relaxation captions: move only relaxation artwork 12 pixels upward, leaving the title, caption, progress rail and button legends fixed. This prevents the sofa, nap cushion and food props from entering the caption area. Break artwork and portrait layouts are unchanged. Native rendering checks include a clear strip above each landscape relaxation caption across all styles, themes and moods.
