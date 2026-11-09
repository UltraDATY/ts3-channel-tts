#pragma once
HWND settingsDialog=nullptr;
constexpr int UI_LANGUAGE=301, UI_LANGUAGE_NOTE=302;
INT_PTR CALLBACK settingsDialogProc(HWND dialog,UINT message,WPARAM wp,LPARAM) {
    try {
        if(message==WM_INITDIALOG) {
            settingsDialog=dialog;
            SetWindowTextW(dialog,englishUI.load()?L"Channel TTS - Settings":L"频道文字朗读 — 设置");
            dialogControl(dialog,L"STATIC",L"界面语言 / Interface language",0,-1,12,12,310,16);
            HWND combo=dialogControl(dialog,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,UI_LANGUAGE,12,34,310,80);
            SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"简体中文"));
            SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"English"));
            SendMessageW(combo,CB_SETCURSEL,englishUI.load()?1:0,0);
            dialogControl(dialog,L"STATIC",englishUI.load()
                ?L"Changes apply to plugin windows and messages after saving. Restart TS3 to refresh existing menu labels. Speech voice is unchanged."
                :L"保存后，插件窗口和提示使用所选语言。现有 TS3 菜单文字在重启客户端后更新。此选项不改变朗读声音。",
                0,UI_LANGUAGE_NOTE,12,66,310,48);
            dialogControl(dialog,L"BUTTON",wtr(L"保存并应用"),WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,177,125,75,20);
            dialogControl(dialog,L"BUTTON",wtr(L"取消"),WS_TABSTOP,IDCANCEL,260,125,62,20);
            return TRUE;
        }
        if(message==WM_COMMAND && LOWORD(wp)==IDOK) {
            auto selection=SendDlgItemMessageW(dialog,UI_LANGUAGE,CB_GETCURSEL,0,0);
            if(selection!=0 && selection!=1)return TRUE;
            if(saveUILanguage(selection==1))EndDialog(dialog,IDOK);
            else SetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,englishUI.load()
                ?L"Cannot save the language. Previous choice retained. Check settings file permissions."
                :L"语言保存失败，原选择仍然保留。请检查配置文件是否可写。");
            return TRUE;
        }
        if(message==WM_CLOSE || (message==WM_COMMAND && LOWORD(wp)==IDCANCEL)){EndDialog(dialog,IDCANCEL);return TRUE;}
        if(message==WM_DESTROY){settingsDialog=nullptr;return TRUE;}
    }catch(...){SetDlgItemTextW(dialog,UI_LANGUAGE_NOTE,englishUI.load()?L"Cannot apply settings. Please try again.":L"无法应用设置，请重试。");}
    return FALSE;
}
void openSettings() {
    if(settingsDialog){SetForegroundWindow(settingsDialog);return;}
    // Do not start a second modal settings loop from a nested host callback.
    if(wordsDialog){SetForegroundWindow(wordsDialog);return;}
    if(showNativeDialog(settingsDialogProc,334,157)==-1 && ts.logMessage)
        ts.logMessage(englishUI.load()?"Cannot open settings window.":"无法打开设置窗口。",LogLevel_WARNING,"Channel TTS",0);
}
