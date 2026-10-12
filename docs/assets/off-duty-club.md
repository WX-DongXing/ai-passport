**English** · [简体中文](off-duty-club.zh_CN.md)

# Off Duty Club

This application replaces the hardware-test menu with a 240 × 320 companion.
The display has eight moods, five nonrepeating break scenes, daily progress,
an animated final-minute launch countdown, and a celebration page. Audio is
synthesized locally and honors the volume setting, including mute.

The home companion breathes, blinks, looks around and occasionally waves; first-run pages include the same living pet.

## First-run home

The home guides new users through Wi-Fi/time synchronization and shift setup.
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
DOWN starts a full-screen pet relaxation scene and OK opens Settings. UP opens the end-time picker. Resuming uses
the current calibrated time, not the moment the break began. Early exit is a
temporary state in RAM and does not change or persist the scheduled end time.
The regular final-minute launch is suppressed while already in early exit.

Settings contains start/end times, brightness, volume, one six-choice screen-off selector,
automatic break scenes, Wi-Fi setup,
manual synchronization, synchronization status, Forget Network, and Return.
UP/DOWN selects a row. OK enters editing; UP/DOWN adjusts and OK saves.
Time settings change by 30 minutes for shift times, brightness and volume by five percent.
Holding OK cancels an unfinished edit and returns. Forget Network requires
another OK; UP/DOWN cancels the confirmation. Existing unrelated NVS data is
preserved. Start and end cannot be equal; overnight shifts are supported.

Default hours are 09:00–18:00 China time, brightness 80%, volume 35%, screen-off
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

The resume picker changes end time in half-hour steps. OK persists the chosen time to the device/phone setting before starting the new session from calibrated time; failed saves stay on the picker. Long OK cancels. Screen-off offers always on and 1, 2, 5, 10 or 30 idle minutes. Stored settings retain the old blob layout; legacy idle values round upward, capped at 30 minutes, and scheduled sleep becomes always on.

The phone portal lists up to 16 supported nearby networks ordered by signal strength, supports rescanning and retains a manual form for hidden networks. Its worker owns scan/connection operations; failed scans preserve previous results and do not interrupt credential attempts. SSIDs are escaped and requests require the private AP interface and session token. Network names/passwords are not logged.

## Full-screen breaks and pet relaxation

DOWN on the off-duty home now starts one of six full-screen pet motions: stretch, sofa flop, dance, snack, bubbles and nap. Titles, captions, layout and three-second return rail follow the approved preview. Pet colors/expressions follow the selected mood; consecutive choices do not repeat. UP cancels into the end-time picker; short/long OK opens settings. Work breaks retain the original five scenes/captions but use the same full-screen structure and three-second return, with five-second DOWN early exit kept separate.

Pet images are cached in Flash to avoid per-frame transparent-layer allocations within the 48 KB LVGL pool. Host rendering covers all 48 mood/action combinations, Chinese glyphs, partial/full frame equality and animation cleanup. Button timing and six-way selection have pure host checks. Confetti uses a fresh hardware random seed per burst for position, entry delay, shape and color; fragments are about 15 percent smaller. Existing saved settings and partitions are unchanged. Physical animation, glyph alignment, sound and button behavior require acceptance of this new build.

## Current validation

Build: PASS. Host tests: PASS, including production LVGL rendering and glyph coverage. Verified full-image identity: `fff5dbde84242e9ea7438b2d27b2fbdd7241ec219e0aede5b2630ba1cc7dd00f`; matching ELF: `6f3ec10f0eea309c8a00ff74e3c12444bbd969398ff992f1ce12d5fe215f3587`.

Authorized segmented flashing wrote only bootloader `0x0` and application `0x10000`, verified both image hashes and retained the original partition table byte-for-byte. The application is 2,568,880 bytes and fits the installed 3 MB factory partition. Device tests: NOT RUN beyond flash/partition verification; USB disappeared after reset and did not reappear within the bounded observation. Startup logs, physical full-screen animations, text alignment, sound, settings retention and smoothness remain unverified. Raw logs and generated firmware/debug bundles stay outside Git.
