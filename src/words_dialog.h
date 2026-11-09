// Included by plugin.cpp after the settings and encoding helpers.
#pragma once
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
HWND wordsDialog=nullptr;
constexpr int WORDS_EDIT=201, WORDS_ENABLED=202, WORDS_STATUS=203;
bool parseEditorWords(const std::wstring& text,std::vector<std::wstring>& words,std::wstring& error) {
    words.clear(); std::wistringstream lines(text); std::wstring line;
    while(std::getline(lines,line)) {
        auto first=line.find_first_not_of(L" \t\r");
        if(first==std::wstring::npos || line[first]==L'#')continue;
        line=line.substr(first,line.find_last_not_of(L" \t\r")-first+1);
        if(line.size()>128){error=wtr(L"每个词最多 128 个字符，请缩短后再保存。");return false;}
        if(words.size()>=1000){error=wtr(L"最多保存 1000 个词，请删减后再保存。");return false;}
        words.push_back(std::move(line));
    }
    return true;
}
bool saveEditorWords(const std::wstring& text,bool enabled,std::wstring& error);
HWND dialogControl(HWND dialog,const wchar_t* cls,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h) {
    RECT r{x,y,x+w,y+h};MapDialogRect(dialog,&r);
    HWND item=CreateWindowExW(wcscmp(cls,L"EDIT")==0?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,r.left,r.top,r.right-r.left,r.bottom-r.top,dialog,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
    if(item)SendMessageW(item,WM_SETFONT,SendMessageW(dialog,WM_GETFONT,0,0),TRUE);
    return item;
}
INT_PTR CALLBACK wordsDialogProc(HWND dialog,UINT message,WPARAM wp,LPARAM) {
    try {
        if(message==WM_INITDIALOG) {
            wordsDialog=dialog;
            SetWindowTextW(dialog,wtr(L"频道文字朗读 — 违禁词设置"));
            dialogControl(dialog,L"BUTTON",wtr(L"启用违禁词过滤"),BS_AUTOCHECKBOX|WS_TABSTOP,WORDS_ENABLED,12,10,260,16);
            dialogControl(dialog,L"STATIC",wtr(L"每行填写一个词；匹配后跳过整条朗读。英文不区分大小写。"),0,-1,12,32,342,22);
            HWND edit=dialogControl(dialog,L"EDIT",L"",WS_TABSTOP|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,WORDS_EDIT,12,57,342,174);
            SendMessageW(edit,EM_SETLIMITTEXT,260000,0);
            std::wstring text;bool enabled;
            {std::lock_guard<std::mutex> lock(mu);enabled=cfg.filter;for(const auto& word:blockedWords)text+=word+L"\r\n";}
            SetWindowTextW(edit,text.c_str());CheckDlgButton(dialog,WORDS_ENABLED,enabled?BST_CHECKED:BST_UNCHECKED);
            dialogControl(dialog,L"STATIC",wtr(L"最多 1000 个词，每词最多 128 个字符。保存后立即生效。"),0,WORDS_STATUS,12,237,342,30);
            dialogControl(dialog,L"BUTTON",wtr(L"保存并应用"),WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,209,275,75,20);
            dialogControl(dialog,L"BUTTON",wtr(L"取消"),WS_TABSTOP,IDCANCEL,292,275,62,20);
            return TRUE;
        }
        if(message==WM_COMMAND && LOWORD(wp)==IDOK) {
            HWND edit=GetDlgItem(dialog,WORDS_EDIT);int length=GetWindowTextLengthW(edit);
            std::wstring text(static_cast<size_t>(length)+1,L'\0');GetWindowTextW(edit,text.data(),length+1);text.resize(length);
            std::wstring error;
            if(saveEditorWords(text,IsDlgButtonChecked(dialog,WORDS_ENABLED)==BST_CHECKED,error))EndDialog(dialog,IDOK);
            else SetDlgItemTextW(dialog,WORDS_STATUS,error.c_str());
            return TRUE;
        }
        if(message==WM_CLOSE || (message==WM_COMMAND && LOWORD(wp)==IDCANCEL)){EndDialog(dialog,IDCANCEL);return TRUE;}
        if(message==WM_DESTROY){wordsDialog=nullptr;return TRUE;}
    }catch(...){SetDlgItemTextW(dialog,WORDS_STATUS,wtr(L"无法应用设置，请重试。原有设置仍然保留。"));}
    return FALSE;
}
INT_PTR showNativeDialog(DLGPROC procedure,short width,short height,LPARAM data=0,WORD fontSize=10) {
    HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&showNativeDialog),&module);
    struct ThemeContext {HANDLE handle=INVALID_HANDLE_VALUE;ULONG_PTR cookie=0;~ThemeContext(){if(cookie)DeactivateActCtx(0,cookie);if(handle!=INVALID_HANDLE_VALUE)ReleaseActCtx(handle);}} theme;
    ACTCTXW context{};context.cbSize=sizeof(context);context.dwFlags=ACTCTX_FLAG_HMODULE_VALID|ACTCTX_FLAG_RESOURCE_NAME_VALID;context.hModule=module;context.lpResourceName=MAKEINTRESOURCEW(101);
    theme.handle=CreateActCtxW(&context);if(theme.handle!=INVALID_HANDLE_VALUE)ActivateActCtx(theme.handle,&theme.cookie);
    // A zero-control template; controls are created in WM_INITDIALOG in dialog units.
    std::vector<DWORD> memory(256,0);
    auto t=reinterpret_cast<DLGTEMPLATE*>(memory.data());
    t->style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_SETFONT|DS_CENTER;
    t->cx=width;t->cy=height;
    auto p=reinterpret_cast<WORD*>(t+1);*p++=0;*p++=0;
    const wchar_t title[]=L"Channel TTS";
    memcpy(p,title,sizeof(title));p+=sizeof(title)/sizeof(WORD);*p++=fontSize;
    const wchar_t font[]=L"Microsoft YaHei UI";memcpy(p,font,sizeof(font));
    HWND owner=GetForegroundWindow();DWORD process=0;GetWindowThreadProcessId(owner,&process);
    if(process!=GetCurrentProcessId())owner=nullptr;
    return DialogBoxIndirectParamW(GetModuleHandleW(nullptr),t,owner,procedure,data);
}
void editWords() {
    if(wordsDialog){SetForegroundWindow(wordsDialog);return;}
    if(showNativeDialog(wordsDialogProc,366,307)==-1 && ts.logMessage)
        ts.logMessage(tr("无法打开违禁词设置窗口。"),LogLevel_WARNING,"Channel TTS",0);
}
