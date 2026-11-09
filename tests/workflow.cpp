#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
#include "dialog_capture.h"
int workflowAction=0,ticks=0,allocated=0,released=0;
uint64 currentServer(){return 1;}
unsigned int onlineClients(uint64,anyID** list){*list=static_cast<anyID*>(malloc(sizeof(anyID)*2));(*list)[0]=8;(*list)[1]=0;allocated++;return 0;}
unsigned int onlineField(uint64,anyID,size_t field,char** text){*text=_strdup(field==CLIENT_UNIQUE_IDENTIFIER?"botUID=":"Music Bot");allocated++;return 0;}
unsigned int freeSDK(void* data){free(data);released++;return 0;}
void CALLBACK interactWorkflow(HWND,UINT,UINT_PTR timer,DWORD) {
    if(!settingsDialog || !IsWindowVisible(settingsDialog) || settingsState(settingsDialog)->initializing){assert(++ticks<100);return;}
    KillTimer(nullptr,timer);HWND dialog=settingsDialog;
    if(workflowAction==2){
        assert(SendDlgItemMessageW(dialog,UI_RATE_SLIDER,TBM_GETPOS,0,0)==-2);
        assert(SendDlgItemMessageW(dialog,UI_VOLUME_SLIDER,TBM_GETPOS,0,0)==22);
        SendMessageW(dialog,WM_COMMAND,UI_DEFAULTS,0);
        assert(GetDlgItemInt(dialog,UI_VOLUME,nullptr,FALSE)==80 && cfg.volume==22);
        assert(windowText(GetDlgItem(dialog,UI_WORDS)).find(L"新词")!=std::wstring::npos);
        SendDlgItemMessageW(dialog,UI_VOLUME_SLIDER,TBM_SETPOS,TRUE,19);
        SendMessageW(dialog,WM_HSCROLL,TB_THUMBPOSITION,reinterpret_cast<LPARAM>(GetDlgItem(dialog,UI_VOLUME_SLIDER)));
        assert(GetDlgItemInt(dialog,UI_VOLUME,nullptr,FALSE)==19);
        SendMessageW(dialog,WM_COMMAND,UI_PREVIEW,0);
        assert(queue.size()==1 && queue.front().settings.volume==19 && cfg.volume==22);
        SendMessageW(dialog,WM_COMMAND,UI_STOP,0);assert(queue.empty());
        SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;
    }
    if(workflowAction==0){
        wchar_t first[256]{};SendDlgItemMessageW(dialog,UI_IGNORED,LB_GETTEXT,0,reinterpret_cast<LPARAM>(first));assert(wcsstr(first,L"Music Bot"));
        captureDialog(dialog,L"build/settings-general.bmp");
        settingsPage(dialog,1);captureDialog(dialog,L"build/settings-words.bmp");
        settingsPage(dialog,2);captureDialog(dialog,L"build/settings-ignored.bmp");
        settingsPage(dialog,0);
    }
    CheckDlgButton(dialog,UI_ENABLED,BST_UNCHECKED);SetDlgItemInt(dialog,UI_VOLUME,22,FALSE);
    SetDlgItemTextW(dialog,UI_WORDS,L"新词\r\nRobot\r\n");CheckDlgButton(dialog,UI_FILTER,BST_CHECKED);
    SendDlgItemMessageW(dialog,UI_NAV,LB_SETCURSEL,2,0);SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(UI_NAV,LBN_SELCHANGE),0);assert(IsWindowVisible(GetDlgItem(dialog,UI_IGNORED)));
    SendDlgItemMessageW(dialog,UI_IGNORED,LB_SETCURSEL,0,0);SendMessageW(dialog,WM_COMMAND,UI_REMOVE,0);
    assert(ignoredUsers.size()==2 && cfg.enabled && cfg.volume==80 && blockedWords[0]==L"原词");
    if(workflowAction==0){SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);return;}
    SetDlgItemTextW(dialog,UI_RATE,L"1xx");SendMessageW(dialog,WM_COMMAND,IDOK,0);assert(settingsDialog==dialog && cfg.volume==80);
    SetDlgItemInt(dialog,UI_RATE,50,TRUE);SendMessageW(dialog,WM_COMMAND,IDOK,0);assert(settingsDialog==dialog);
    assert(IsWindowVisible(GetDlgItem(dialog,UI_RATE)) && cfg.volume==80);
    SetDlgItemInt(dialog,UI_RATE,static_cast<UINT>(-2),TRUE);SetDlgItemTextW(dialog,UI_WORDS,std::wstring(129,L'a').c_str());
    SendMessageW(dialog,WM_COMMAND,IDOK,0);assert(settingsDialog==dialog && IsWindowVisible(GetDlgItem(dialog,UI_WORDS)));
    SetDlgItemTextW(dialog,UI_WORDS,L"新词\r\nRobot\r\n");SendMessageW(dialog,WM_COMMAND,UI_APPLY,0);
    assert(settingsDialog==dialog && cfg.volume==22 && !IsWindowEnabled(GetDlgItem(dialog,UI_APPLY)));
    SetDlgItemInt(dialog,UI_VOLUME,44,FALSE);assert(IsWindowEnabled(GetDlgItem(dialog,UI_APPLY)));
    SendMessageW(dialog,WM_COMMAND,IDCANCEL,0);
}
std::string bytes(const std::wstring& path){std::ifstream file(std::filesystem::path(path),std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("tts-workflow-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);
    ts.getCurrentServerConnectionHandlerID=currentServer;ts.getClientList=onlineClients;ts.getClientVariableAsString=onlineField;ts.freeMemory=freeSDK;
    ini=(dir/L"settings.ini").wstring();wordPath=(dir/L"words.txt").wstring();cfg=Settings{};blockedWords={L"原词"};ignoredUsers={"botUID=","otherUID="};
    std::ofstream(std::filesystem::path(wordPath),std::ios::binary)<<"\xEF\xBB\xBF原词\r\n";
    std::wstring error;assert(commitDraft(currentDraft(),error));auto originalIni=bytes(ini),originalWords=bytes(wordPath);
    queue.push_back({L"queued",cfg,generation});auto before=generation;
    for(workflowAction=0;workflowAction<3;workflowAction++){
        ticks=0;SetTimer(nullptr,0,100,interactWorkflow);openSettings();assert(!settingsDialog);
        if(workflowAction==0){assert(cfg.volume==80 && cfg.enabled && blockedWords[0]==L"原词" && ignoredUsers.size()==2 && generation==before);assert(bytes(ini)==originalIni && bytes(wordPath)==originalWords);}
        else {assert(cfg.volume==22 && !cfg.enabled && cfg.rate== -2 && cfg.filter && blockedWords[0]==L"新词" && ignoredUsers.size()==1 && queue.empty());}
    }
    cfg=Settings{};blockedWords.clear();ignoredUsers.clear();loadLocked();loadWordsLocked();loadIgnoredLocked();
    assert(cfg.volume==22 && blockedWords[0]==L"新词" && ignoredUsers.size()==1);
    originalIni=bytes(ini);originalWords=bytes(wordPath);before=generation;
    for(const auto& lockedPath:{ini,wordPath}){
        HANDLE file=CreateFileW(lockedPath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
        auto draft=currentDraft();draft.words={L"should rollback"};draft.settings.volume=40;draft.ignored.clear();
        assert(!commitDraft(draft,error));CloseHandle(file);
        assert(cfg.volume==22 && blockedWords[0]==L"新词" && ignoredUsers.size()==1 && generation==before);
        assert(bytes(ini)==originalIni && bytes(wordPath)==originalWords);
    }
    assert(std::distance(std::filesystem::directory_iterator(dir),std::filesystem::directory_iterator{})==2);
    assert(allocated==released);
    std::filesystem::remove_all(dir);
    std::cout<<"PASS: unified pages/draft/save/cancel, numeric and word validation, ignore removal, defaults, preview/stop, full persistence, config/word lock rollback, temp cleanup\n";
}
