# 频道文字朗读 (Channel TTS) 1.0.1

Windows x64 原生 C++17 插件，使用 Windows SAPI 5 的默认语音在本机播放。无需云服务、密钥、Qt 或额外 TTS 运行时。提供的 DLL 已在本机 MSVC x64 编译；未在真实 TS3 客户端中联调，请按文末步骤验收。

## 中文版更新

插件名称、简介、命令帮助、状态和操作提示已中文化。命令关键词仍使用 /tts。安装包为 ChannelTTS-1.0.1-zh-CN-win64.ts3_plugin，可双击安装后重启客户端。界面中文化不会自动安装中文语音。

## 安装

1. 使用 **TeamSpeak 3 Windows 64 位、API 26** 客户端。官方 SDK 26 标注最低 TS3 3.6.0；未来 API 若改变需重新适配。此 DLL 不适用于 TS6、32 位客户端、Linux 或 macOS。
2. 完全退出 TS3，将根目录的 `channel_tts_win64.dll` 复制到 TS3 用户配置目录下的 `plugins` 文件夹。标准安装通常为 `%APPDATA%\TS3Client\plugins`；便携版请使用其实际配置目录。没有文件夹时手动创建。
3. 启动 TS3，在“工具 → 选项 → 附加组件/插件”（名称随语言和版本变化）找到 **Channel TTS** 并启用。
4. 进入频道，让另一位用户发送文字；在聊天输入框输入 `/tts status` 查看状态。默认不朗读自己，测试自己消息前输入 `/tts self on`。
5. 卸载时先退出 TS3，再移除 DLL；如需清除设置，移除配置目录中的 `channel_tts.ini`。

语音走 Windows 默认音频输出，不自动跟随 TS3 的播放设备，也不会注入麦克风或广播给频道。中文发音取决于安装的 **64 位 SAPI 兼容语音**；Windows 设置中显示的部分 OneCore/自然语音不一定向桌面 SAPI 暴露。本版本使用 SAPI 默认语音，不提供语音枚举或声音选择。无声时先检查 `/tts on`、`/tts volume 80`、Windows 音量混合器和默认输出设备；初始化失败请查看 TS3 客户端日志。

## 命令与配置

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

没有独立配置窗口。插件可从 TS3 插件管理器整体启用或禁用。保存失败会写入客户端日志，当前会话中的设置仍有效。

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

已执行：MSVC x64 编译无警告；测试宿主动态加载 DLL，核对 API 26 与所需入口，初始化/卸载三轮，调用频道/私聊/其他服务器/自身/已忽略消息回调及开关、非法语速、reload、stop 命令。SAPI COM 对象创建成功。测试宿主不是 TS3，因此这些检查不证明真实客户端接收过滤或声音效果。

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

