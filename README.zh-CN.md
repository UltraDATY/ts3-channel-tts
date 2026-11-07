## 1.3.0：不朗读指定用户（音乐机器人）

在频道树里右键音乐机器人或任意用户，进入“频道文字朗读”子菜单：

- **不朗读此用户的文字消息**：加入名单，立即停止当前朗读并清空等待队列。
- **恢复朗读此用户的文字消息**：移出名单。
- **查看此用户的朗读状态**：检查是否在名单中。

顶部“插件 → 频道文字朗读 → 查看不朗读名单”显示名单数量与用户唯一身份 UID。名单保存在 `channel_tts.ini` 的 `[ignored_users]` 段，不覆盖旧配置或违禁词库。插件重载、TS3 重启后自动恢复；“重新读取配置文件”也会刷新名单。“恢复默认设置”保留此名单。最多 256 人，保存失败会撤回本次修改并提示。

名单按 TS3 的用户唯一身份 UID 精确匹配，不依赖容易变化的昵称或会话 clientID，因此重连、改名后仍生效，同名但不同身份的用户不受影响。名单跨服务器连接生效。机器人更换身份后需要重新加入。不自动识别机器人，需要手动选中目标。只跳过其文字朗读，不屏蔽频道文字、不静音机器人播放的音乐。该功能不依赖违禁词过滤开关。已在队列中的旧消息在名单变更时全部清除，避免继续朗读。

实现使用 SDK 的 CLIENT_UNIQUE_IDENTIFIER 与文字回调的 fromUniqueIdentifier。新增自动测试验证：加入/移出、重连/改名、同名不同 UID、保存和重新加载、空名单、重复添加、数量上限、查询失败、保存失败回退及队列取消。通过本机 MSVC x64 编译、原有过滤测试和 DLL 加载/菜单测试；尚未在真实 TS3 中验证右键菜单。

# 频道文字朗读 (Channel TTS) 1.3.0

Windows x64 原生 C++17 插件，使用 Windows SAPI 5 的默认语音在本机播放。无需云服务、密钥、Qt 或额外 TTS 运行时。提供的 DLL 已在本机 MSVC x64 编译；未在真实 TS3 客户端中联调，请按文末步骤验收。

## 中文版更新

插件名称、简介、命令帮助、状态和操作提示已中文化。命令关键词仍使用 /tts。安装包为 ChannelTTS-1.3.0-zh-CN-win64.ts3_plugin，可双击安装后重启客户端。1.3.0 新增顶部“插件”菜单设置，所有选项均可鼠标操作；原命令作为可选快捷方式保留。界面中文化不会自动安装中文语音。

## 安装

1. 使用 **TeamSpeak 3 Windows 64 位、API 26** 客户端。官方 SDK 26 标注最低 TS3 3.6.0；未来 API 若改变需重新适配。此 DLL 不适用于 TS6、32 位客户端、Linux 或 macOS。
2. 完全退出 TS3，将根目录的 `channel_tts_win64.dll` 复制到 TS3 用户配置目录下的 `plugins` 文件夹。标准安装通常为 `%APPDATA%\TS3Client\plugins`；便携版请使用其实际配置目录。没有文件夹时手动创建。
3. 启动 TS3，在“工具 → 选项 → 附加组件/插件”（名称随语言和版本变化）找到 **Channel TTS** 并启用。
4. 进入频道，让另一位用户发送文字；在聊天输入框输入 `/tts status` 查看状态。默认不朗读自己，测试自己消息前输入 `/tts self on`。
5. 卸载时先退出 TS3，再移除 DLL；如需清除设置，移除配置目录中的 `channel_tts.ini`。

语音走 Windows 默认音频输出，不自动跟随 TS3 的播放设备，也不会注入麦克风或广播给频道。中文发音取决于安装的 **64 位 SAPI 兼容语音**；Windows 设置中显示的部分 OneCore/自然语音不一定向桌面 SAPI 暴露。本版本使用 SAPI 默认语音，不提供语音枚举或声音选择。无声时先检查 `/tts on`、`/tts volume 80`、Windows 音量混合器和默认输出设备；初始化失败请查看 TS3 客户端日志。

## 违禁词过滤（1.3.0 新增）

顶部“插件 → 频道文字朗读”新增：

- **违禁词过滤：开启 / 关闭**：点击切换，默认关闭，自动保存。
- **编辑违禁词库…**：用记事本打开词库。每行一个词，保存为 UTF-8；空行和以 # 开头的行忽略。
- **重新加载违禁词库**：编辑保存后点击，让修改生效，并停止旧朗读队列。

初次启动在 TS3 配置目录创建 `channel_tts_blocked_words.txt`，内含 3 个可修改的示例词条，不会覆盖已有词库。词库最多 1MB、1000 个词，每词最多 128 个 UTF-16 单元。无效编码或超限时保留上次已加载的词库，错误记录在客户端日志中；首次启动无法加载词库时列表为空，过滤不生效。“查看当前设置”显示过滤状态和词条数量。

开启后，消息正文包含任一词条则整条跳过朗读。开启“朗读发送者昵称”时，昵称也参与过滤；关闭昵称朗读时不检查昵称。匹配为连续子串，英文不区分大小写，不做拼音、谐音、插入空格、正则表达式或网络词库识别。可能命中包含词条的较长单词，建议自行调整词库。频道文字显示不受影响。正文在截断前完整检查，切换过滤开关会停止并清空旧朗读队列。

开关存储在 `channel_tts.ini` 的 `[tts]` 下，键为 `filter=0` 或 `filter=1`。恢复默认设置会关闭过滤，但保留自定义词库。

已通过额外自动测试：中文命中、英文大小写、昵称过滤、关闭后放行、正文截断后的词条、空词库、UTF-8/BOM/注释解析、已有词库不覆盖、设置持久化、菜单切换与词库重读、非法编码回退。仍需在真实 TS3 客户端验证菜单与接收效果。

## 菜单设置（推荐）

安装 1.3.0 后重启 TS3，点击顶部“插件”→“频道文字朗读 (Channel TTS)”。可以直接通过菜单完成全部设置，不需要输入 /tts help。语速每次调整 1，音量每次调整 10，设置自动保存；当前值在本地提示中显示。该菜单使用 SDK 的 PLUGIN_MENU_TYPE_GLOBAL、ts3plugin_initMenus 与 ts3plugin_onMenuItemEvent 实现，见官方 SDK 示例。已通过测试宿主验证，真实 TS3 菜单显示仍待验证。

## 命令与配置（可选）

命令在本地处理，不发送到聊天频道。配置为全局设置，适用于所有服务器连接。

| 命令 | 效果 |
| --- | --- |
| `/tts on`、`/tts off` | 启用/禁用，禁用同时停止当前朗读并清队列 |
| `/tts stop` | 停止当前朗读并清队列，仍接收后续消息 |
| `/tts self on`、`/tts self off` | 是否朗读自己的消息，默认 off |
| `/tts names on`、`/tts names off` | 是否先读发送者昵称，默认 on |
| `/tts rate -2` | 语速 -10 到 10，默认 0 |
| `/tts volume 80` | 音量 0 到 100，默认 80 |
| `/tts status` | 显示设置 |
| `/tts reload` | 重读配置并停止当前队列 |
| `/tts help` | 显示命令帮助 |

设置命令自动写入 TS3 `getConfigPath()` 返回目录下的 `channel_tts.ini`。手动编辑后执行 `/tts reload`。可使用以下内容：

```ini
[tts]
enabled=1
self=0
names=1
rate=0
volume=80
```

无需输入命令：顶部“插件”→“频道文字朗读 (Channel TTS)”提供开启/关闭、停止朗读、切换朗读自己/昵称、语速 ±1、音量 ±10、查看设置、重新读取配置、恢复默认设置。每次调整后在本地显示状态并自动保存。菜单文字为固定操作名，当前状态以本地提示为准。插件也可从 TS3 插件管理器整体启用或禁用。保存失败会写入客户端日志，当前会话中的设置仍有效。

## 源码与构建

需要 Visual Studio 2022 或更新版本（或 Build Tools）的“使用 C++ 的桌面开发”组件、MSVC x64 与 Windows SDK。项目携带官方 API 26 头文件，无需构建时联网下载。

最直接：在项目目录运行 `build.cmd`。输出为 `build\channel_tts_win64.dll`。脚本自动通过 vswhere 查找 MSVC；也可在 x64 Native Tools Command Prompt 内运行。使用静态 C++ 运行库，避免单独部署 VC 运行库 DLL。

也提供 CMake 3.20+ 配置。在 x64 开发者命令行中运行（Visual Studio 2026 需使用支持该版本的新版 CMake）：

```powershell
cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
```

或 VS2022：

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
```

安装重新编译后的 DLL 时使用上述安装步骤。请勿直接覆盖正在运行的插件。

## API 与实现边界

- `ts3plugin_apiVersion()` 返回 26；导出 C ABI，入口采用 SDK 的函数签名；SDK 通过 `ts3plugin_setFunctionPointers()` 提供函数表，不链接商业 TeamSpeak SDK 库。
- `ts3plugin_onTextMessageEvent()` 仅接受 `TextMessageTarget_CHANNEL`，跳过 `ffIgnored`，仅处理 `getCurrentServerConnectionHandlerID()` 对应的所选服务器连接；此处“当前频道”是该连接中用户实际加入的频道，不是历史聊天标签页。
- 官方枚举说明频道消息只交给当时在该频道的客户端。回调中的 `toID` 是 `anyID`，不能当作完整 `uint64` 频道 ID 使用；因此不以它推断频道。返回 0 保留客户端正常文字显示。
- 切换服务器、自身移动、被移动/踢出及连接状态变化会停止旧朗读。连接状态变化保守地清空全局队列，包括其他服务器连接的状态变化。
- TS3 回调只转换、复制和排队；独立线程创建及释放 COM/SAPI 对象。异步朗读每 40ms 检查取消，卸载时清队列、取消并等待工作线程结束。第三方 SAPI 引擎若自身阻塞，无法保证卸载耗时上限。
- 队列最多 20 条等待消息，满时丢弃最旧等待项。正文最多 500 个 UTF-16 单元，昵称最多 80 个。无额外每用户限流或去重。
- UTF-8 输入严格转换为 UTF-16。`SPF_IS_NOT_XML` 防止将消息作为 SAPI XML/SSML 解释。BBCode、URL 和表情保留为文字，实际读法依赖语音引擎。不会主动打开链接、读取聊天内引用的文件或上传消息。

## 验证记录与手动验收

已执行：MSVC x64 编译无警告；测试宿主动态加载 DLL，验证 15 个全局菜单项、UTF-8 中文标签、菜单事件操作、语速/音量边界及保存重读，核对 API 26 与所需入口，初始化/卸载三轮，调用频道/私聊/其他服务器/自身/已忽略消息回调及开关、非法语速、reload、stop 命令。SAPI COM 对象创建成功。测试宿主不是 TS3，因此这些检查不证明真实客户端接收过滤或声音效果。

真实客户端尚待验证：

1. 另一用户发送中文/英文频道消息，应在本机朗读并保留文字。
2. 私聊、全服文字、自己消息（self off）、屏蔽用户消息不朗读。
3. 排队长消息时执行 off/stop、切频道、切服务器标签、被管理员移动，旧语音应停止。
4. 重新加载插件和重启 TS3，确认设置保留；连续禁用/启用不崩溃。
5. 测试多连接、中文语音、音频设备和大量消息；确认用户需要的发音及输出设备。

## 官方依据与第三方文件

核对日期：2026-11-06。

- SDK 26 / TS3 3.6.0+：[官方发布页](https://github.com/teamspeak/ts3client-pluginsdk/releases/tag/26)
- 头文件来源：[官方 SDK](https://github.com/teamspeak/ts3client-pluginsdk)，tag `26`，commit `b2f4b8e574d13c584ba8cae0c5ab6e1a0c3048e6`。`sdk/include` 原样保留其版权声明；第三方头文件不适用本项目源码许可，使用或分发时须遵守 TeamSpeak 对 SDK 的适用条款。仓库未附单独 LICENSE 文件，商用分发前应向 TeamSpeak 核实授权。
- 回调签名：[官方示例](https://github.com/teamspeak/ts3client-pluginsdk/blob/26/src/plugin.c)、[频道消息枚举](https://github.com/teamspeak/ts3client-pluginsdk/blob/26/include/teamspeak/public_definitions.h)
- TTS 标志：[Microsoft SPEAKFLAGS](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms717252(v=vs.85))

本项目自行编写的 `src/plugin.cpp`、`CMakeLists.txt`、`build.cmd` 和说明文档采用 MIT 许可，见 LICENSE。




