# Channel TTS 1.6.0

[English](README.md) | [简体中文](README.zh-CN.md)

A C++17 plugin for **TeamSpeak 3 on Windows x64, Plugin API 26**. It reads text messages from your current channel aloud on your computer using the default Windows desktop SAPI voice.

[Download the latest release](https://github.com/UltraDATY/ts3-channel-tts/releases/latest) · [All releases](https://github.com/UltraDATY/ts3-channel-tts/releases)

## Interface and workflow in 1.6.0

The settings window follows the layout of TeamSpeak 3's Options dialog: a white navigation sidebar with blue-gray icons, a blue-gray gradient header, grouped settings, and **OK / Cancel / Apply** buttons. It uses native Windows controls and an embedded Common Controls v6 manifest. No Qt installation or external image assets are required. Layout uses dialog units and a 9-point font with Chinese support, without changing the host application's DPI settings.

Open **Plugins → 频道文字朗读 (Channel TTS)** for three shortcuts:

- **设置 / Settings…** opens the unified settings window. You can also use the plugin's configuration button in the Addons manager.
- **朗读：开启 / 关闭 | Speech on/off** immediately toggles speech and saves the choice.
- **停止朗读 / Stop speech** stops the current speech and clears the queue. New messages still follow the saved enable/disable setting.

The settings window has three pages. Switching pages preserves unapplied edits.

| Page | Settings |
| --- | --- |
| Speech | Enable speech, read your own messages, read sender names, adjust rate and volume with synchronized sliders and number fields, select the interface language, and preview speech |
| Word filter | Enable filtering, edit one word per line, and view entry counts and limits |
| Ignored users | View the list and restore speech for selected users; online users on the current server show nicknames, while other users show UIDs |

**OK** validates and saves all three pages, then closes the window. **Apply** saves all pages and keeps the window open; it is disabled until you make a change. Applying a language choice immediately updates the settings window. **Cancel** or closing the window discards unapplied edits; previously applied changes remain saved. Invalid values keep the window open and select the relevant page without discarding your input. Save failures show an error and keep the window open. Ordinary file-lock failures roll back the word-list file and leave the active settings unchanged.

**Preview** immediately plays a sample using the entered rate and volume, clears the old speech queue, and does not save the entered settings. Preview works even when speech is disabled. Closing the settings window stops the preview. **Stop speech** also runs immediately. Cancel cannot undo these immediate actions.

**Speech defaults** fills in the default speech settings, which take effect only after saving: speech enabled, own messages excluded, sender names included, rate 0, volume 80, and filtering disabled. It preserves the word list, ignored users, and interface language.

The interface supports **Simplified Chinese and English**, with Chinese selected by default. Applying a choice updates the current settings window, status messages, prompts, and help. The three top-level shortcuts always use bilingual labels. Restart TS3 to refresh existing user context-menu labels. Interface language does not translate messages, modify the word list, or select a different Windows voice.

## Installation and upgrades

1. Use **TeamSpeak 3 for Windows, 64-bit, Plugin API 26**. The official SDK 26 specifies TS3 3.6.0 or later. This plugin does not support TS6, 32-bit clients, Linux, or macOS.
2. **Exit TS3 completely, including its system-tray process**, then open `ChannelTTS-1.6.0-win64.ts3_plugin` to install it.
3. If the addon installer fails, extract `ChannelTTS-1.6.0-win64-manual.zip`. Copy `plugins/channel_tts_win64.dll` into `%APPDATA%\TS3Client\plugins`, replacing the old DLL. For portable installations, use the client's actual configuration directory.
4. Start TS3, enable the plugin in the Addons manager, and confirm that its version is **1.6.0**. Upgrading preserves your settings, word list, and ignored users.
5. Open Settings and ask another user to send a channel message. To test your own messages, enable **Read my own messages** first.

Speech plays locally through the default Windows output device. It is not injected into your microphone or broadcast to the channel, and it does not automatically follow TS3's output-device selection. Chinese speech requires a compatible desktop SAPI Chinese voice; some Windows OneCore or natural voices are not exposed to desktop SAPI. No cloud service or API key is needed. If speech is silent, check the enable switch, volume, Windows volume mixer, and default output device. Initialization errors appear in the client log.

## Music bots and ignored users

Right-click a user in the channel tree, open the Channel TTS submenu, and choose **Ignore this user for speech**. Choose **Allow speech for this user** to restore it. Context-menu changes are saved immediately and reverted if saving fails. Removing a user inside Settings takes effect after choosing **OK** or **Apply**.

The list stores up to **256 unique user identities (UIDs)** and applies across server connections. Reconnecting or renaming does not remove an entry; another user with the same nickname and a different UID is unaffected. Add a bot again if it changes its identity. Ignoring a user only skips their text-to-speech: it does not hide chat or mute music. Nicknames help identify online users, but matching always uses the UID.

## Word filter

Edit the list on the Word filter page and apply it directly. No text editor or chat command is required. If a message contains any listed entry, the entire message is skipped. Sender names are also checked when reading names is enabled. Matching uses contiguous substrings and ignores English letter case. It does not perform phonetic, homophone, regular-expression, or online-dictionary matching. Chat remains visible.

The list supports **1,000 entries**, with **128 UTF-16 code units per entry**. Blank lines, surrounding whitespace, and comments beginning with `#` are ignored. Saving an empty list clears it. Changed lists are saved as one entry per line without comments. First launch creates three editable sample entries without overwriting an existing file. Disabling the filter does not delete your list.

## Configuration and optional commands

Files are stored in the directory returned by the SDK's `getConfigPath()`:

- `channel_tts.ini`: speech options in `[tts]`, `language=zh-CN` or `language=en` in `[ui]`, and UIDs in `[ignored_users]`.
- `channel_tts_blocked_words.txt`: a UTF-8 word list, up to 1 MB. After editing files manually, use `/tts reload` to reload settings, language, ignored users, and the word list together.

Optional commands are handled locally and are not sent to the chat channel.

| Command | Action |
| --- | --- |
| `/tts on`, `/tts off` | Enable or disable speech |
| `/tts stop` | Stop speech and clear the queue |
| `/tts status` | Show current settings |
| `/tts reload` | Reload configuration and clear the queue |
| `/tts self on`, `/tts self off` | Include or exclude your own messages |
| `/tts names on`, `/tts names off` | Include or exclude sender names |
| `/tts rate -2` | Set the rate, from -10 to 10 |
| `/tts volume 80` | Set the volume, from 0 to 100 |

The settings window, menu shortcuts, and commands share the same save process. Active settings are updated only after a successful save. Configuration is staged in temporary files in the same directory. When the word list changes, the old file is backed up before replacement. If replacing the configuration fails, the plugin attempts to restore the previous word list. If rollback fails, it retains the backup and reports its path. This is not a cross-file transaction guaranteed to survive power loss. Upgrading or uninstalling does not automatically delete user settings.

## Building and validation

Install the **Desktop development with C++** workload for Visual Studio 2022 or later, including MSVC x64 and the Windows SDK. Official API 26 headers are included. Run `build.cmd` to produce `build/channel_tts_win64.dll`. The C++ runtime is linked statically, so no separate VC runtime DLLs are required.

CMake 3.20 or later is also supported. Run these commands in an x64 developer terminal with Ninja available:

```powershell
cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
```

Visual Studio 2026 requires a CMake version that supports it. To package the result, copy the built DLL into the project root and run `package.ps1`. It packages that DLL and the BOM-free ASCII addon manifest as a `.ts3_plugin` file.

Run `test.cmd` to build and execute six test groups: DLL loading/callbacks and the compact menus, message filtering, UID-based ignored users, the word-list dialog, language persistence and bilingual controls, and the unified settings workflow. UI checks cover sidebar navigation, slider/number synchronization, Apply without closing, Cancel preserving applied changes, Apply-button state, and immediate language refresh. Workflow checks also cover drafts across pages, validation, saving and cancellation, restoring users, defaults, preview/stop, file-lock rollback, and temporary-file cleanup.

Tests briefly display their own native windows using fictional data. They do not operate a running TS3 client. Screenshots are captured from test-owned windows. **Local tests have passed; appearance and interaction inside the actual TS3 client still need verification.**

## Implementation scope and sources

The plugin exports the API 26 C entry points and uses the official TS3 function table. A dedicated worker thread owns COM/SAPI. Only channel text from the currently selected server connection is read; TS3-ignored messages and ignored UIDs are excluded, as are your own messages by default. Connection changes, your own channel moves, kicks, and related events clear old speech.

The queue holds up to 20 pending messages. Message bodies are limited to 500 UTF-16 code units, with filtering performed on the complete original text first. `SPF_IS_NOT_XML` prevents chat text from being interpreted as SAPI XML. The worker checks cancellation every 40 ms. A third-party speech engine that blocks internally can still delay unloading.

- [Official SDK 26](https://github.com/teamspeak/ts3client-pluginsdk/releases/tag/26): bundled headers come from commit `b2f4b8e574d13c584ba8cae0c5ab6e1a0c3048e6`, with their copyright notices preserved.
- [Official plugin and configuration example](https://github.com/teamspeak/ts3client-pluginsdk/blob/26/src/plugin.c): settings request the client's GUI thread and do not depend on Qt.
- [Microsoft SPEAKFLAGS](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms717252(v=vs.85)).

Project-owned source code uses the [MIT License](LICENSE). SDK headers remain subject to their applicable terms.

Historical source and installation packages are preserved in [GitHub Releases](https://github.com/UltraDATY/ts3-channel-tts/releases): 1.0.0 initial release, 1.0.1 Chinese UI, 1.1.0 menus, 1.2.0 word filtering, 1.3.0 ignored users, 1.4.0 word-list window, 1.5.0 language switching, and 1.6.0 the unified settings workflow.
