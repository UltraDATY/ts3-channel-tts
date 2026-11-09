#pragma once
HWND settingsDialog=nullptr;
constexpr int UI_LANGUAGE=301, UI_LANGUAGE_NOTE=302;
constexpr int UI_ENABLED=310, UI_SELF=311, UI_NAMES=312, UI_RATE=313, UI_VOLUME=314;
constexpr int UI_WORDS=320, UI_FILTER=321, UI_WORD_COUNT=322, UI_IGNORED=330, UI_REMOVE=331;
constexpr int UI_DEFAULTS=340, UI_STOP=341, UI_PREVIEW=342, UI_APPLY=343;
constexpr int UI_PAGE=350,UI_NAV=360,UI_HEADER=361,UI_RATE_SLIDER=362,UI_VOLUME_SLIDER=363;
struct ControlLabel { HWND control;const wchar_t* zh;const wchar_t* en; };
struct SettingsUI {
    SettingsDraft draft;
    std::vector<HWND> pages[3];
    std::vector<ControlLabel> labels;
    std::vector<std::pair<std::string,std::wstring>> onlineNames;
    int page=0;
    bool initializing=true,previewed=false,dirty=false;
};
SettingsUI* settingsState(HWND dialog){return reinterpret_cast<SettingsUI*>(GetWindowLongPtrW(dialog,GWLP_USERDATA));}
void collectOnlineNames(SettingsUI& state) {
    if(!ts.getCurrentServerConnectionHandlerID || !ts.getClientList || !ts.getClientVariableAsString || !ts.freeMemory)return;
    uint64 server=ts.getCurrentServerConnectionHandlerID();if(!server)return;
    struct ClientList {anyID* value=nullptr;~ClientList(){if(value)ts.freeMemory(value);}} clients;
    if(ts.getClientList(server,&clients.value)!=ERROR_ok || !clients.value)return;
    auto get=[&](anyID client,size_t field){
        struct Text {char* value=nullptr;~Text(){if(value)ts.freeMemory(value);}} text;
        if(ts.getClientVariableAsString(server,client,field,&text.value)!=ERROR_ok || !text.value)return std::string{};
        return std::string(text.value);
    };
    for(size_t i=0;i<65536 && clients.value[i];i++){
        auto uid=get(clients.value[i],CLIENT_UNIQUE_IDENTIFIER);
        if(state.draft.ignored.count(uid))state.onlineNames.emplace_back(uid,wide(get(clients.value[i],CLIENT_NICKNAME).c_str()));
    }
}
void settingsPage(HWND dialog,int page) {
    auto state=settingsState(dialog);if(!state)return;
    state->page=std::clamp(page,0,2);SendDlgItemMessageW(dialog,UI_NAV,LB_SETCURSEL,state->page,0);
    for(int i=0;i<3;i++)for(HWND control:state->pages[i])ShowWindow(control,i==state->page?SW_SHOW:SW_HIDE);
    InvalidateRect(GetDlgItem(dialog,UI_HEADER),nullptr,TRUE);
}
void settingsNotice(HWND dialog,const wchar_t* zh,const wchar_t* en){SetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,uiText(zh,en));}
void markSettingsDirty(HWND dialog){auto state=settingsState(dialog);if(!state || state->initializing)return;state->dirty=true;EnableWindow(GetDlgItem(dialog,UI_APPLY),TRUE);}
void fillSpeechControls(HWND dialog,const Settings& c) {
    CheckDlgButton(dialog,UI_ENABLED,c.enabled?BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(dialog,UI_SELF,c.self?BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(dialog,UI_NAMES,c.names?BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(dialog,UI_FILTER,c.filter?BST_CHECKED:BST_UNCHECKED);
    SendDlgItemMessageW(dialog,UI_RATE_SLIDER,TBM_SETPOS,TRUE,c.rate);SendDlgItemMessageW(dialog,UI_VOLUME_SLIDER,TBM_SETPOS,TRUE,c.volume);
    SetDlgItemInt(dialog,UI_RATE,c.rate,TRUE);SetDlgItemInt(dialog,UI_VOLUME,c.volume,FALSE);
}
bool readSpeechControls(HWND dialog,Settings& c) {
    auto number=[&](int id,int& value){wchar_t text[16]{};GetDlgItemTextW(dialog,id,text,16);wchar_t* end=nullptr;long parsed=wcstol(text,&end,10);if(end==text || *end)return false;value=static_cast<int>(parsed);return true;};
    int rate=0,volume=0;
    if(!number(UI_RATE,rate) || !number(UI_VOLUME,volume) || rate < -10 || rate>10 || volume<0 || volume>100) {
        settingsPage(dialog,0);settingsNotice(dialog,L"请输入有效数值：语速 -10～10，音量 0～100。",L"Enter valid values: rate -10 to 10; volume 0 to 100.");return false;
    }
    c.enabled=IsDlgButtonChecked(dialog,UI_ENABLED)==BST_CHECKED;
    c.self=IsDlgButtonChecked(dialog,UI_SELF)==BST_CHECKED;
    c.names=IsDlgButtonChecked(dialog,UI_NAMES)==BST_CHECKED;
    c.filter=IsDlgButtonChecked(dialog,UI_FILTER)==BST_CHECKED;
    c.rate=rate;c.volume=volume;return true;
}
std::wstring windowText(HWND control) {
    int length=GetWindowTextLengthW(control);std::wstring text(static_cast<size_t>(length)+1,L'\0');
    GetWindowTextW(control,text.data(),length+1);text.resize(length);return text;
}
void updateWordCount(HWND dialog) {
    std::vector<std::wstring> words;std::wstring error;
    if(!parseEditorWords(windowText(GetDlgItem(dialog,UI_WORDS)),words,error)){SetDlgItemTextW(dialog,UI_WORD_COUNT,error.c_str());return;}
    auto text=std::to_wstring(words.size())+uiText(L" / 1000 个词；每词最多 128 个字符。",L" / 1000 entries; max. 128 UTF-16 units each.");
    SetDlgItemTextW(dialog,UI_WORD_COUNT,text.c_str());
}
void fillIgnoredList(HWND dialog) {
    auto state=settingsState(dialog);HWND list=GetDlgItem(dialog,UI_IGNORED);
    SendMessageW(list,LB_RESETCONTENT,0,0);
    for(const auto& uid:state->draft.ignored){
        auto text=wide(uid.c_str());
        for(const auto& name:state->onlineNames)if(name.first==uid && !name.second.empty()){text=name.second+L" [UID: "+text+L"]";break;}
        SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));
    }
    if(!state->draft.ignored.empty())SendMessageW(list,LB_SETCURSEL,0,0);
    EnableWindow(GetDlgItem(dialog,UI_REMOVE),!state->draft.ignored.empty());
}
void refreshSettingsLanguage(HWND dialog) {
    auto state=settingsState(dialog);
    state->initializing=true;
    SetWindowTextW(dialog,uiText(L"频道文字朗读 — 选项",L"Channel TTS - Options"));
    for(const auto& label:state->labels)SetWindowTextW(label.control,uiText(label.zh,label.en));
    HWND nav=GetDlgItem(dialog,UI_NAV);SendMessageW(nav,LB_RESETCONTENT,0,0);
    const wchar_t* titles[]={uiText(L"朗读设置",L"Speech"),uiText(L"违禁词",L"Word filter"),uiText(L"忽略名单",L"Ignored users")};
    for(const auto* title:titles)SendMessageW(nav,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(title));
    SendMessageW(nav,LB_SETCURSEL,state->page,0);updateWordCount(dialog);
    InvalidateRect(GetDlgItem(dialog,UI_HEADER),nullptr,TRUE);state->initializing=false;
}
#include "settings_paint.h"
INT_PTR CALLBACK settingsDialogProc(HWND dialog,UINT message,WPARAM wp,LPARAM lp) {
    try {
        if(message==WM_INITDIALOG) {
            INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_BAR_CLASSES};InitCommonControlsEx(&controls);
            auto state=new SettingsUI;SetWindowLongPtrW(dialog,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));
            state->draft=currentDraft();collectOnlineNames(*state);settingsDialog=dialog;
            auto add=[&](int page,const wchar_t* cls,const wchar_t* zh,const wchar_t* en,DWORD style,int id,int x,int y,int w,int h){
                HWND item=dialogControl(dialog,cls,uiText(zh,en),style,id,x,y,w,h);
                if(page>=0)state->pages[page].push_back(item);
                if(*zh || *en)state->labels.push_back({item,zh,en});return item;
            };
            HWND nav=add(-1,L"LISTBOX",L"",L"",LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|WS_TABSTOP|WS_BORDER,UI_NAV,6,8,80,284);
            RECT row{0,0,80,22};MapDialogRect(dialog,&row);SendMessageW(nav,LB_SETITEMHEIGHT,0,row.bottom);
            add(-1,L"STATIC",L"",L"",SS_OWNERDRAW,UI_HEADER,92,8,346,25);
            auto group=[&](int page,const wchar_t* zh,const wchar_t* en,int y,int h){return add(page,L"BUTTON",zh,en,BS_GROUPBOX,-1,98,y,334,h);};
            group(0,L"朗读选项",L"Speech options",42,74);
            add(0,L"BUTTON",L"开启频道文字朗读",L"Enable channel text-to-speech",BS_AUTOCHECKBOX|WS_TABSTOP,UI_ENABLED,108,57,314,16);
            add(0,L"BUTTON",L"朗读自己的消息",L"Read my own messages",BS_AUTOCHECKBOX|WS_TABSTOP,UI_SELF,108,76,314,16);
            add(0,L"BUTTON",L"先朗读发送者昵称",L"Read the sender's name first",BS_AUTOCHECKBOX|WS_TABSTOP,UI_NAMES,108,95,314,16);
            group(0,L"语音调节",L"Voice adjustment",122,95);
            add(0,L"STATIC",L"语速（-10～10）",L"Rate (-10 to 10)",0,-1,108,135,314,12);
            HWND rateSlider=add(0,TRACKBAR_CLASSW,L"",L"",WS_TABSTOP|TBS_AUTOTICKS,UI_RATE_SLIDER,106,149,262,20);
            SendMessageW(rateSlider,TBM_SETRANGEMIN,FALSE,-10);SendMessageW(rateSlider,TBM_SETRANGEMAX,FALSE,10);SendMessageW(rateSlider,TBM_SETTICFREQ,5,0);
            HWND rate=add(0,L"EDIT",L"",L"",WS_TABSTOP|ES_AUTOHSCROLL|ES_RIGHT,UI_RATE,378,148,44,18);SendMessageW(rate,EM_SETLIMITTEXT,4,0);
            add(0,L"STATIC",L"音量（0～100）",L"Volume (0 to 100)",0,-1,108,173,314,12);
            HWND volumeSlider=add(0,TRACKBAR_CLASSW,L"",L"",WS_TABSTOP|TBS_AUTOTICKS,UI_VOLUME_SLIDER,106,187,262,20);
            SendMessageW(volumeSlider,TBM_SETRANGE,FALSE,MAKELONG(0,100));SendMessageW(volumeSlider,TBM_SETTICFREQ,20,0);
            HWND volume=add(0,L"EDIT",L"",L"",WS_TABSTOP|ES_AUTOHSCROLL|ES_RIGHT,UI_VOLUME,378,186,44,18);SendMessageW(volume,EM_SETLIMITTEXT,4,0);
            add(0,L"BUTTON",L"试听",L"Preview",WS_TABSTOP,UI_PREVIEW,108,223,66,19);
            add(0,L"BUTTON",L"停止朗读",L"Stop speech",WS_TABSTOP,UI_STOP,182,223,76,19);
            add(0,L"BUTTON",L"恢复朗读默认值",L"Speech defaults",WS_TABSTOP,UI_DEFAULTS,266,223,156,19);
            add(0,L"STATIC",L"界面语言 / Language",L"Language / 界面语言",0,-1,108,253,118,14);
            HWND combo=add(0,L"COMBOBOX",L"",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,UI_LANGUAGE,232,249,190,80);
            SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"简体中文"));SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));SendMessageW(combo,CB_SETCURSEL,state->draft.english?1:0,0);
            group(1,L"过滤规则",L"Filter rules",42,222);
            add(1,L"BUTTON",L"启用违禁词过滤",L"Enable word filter",BS_AUTOCHECKBOX|WS_TABSTOP,UI_FILTER,108,57,314,16);
            add(1,L"STATIC",L"每行一个词。命中后跳过整条朗读；英文不区分大小写。",L"One word per line. Matching messages are skipped; English ignores case.",0,-1,108,79,314,25);
            HWND editor=add(1,L"EDIT",L"",L"",WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,UI_WORDS,108,109,314,119);
            SendMessageW(editor,EM_SETLIMITTEXT,260000,0);std::wstring words;for(const auto& word:state->draft.words)words+=word+L"\r\n";SetWindowTextW(editor,words.c_str());
            add(1,L"STATIC",L"",L"",0,UI_WORD_COUNT,108,236,314,22);
            group(2,L"不朗读这些用户的文字",L"Users excluded from speech",42,222);
            add(2,L"STATIC",L"右键频道用户可加入名单。选中用户并恢复，点击应用后生效。在线用户显示昵称，离线使用 UID；改名不影响屏蔽。",L"Right-click a channel user to ignore them. Restore a user below, then apply. Online users show names; others show UIDs. Renaming keeps the block.",0,-1,108,57,314,39);
            add(2,L"LISTBOX",L"",L"",LBS_NOTIFY|WS_VSCROLL|WS_HSCROLL|WS_TABSTOP|WS_BORDER,UI_IGNORED,108,103,314,125);
            SendDlgItemMessageW(dialog,UI_IGNORED,LB_SETHORIZONTALEXTENT,1200,0);
            add(2,L"BUTTON",L"恢复选中用户的朗读",L"Restore selected user's speech",WS_TABSTOP,UI_REMOVE,108,237,220,20);
            add(-1,L"STATIC",L"",L"",0,UI_LANGUAGE_NOTE,98,273,334,19);
            add(-1,L"STATIC",L"",L"",SS_ETCHEDHORZ,-1,6,292,432,1);
            add(-1,L"BUTTON",L"确定",L"OK",WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,270,300,52,17);
            add(-1,L"BUTTON",L"取消",L"Cancel",WS_TABSTOP,IDCANCEL,328,300,52,17);
            add(-1,L"BUTTON",L"应用",L"Apply",WS_TABSTOP,UI_APPLY,386,300,52,17);
            fillSpeechControls(dialog,state->draft.settings);fillIgnoredList(dialog);settingsPage(dialog,static_cast<int>(lp));refreshSettingsLanguage(dialog);
            EnableWindow(GetDlgItem(dialog,UI_APPLY),FALSE);state->dirty=false;
            settingsNotice(dialog,L"确定保存并关闭；应用保存后继续编辑；取消放弃未应用的修改。",L"OK saves and closes; Apply saves and stays open; Cancel discards unapplied edits.");
            return TRUE;
        }
        auto state=settingsState(dialog);
        if(message==WM_MEASUREITEM && wp==UI_NAV){auto item=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);item->itemHeight=34;return TRUE;}
        if(message==WM_DRAWITEM && state)return drawSettingsItem(dialog,reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        if(message==WM_HSCROLL && state){int id=GetDlgCtrlID(reinterpret_cast<HWND>(lp));if(id==UI_RATE_SLIDER || id==UI_VOLUME_SLIDER){int value=static_cast<int>(SendMessageW(reinterpret_cast<HWND>(lp),TBM_GETPOS,0,0));SetDlgItemInt(dialog,id==UI_RATE_SLIDER?UI_RATE:UI_VOLUME,static_cast<UINT>(value),TRUE);markSettingsDirty(dialog);return TRUE;}}
        if(message==WM_COMMAND && state){
            int id=LOWORD(wp);
            if(id==UI_NAV && HIWORD(wp)==LBN_SELCHANGE){settingsPage(dialog,static_cast<int>(SendDlgItemMessageW(dialog,UI_NAV,LB_GETCURSEL,0,0)));return TRUE;}
            if(id>=UI_PAGE && id<UI_PAGE+3){settingsPage(dialog,id-UI_PAGE);return TRUE;}
            if(id==IDOK || id==UI_APPLY){
                auto draft=state->draft;if(!readSpeechControls(dialog,draft.settings))return TRUE;std::wstring error;
                if(!parseEditorWords(windowText(GetDlgItem(dialog,UI_WORDS)),draft.words,error)){settingsPage(dialog,1);SetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,error.c_str());return TRUE;}
                auto selection=SendDlgItemMessageW(dialog,UI_LANGUAGE,CB_GETCURSEL,0,0);if(selection<0 || selection>1)return TRUE;draft.english=selection==1;
                if(commitDraft(draft,error)){
                    if(id==IDOK)EndDialog(dialog,IDOK);
                    else{state->draft=std::move(draft);refreshSettingsLanguage(dialog);state->dirty=false;EnableWindow(GetDlgItem(dialog,UI_APPLY),FALSE);settingsNotice(dialog,L"已应用。可继续编辑，或点击确定关闭。",L"Applied. Continue editing or choose OK to close.");}
                }else SetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,error.c_str());return TRUE;
            }
            if(id==UI_REMOVE){auto selected=SendDlgItemMessageW(dialog,UI_IGNORED,LB_GETCURSEL,0,0);if(selected!=LB_ERR && static_cast<size_t>(selected)<state->draft.ignored.size()){auto it=state->draft.ignored.begin();std::advance(it,selected);state->draft.ignored.erase(it);fillIgnoredList(dialog);markSettingsDirty(dialog);settingsNotice(dialog,L"已从待应用名单移除，点击确定或应用后恢复朗读。",L"Removed from the draft list. Choose OK or Apply to restore speech.");}return TRUE;}
            if(id==UI_DEFAULTS){fillSpeechControls(dialog,Settings{});markSettingsDirty(dialog);settingsNotice(dialog,L"已填入朗读默认值，应用后生效；词库、名单和语言保留。",L"Speech defaults filled in. Apply to save; words, users and language are retained.");return TRUE;}
            if(id==UI_STOP){std::lock_guard<std::mutex> lock(mu);clearLocked();settingsNotice(dialog,L"已停止当前朗读，后续消息按已保存开关处理。",L"Current speech stopped. New messages follow the saved enable/disable setting.");return TRUE;}
            if(id==UI_PREVIEW){Settings sample;if(!readSpeechControls(dialog,sample))return TRUE;std::lock_guard<std::mutex> lock(mu);clearLocked();queue.push_back({uiText(L"这是频道文字朗读试听。",L"This is a Channel TTS speech preview."),sample,generation});cv.notify_one();state->previewed=true;settingsNotice(dialog,L"正在试听；没有保存修改。",L"Preview queued; edits have not been saved.");return TRUE;}
            if(id==UI_WORDS && HIWORD(wp)==EN_CHANGE)updateWordCount(dialog);
            if((id==UI_RATE || id==UI_VOLUME) && HIWORD(wp)==EN_CHANGE){BOOL valid=FALSE;int value=static_cast<int>(GetDlgItemInt(dialog,id,&valid,TRUE));if(valid && value>=(id==UI_RATE?-10:0) && value<=(id==UI_RATE?10:100))SendDlgItemMessageW(dialog,id==UI_RATE?UI_RATE_SLIDER:UI_VOLUME_SLIDER,TBM_SETPOS,TRUE,value);}
            if(!state->initializing && (HIWORD(wp)==EN_CHANGE || (id==UI_LANGUAGE && HIWORD(wp)==CBN_SELCHANGE) || (HIWORD(wp)==BN_CLICKED && (id==UI_ENABLED || id==UI_SELF || id==UI_NAMES || id==UI_FILTER)))){
                markSettingsDirty(dialog);
                if(id==UI_LANGUAGE)settingsNotice(dialog,L"语言在应用后切换；用户右键菜单重启客户端后刷新。",L"Apply to switch language. Restart TS3 to refresh user menu labels.");
                else settingsNotice(dialog,L"有尚未应用的修改。确定或应用将保存全部页面。",L"Unapplied changes. OK or Apply saves all pages.");
            }
        }
        if(message==WM_CLOSE || (message==WM_COMMAND && LOWORD(wp)==IDCANCEL)){EndDialog(dialog,IDCANCEL);return TRUE;}
        if(message==WM_DESTROY){if(state && state->previewed){std::lock_guard<std::mutex> lock(mu);clearLocked();}delete state;SetWindowLongPtrW(dialog,GWLP_USERDATA,0);settingsDialog=nullptr;return TRUE;}
    }catch(...){settingsNotice(dialog,L"无法应用设置，请重试。",L"Cannot apply settings. Please try again.");}
    return FALSE;
}
void openSettings(int page=0){
    if(settingsDialog){settingsPage(settingsDialog,page);SetForegroundWindow(settingsDialog);return;}
    if(wordsDialog){SetForegroundWindow(wordsDialog);return;}
    if(showNativeDialog(settingsDialogProc,444,322,page,9)==-1 && ts.logMessage)ts.logMessage(englishUI.load()?"Cannot open settings window.":"无法打开设置窗口。",LogLevel_WARNING,"Channel TTS",0);
}
