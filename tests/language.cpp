#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
int action=0,ticks=0;
void CALLBACK interact(HWND,UINT,UINT_PTR timer,DWORD) {
    HWND dialog=action==3?wordsDialog:settingsDialog;
    if(!dialog){assert(++ticks<100);return;}
    KillTimer(nullptr,timer);
    wchar_t text[256]{};
    if(action==3){
        GetWindowTextW(dialog,text,256);assert(wcsstr(text,L"Word filter settings"));
        GetDlgItemTextW(dialog,WORDS_ENABLED,text,256);assert(wcscmp(text,L"Enable word filter")==0);
        GetDlgItemTextW(dialog,IDOK,text,256);assert(wcscmp(text,L"Save and apply")==0);
        SetDlgItemTextW(dialog,WORDS_EDIT,std::wstring(129,L'a').c_str());SendMessageW(dialog,WM_COMMAND,IDOK,0);
        GetDlgItemTextW(dialog,WORDS_STATUS,text,256);assert(wcsstr(text,L"128 UTF-16 units"));
        SendMessageW(dialog,WM_CLOSE,0,0);return;
    }
    assert(GetDlgItem(dialog,UI_LANGUAGE));
    assert(SendDlgItemMessageW(dialog,UI_LANGUAGE,CB_GETCOUNT,0,0)==2);
    auto current=SendDlgItemMessageW(dialog,UI_LANGUAGE,CB_GETCURSEL,0,0);
    assert(current==(englishUI.load()?1:0));
    GetDlgItemTextW(dialog,IDOK,text,256);
    assert(wcscmp(text,englishUI.load()?L"Save and apply":L"保存并应用")==0);
    SendDlgItemMessageW(dialog,UI_LANGUAGE,CB_SETCURSEL,action==2?0:1,0);
    SendMessageW(dialog,WM_COMMAND,action==0?IDCANCEL:IDOK,0);
    if(action==4){
        assert(settingsDialog==dialog);GetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,text,256);assert(wcsstr(text,L"失败"));
        SendMessageW(dialog,WM_CLOSE,0,0);
    }
}
void dialogAction(int next) {
    action=next;ticks=0;SetTimer(nullptr,0,100,interact);
    if(action==3)editWords();else openSettings();
    assert(!settingsDialog && !wordsDialog);
}
void checkMenus(bool english) {
    PluginMenuItem** items=nullptr;char* icon=nullptr;ts3plugin_initMenus(&items,&icon);assert(items);
    assert(strcmp(items[0]->text,english?"Enable speech":"开启朗读")==0);
    assert(items[16]->id==17 && strstr(items[16]->text,"Settings"));
    assert(items[17]->id==101 && items[17]->type==PLUGIN_MENU_TYPE_CLIENT);
    assert(!items[20]);for(int i=0;i<20;i++)ts3plugin_freeMemory(items[i]);ts3plugin_freeMemory(items);
}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("tts-language-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);
    ini=(dir/L"settings.ini").wstring();wordPath=(dir/L"words.txt").wstring();loadUILanguage();assert(!englishUI.load());
    cfg.volume=33;cfg.filter=true;blockedWords={L"保留词条"};ignoredUsers.insert("botUID=");
    auto before=generation;
    dialogAction(0);assert(!englishUI.load());
    dialogAction(1);assert(englishUI.load());
    englishUI.store(false);loadUILanguage();assert(englishUI.load());checkMenus(true);
    assert(strcmp(tr("是"),"Yes")==0 && strcmp(tr("；语速："),"; Rate: ")==0);
    dialogAction(3);assert(blockedWords.size()==1 && blockedWords[0]==L"保留词条");
    dialogAction(2);assert(!englishUI.load());loadUILanguage();checkMenus(false);
    assert(cfg.volume==33 && cfg.filter && ignoredUsers.count("botUID=") && generation==before);
    WritePrivateProfileStringW(L"ui",L"language",L"unsupported",ini.c_str());loadUILanguage();assert(!englishUI.load());
    ini=(dir/L"missing"/L"settings.ini").wstring();dialogAction(4);assert(!englishUI.load());
    assert(ts3plugin_offersConfigure()==PLUGIN_OFFERS_CONFIGURE_QT_THREAD);
    std::filesystem::remove_all(dir);
    std::cout<<"PASS: language dialog selection/save/cancel, Chinese/English controls/errors/menus, persistence, fallback, save failure, unchanged voice/filter/ignore settings, configure offer\n";
}
