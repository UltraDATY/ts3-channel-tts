可选测试（项目根目录，x64 Native Tools Command Prompt）：
cl /nologo /std:c++17 /EHsc /MT /Isdk\include tests\smoke.cpp /Fobuild\smoke.obj /Febuild\smoke.exe
build\smoke.exe

先运行 build.cmd，再将 build\channel_tts_win64.dll 复制到项目根目录。
测试使用 Windows 临时目录 channel-tts-smoke 保存测试设置，并以音量 0 调用 SAPI。
它只验证宿主加载、入口和调用生命周期，不代替真实 TS3 或听音验收。
