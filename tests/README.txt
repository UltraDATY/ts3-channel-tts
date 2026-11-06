可选测试（项目根目录，x64 Native Tools Command Prompt）：
cl /nologo /std:c++17 /utf-8 /EHsc /MT /Isdk\include tests\smoke.cpp /Fobuild\smoke.obj /Febuild\smoke.exe
build\smoke.exe

先运行 build.cmd，再将 build\channel_tts_win64.dll 复制到项目根目录。
测试使用 Windows 临时目录 channel-tts-smoke 保存测试设置，并以音量 0 调用 SAPI。
它只验证宿主加载、入口和调用生命周期，不代替真实 TS3 或听音验收。


过滤专项测试（x64 开发者命令行，在项目根目录）：
cl /nologo /std:c++17 /utf-8 /EHsc /MT /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Isdk\include tests\filter.cpp /Fobuild\filter.obj /Febuild\filter.exe /link sapi.lib ole32.lib uuid.lib
build\filter.exe

测试隔离在 Windows 临时目录下，直接验证真实消息回调的排队行为和词库加载，不播放语音。
