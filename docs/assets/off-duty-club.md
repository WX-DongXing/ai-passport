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
Shift and waiting times change by one minute, brightness and volume by five percent.
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

From the off-duty home, UP opens a two-option page. Continue work adjusts the daily end time in one-minute steps and saves it to device settings. Overtime starts a separate 30-minute to 8-hour session, adjusted in half-hour steps; it does not change daily start/end settings. The picker displays duration and expected end time, including the next-day label.

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

The resume picker changes end time in one-minute steps. OK persists the chosen time to the device/phone setting before starting the new session from calibrated time; failed saves stay on the picker. Long OK cancels. Screen-off offers always on and 1, 2, 5, 10 or 30 idle minutes. Stored settings retain the old blob layout; legacy idle values round upward, capped at 30 minutes, and scheduled sleep becomes always on.

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

The application version is `1.0.0`, defined once in `main/offduty_version.h`. The ESP-IDF application descriptor, device settings header and phone settings header use this value. The version is read-only and does not change saved preferences.

Build: PASS. Host tests: PASS, including Chinese glyph coverage, 40 waiting-page frames, long countdowns, palette/pet matching, settings alignment, phone version output and partial/full rendering. Verified archive: `build/firmware/979ea53ed367a0857c7e392235e5ed747ec4b1c81150f2cbe348b72f23605fbd/`; ELF SHA-256 `97ca16a07f5bae5ee50b730ece620146f4e35ab49f69a7ce424e062431c1ac8b`. The application is 2,800,032 bytes and fits the installed 3 MiB partition.

Device tests: PASS for segmented bootloader/application flashing, unchanged-partition verification and a 35-second startup check. The running application reports version `1.0.0` and matching ELF prefix `97ca16a07`; the splash lasted 3,087 ms, with no errors or crash markers. The serial monitor is closed. Unverified: physical text alignment, eight-mood switching, animation smoothness, phone Safari layout and automatic work-start transition. Use verified bootloader/app components at `0x0`/`0x10000` when retaining the installed partition table and settings; do not flash the merged image for this workflow.

## Settings and refresh optimization

Device and phone labels use preparation time, start time and end time. All three support one-minute adjustment; overtime duration retains half-hour steps. Preparation defaults to one hour before the next start and follows schedule edits. New installations default to 09:00–18:00, 75% brightness, 50% volume and always-on display; stored preferences are retained.

A completed resumed shift now expires at the next valid preparation window instead of masking it until the next start. Wi-Fi connection success waits for the network-configuration storage result; persistence failure is reported separately from connectivity. Waiting-page redraws skip unchanged frames, update the central 120 rows for pet/countdown changes, and redraw the small charging region only on percentage changes. Host frames are compared against complete redraws. The unused 47 px clock font is removed and the 8 px subset is limited to the waiting header and mood names; this saves Flash, not runtime RAM.

Optimization validation: Build PASS; Host tests PASS, including minute-level forms, preserved preferences, resumed-shift preparation boundary, storage-failure status and LVGL partial/full frame equality. Archive `build/firmware/3daee2b5683a884377e106aeadf36bc2aba47c4561daa6a0ef62e72164386fe8/`, ELF SHA-256 `00ea0f17ec1a3783bd664b14decfcf03795f1d66ea3a07df3e2084f65d7d31a8`. Application size 2,781,168 bytes, 18,864 bytes smaller than the last flashed build. Device tests: segmented write/hash verification, unchanged installed partition table and 35-second startup observation PASS. The running ELF prefix is `00ea0f17e`, version `1.0.0`, splash 3,003 ms, no error/crash markers; monitor closed. Physical UI, animation smoothness, one-minute button editing, phone saving and Wi-Fi persistence remain unverified.
