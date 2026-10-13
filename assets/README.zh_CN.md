<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

### 下班应用字体

[应用说明](../docs/assets/off-duty-club.zh_CN.md)使用 `fonts/offduty/` 下可再分发的子集：Source Han Sans SC Medium 用于文字，Roboto Mono 700 用于时分秒，Anton 向右倾斜六度用于发射数字。原始来源：[Source Han Sans](https://github.com/adobe-fonts/source-han-sans/tree/release/OTF/SimplifiedChinese)、[Anton](https://github.com/google/fonts/tree/main/ofl/anton)、[Roboto Mono](https://github.com/google/fonts/tree/main/ofl/robotomono)。均采用 SIL OFL，许可证保留在字体目录。修改后的子集命名为 OffDutySans、OffDutyClock、OffDutyDigits。OTF/TTF 是可复用转换输入，WOFF2 与浏览器预览共用字形；固件不嵌入完整 CJK 字库。

8 px 字体仅包含等待页页眉和心情名称。`characters.txt` 收集 `main/offduty_ui.c` 的所有非 ASCII 字符。界面字号 9、10、11、12、14、21、23、24、29、44 均覆盖字表和可打印 ASCII；时分秒字号 24/35/45，发射数字字号 128/148 覆盖 `0123456789:-`。生成的位图为常量 4 bpp、不压缩、无字距微调，兼容 LVGL 9.5.0，通过 `main/CMakeLists.txt` 编译。任意 SSID 仅在手机上输入。应用渲染测试用实际 LVGL 字形描述验证覆盖，拒绝占位符。

使用 fonttools 4.60.1、brotli 1.1.0、lv_font_conv 1.5.3 重现：

```sh
python3 tools/prepare_offduty_fonts.py --cjk /path/SourceHanSansSC-Medium.otf --anton /path/Anton-Regular.ttf --mono '/path/RobotoMono[wght].ttf'
python3 tools/generate_offduty_fonts.py --converter /path/node_modules/lv_font_conv/lv_font_conv.js --cjk assets/fonts/offduty/OffDutySans.otf --digits assets/fonts/offduty/OffDutyDigits.ttf --mono assets/fonts/offduty/OffDutyClock.ttf
```

界面文字变化后重新生成子集和位图，运行覆盖检查，核对固件 MAP 及真机渲染。字库位图在 Flash，应用预留 48 KB LVGL 内存池。渲染检查见应用说明。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

### 下班盖章栅格图

`images/offduty/od_stamps.c` 包含八个 100 × 90 RGB565 印章，由本应用原版 LVGL 图形及 OFL 字体渲染得到。占用 144,000 字节 Flash，无需常驻 RAM 缓冲。修改印章颜色或字体后，将 `tests/offduty_render` 配置为 `-DOD_STAMP_REFERENCE=ON`，构建并在图像目录运行，再执行 `python3 tools/generate_offduty_stamps.py <图像目录>`。交付前恢复 `-DOD_STAMP_REFERENCE=OFF` 并比较八种下班页面。

放松宠物精灵 `assets/images/offduty/od_pets.c` 保留原版 LVGL 造型。六种动作共用八种普通及八种休息表情（13 张去重 RGB565A8 图片，Flash 数据 261,924 字节），无需动态宠物图层。通过主机渲染器的 `OD_PET_REFERENCE` 目标 `offduty_pet_assets` 重新生成透明 BGRA 输入，再运行 `python3 tools/generate_offduty_pets.py <render-directory>`。资源是本项目原创，复用实际应用的宠物绘制代码。

充能等待页另外使用 22 像素的 `od_wait_title` 固定标题字库；47 像素时钟与原时钟共用 Roboto Mono 源字体。
