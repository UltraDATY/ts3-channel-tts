#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
#include "dialog_capture.h"
void CALLBACK cloudDialogTest(HWND,UINT,UINT_PTR timer,DWORD){
    if(!settingsDialog || !IsWindowVisible(settingsDialog) || settingsState(settingsDialog)->initializing)return;
    KillTimer(nullptr,timer);HWND dialog=settingsDialog;
    settingsPage(dialog,3);assert(IsWindowVisible(GetDlgItem(dialog,UI_CLOUD_KEY)));
    assert(GetWindowLongPtrW(GetDlgItem(dialog,UI_CLOUD_KEY),GWL_STYLE)&ES_PASSWORD);
    CheckDlgButton(dialog,UI_CLOUD_ENABLE,BST_CHECKED);
    SetDlgItemTextW(dialog,UI_REGION,L"eastasia");SetDlgItemTextW(dialog,UI_CLOUD_KEY,L"0123456789abcdef0123456789abcdef");
    captureDialog(dialog,L"build/settings-cloud.bmp");
    SendMessageW(dialog,WM_COMMAND,UI_APPLY,0);assert(settingsDialog==dialog && cfg.online);
    SetDlgItemTextW(dialog,UI_CLOUD_KEY,L"bad\r\nInjected");SendMessageW(dialog,WM_COMMAND,IDOK,0);
    assert(settingsDialog==dialog && cfg.cloudKey==L"0123456789abcdef0123456789abcdef");
    SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);
}
int main(){
    Settings c;c.online=true;c.cloudKey=L"0123456789abcdef0123456789abcdef";assert(validCloudSettings(c));
    c.region=L"eastasia.evil.example";assert(!validCloudSettings(c));c.region=L"eastasia";
    c.cloudVoice=L"voice' injected";assert(!validCloudSettings(c));c.cloudVoice=L"en-US-EmmaMultilingualNeural";
    auto body=cloudSSML(c,L"你好 hello こんにちは <voice>&\"'");
    assert(body.find("&lt;voice&gt;&amp;&quot;&apos;")!=std::string::npos && body.find("こんにちは")!=std::string::npos);
    assert(body.find("0123456789abcdef")==std::string::npos);
    std::wstring encrypted;assert(protectCloudKey(c.cloudKey,encrypted) && encrypted.find(c.cloudKey)==std::wstring::npos && unprotectCloudKey(encrypted)==c.cloudKey);
    assert(unprotectCloudKey(L"xyz").empty() && unprotectCloudKey(L"00").empty());
    std::vector<unsigned char> wav(48,0);memcpy(wav.data(),"RIFF",4);memcpy(wav.data()+8,"WAVEfmt ",8);
    auto u32=[&](size_t p,DWORD v){memcpy(wav.data()+p,&v,4);};auto u16=[&](size_t p,WORD v){memcpy(wav.data()+p,&v,2);};
    u32(4,40);u32(16,16);u16(20,1);u16(22,1);u32(24,24000);u32(28,48000);u16(32,2);u16(34,16);memcpy(wav.data()+36,"data",4);u32(40,4);
    DWORD duration=0;assert(waveDuration(wav,duration));u32(40,999999);assert(!waveDuration(wav,duration));
    std::vector<unsigned char> audio;std::wstring error;assert(!fetchCloudAudio(c,L"test",audio,error,[]{return true;}));
    auto dir=std::filesystem::temp_directory_path()/("tts-cloud-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);
    ini=(dir/L"settings.ini").wstring();wordPath=(dir/L"words.txt").wstring();quitting=false;
    SetTimer(nullptr,0,100,cloudDialogTest);openSettings(3);
    cfg=Settings{};loadLocked();assert(cfg.online && cfg.cloudKey==c.cloudKey);
    std::ifstream file(std::filesystem::path(ini),std::ios::binary);std::string content{std::istreambuf_iterator<char>(file),{}};file.close();assert(content.find("0123456789abcdef")==std::string::npos);
    auto draft=currentDraft();draft.settings.online=false;assert(commitDraft(draft,error));cfg=Settings{};loadLocked();assert(!cfg.online && cfg.cloudKey==c.cloudKey);
    std::filesystem::remove_all(dir);
    std::cout<<"PASS: cloud configuration UI/apply/cancel, DPAPI roundtrip/no plaintext, region/voice/header validation, SSML escaping/Japanese, WAV bounds, cancellation, engine persistence. No live Azure call performed.\n";
}
