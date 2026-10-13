<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

### Off Duty Club fonts

The [application guide](../docs/assets/off-duty-club.md) uses the redistributable
subsets under `fonts/offduty/`: Source Han Sans SC Medium for UI text, Roboto Mono
700 for clocks, and Anton sheared six degrees for launch digits. Original sources:
[Source Han Sans](https://github.com/adobe-fonts/source-han-sans/tree/release/OTF/SimplifiedChinese),
[Anton](https://github.com/google/fonts/tree/main/ofl/anton),
[Roboto Mono](https://github.com/google/fonts/tree/main/ofl/robotomono).
All use SIL OFL; their license files are retained alongside the subsets.
Modified subset families are named OffDutySans, OffDutyClock and OffDutyDigits.
OTF/TTF files are the reusable conversion inputs; WOFF2 versions share these
glyphs with the browser preview. No full CJK font is embedded in firmware.

`characters.txt` inventories all non-ASCII characters in `main/offduty_ui.c`.
The 8 px subset contains only the waiting-page header and mood names. Each UI size (9, 10, 11, 12, 14, 21, 23, 24, 29, 44) contains this inventory plus printable ASCII;
clock sizes 24/35/45 and launch sizes 128/148 contain `0123456789:-`. Generated bitmap
sources are constant 4 bpp, uncompressed, without kerning, compatible with LVGL
9.5.0, and compiled by `main/CMakeLists.txt`. Dynamic SSIDs are entered only on
the phone. Font coverage is checked with real LVGL glyph descriptors by the
application renderer, including placeholder rejection.

Reproduce with fonttools 4.60.1, brotli 1.1.0 and lv_font_conv 1.5.3:

```sh
python3 tools/prepare_offduty_fonts.py --cjk /path/SourceHanSansSC-Medium.otf --anton /path/Anton-Regular.ttf --mono '/path/RobotoMono[wght].ttf'
python3 tools/generate_offduty_fonts.py --converter /path/node_modules/lv_font_conv/lv_font_conv.js --cjk assets/fonts/offduty/OffDutySans.otf --digits assets/fonts/offduty/OffDutyDigits.ttf --mono assets/fonts/offduty/OffDutyClock.ttf
```

After UI strings change, regenerate both subsets and bitmap fonts, rerun coverage,
and check the firmware MAP and real-device rendering. Font data resides in Flash;
the app reserves a 48 KB LVGL pool. See the application guide for rendering checks.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

### Off-duty stamp rasters

`images/offduty/od_stamps.c` contains eight 100 × 90 RGB565 stamps rendered from this application's original LVGL artwork and OFL fonts. They use 144,000 bytes of Flash and no retained RAM buffer. To regenerate after changing stamp colors or fonts, configure `tests/offduty_render` with `-DOD_STAMP_REFERENCE=ON`, build and run it in its images directory, then run `python3 tools/generate_offduty_stamps.py <images-directory>`. Restore `-DOD_STAMP_REFERENCE=OFF` and compare all eight completion renders before delivery.

The relaxation pet sprites in `assets/images/offduty/od_pets.c` preserve the original LVGL artwork. Six motions share eight normal and eight resting expressions (13 unique RGB565A8 images, 261,924 data bytes in Flash); no dynamic pet layer is allocated. Regenerate transparent BGRA inputs with the `OD_PET_REFERENCE` host-renderer target `offduty_pet_assets`, then run `python3 tools/generate_offduty_pets.py <render-directory>`. These are original project assets, using the same pet drawing code as the live app.

The charging wait page additionally uses `od_wait_title` at 22 px for its fixed heading; the 47 px clock shares the Roboto Mono source.
