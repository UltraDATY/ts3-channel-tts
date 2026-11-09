#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
int scenario=0, ticks=0;
void CALLBACK drive(HWND,UINT,UINT_PTR timer,DWORD) {
    if(!wordsDialog || !IsWindowVisible(wordsDialog)){assert(++ticks<100);return;}
    HWND dialog=wordsDialog;
    assert(GetDlgItem(dialog,WORDS_EDIT) && GetDlgItem(dialog,WORDS_ENABLED));
    assert(GetDlgItem(dialog,IDOK) && GetDlgItem(dialog,IDCANCEL));
    KillTimer(nullptr,timer);
    SetDlgItemTextW(dialog,WORDS_EDIT,scenario==2?std::wstring(129,L'a').c_str():L"  测试禁词  \r\nRobot\r\n\r\n# comment\r\n");
    CheckDlgButton(dialog,WORDS_ENABLED,BST_CHECKED);
    if(scenario==1){SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;}
    SendMessageW(dialog,WM_COMMAND,IDOK,0);
    if(scenario==2 || scenario==3){
        assert(wordsDialog==dialog);wchar_t status[256]{};GetDlgItemTextW(dialog,WORDS_STATUS,status,256);
        assert(wcsstr(status,scenario==2?L"128":L"失败"));
        SendMessageW(dialog,WM_CLOSE,0,0);
    }
}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("tts-dialog-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);
    ini=(dir/L"settings.ini").wstring();wordPath=(dir/L"words.txt").wstring();quitting=false;
    for(scenario=0;scenario<4;++scenario){
        blockedWords={L"original"};cfg.filter=false;queue.push_back({L"queued",cfg,generation});
        if(scenario==3){std::ofstream(wordPath).put('x');}
        HANDLE locked=scenario==3?CreateFileW(wordPath.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr):INVALID_HANDLE_VALUE;
        SetTimer(nullptr,0,100,drive);editWords();assert(!wordsDialog);
        if(scenario==0){
            assert(cfg.filter && blockedWords.size()==2 && blockedWords[0]==L"测试禁词" && queue.empty());
            blockedWords.clear();loadWordsLocked();assert(blockedWords.size()==2 && blockedWords[0]==L"测试禁词");
            cfg.filter=false;loadLocked();assert(cfg.filter);
        }else {assert(!cfg.filter && blockedWords.size()==1 && blockedWords[0]==L"original" && !queue.empty());}
        if(locked!=INVALID_HANDLE_VALUE)CloseHandle(locked);
    }
    std::wstring error;std::vector<std::wstring> parsed;
    std::wstring many;for(int i=0;i<1001;i++)many+=L"word\n";
    assert(!parseEditorWords(many,parsed,error));
    assert(saveEditorWords(L"",false,error) && blockedWords.empty() && !cfg.filter);
    loadWordsLocked();assert(blockedWords.empty());
    std::filesystem::remove_all(dir);
    std::cout<<"PASS: real dialog controls, Chinese edit/save/reload, checkbox persistence, cancel, overlength/word cap, locked-file failure preserves state, empty list, queue cancellation\n";
}
