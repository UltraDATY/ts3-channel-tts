#pragma once
void drawSettingsIcon(HDC dc,RECT area,int kind) {
    const int size=std::min(area.right-area.left,area.bottom-area.top)-8;
    int x=area.left+4,y=area.top+(area.bottom-area.top-size)/2;
    HBRUSH blue=CreateSolidBrush(RGB(77,113,148)),white=CreateSolidBrush(RGB(255,255,255));
    HPEN pen=CreatePen(PS_SOLID,2,RGB(255,255,255));
    auto oldBrush=SelectObject(dc,blue);auto oldPen=SelectObject(dc,GetStockObject(NULL_PEN));
    Ellipse(dc,x,y,x+size,y+size);SelectObject(dc,white);SelectObject(dc,pen);
    auto px=[&](int n){return x+MulDiv(n,size,24);};auto py=[&](int n){return y+MulDiv(n,size,24);};
    if(kind==0){
        POINT speaker[]={{px(5),py(9)},{px(9),py(9)},{px(14),py(5)},{px(14),py(19)},{px(9),py(15)},{px(5),py(15)}};Polygon(dc,speaker,6);
        SelectObject(dc,GetStockObject(NULL_BRUSH));Arc(dc,px(12),py(6),px(21),py(18),px(17),py(6),px(17),py(18));
    }else if(kind==1){
        SelectObject(dc,GetStockObject(NULL_BRUSH));POINT shield[]={{px(5),py(6)},{px(12),py(4)},{px(19),py(6)},{px(17),py(15)},{px(12),py(20)},{px(7),py(15)}};Polygon(dc,shield,6);
        MoveToEx(dc,px(12),py(8),nullptr);LineTo(dc,px(12),py(13));SetPixel(dc,px(12),py(16),RGB(255,255,255));
    }else{Ellipse(dc,px(8),py(4),px(16),py(12));RoundRect(dc,px(5),py(14),px(19),py(20),3,3);}
    SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);DeleteObject(blue);DeleteObject(white);
}
INT_PTR drawSettingsItem(HWND dialog,DRAWITEMSTRUCT* item) {
    if(item->CtlID!=UI_NAV && item->CtlID!=UI_HEADER)return FALSE;
    HDC dc=item->hDC;RECT area=item->rcItem;int saved=SaveDC(dc);
    SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(dialog,WM_GETFONT,0,0)));SetBkMode(dc,TRANSPARENT);
    if(item->CtlID==UI_HEADER){
        int height=std::max(1L,area.bottom-area.top);
        for(int row=0;row<height;row++){
            HBRUSH brush=CreateSolidBrush(RGB(132+row*77/height,158+row*63/height,188+row*45/height));
            RECT stripe{area.left,area.top+row,area.right,area.top+row+1};FillRect(dc,&stripe,brush);DeleteObject(brush);
        }
        auto state=settingsState(dialog);const wchar_t* titles[]={uiText(L"朗读设置",L"Speech"),uiText(L"违禁词",L"Word filter"),uiText(L"忽略名单",L"Ignored users")};
        const wchar_t* subtitles[]={uiText(L"常规选项与语音调节",L"General options and voice adjustment"),uiText(L"频道文字过滤规则",L"Channel text filtering rules"),uiText(L"不朗读指定用户的文字",L"Exclude selected users from speech")};
        int page=state?state->page:0;SetTextColor(dc,RGB(31,63,95));
        RECT title{area.left+8,area.top+3,area.right-36,area.bottom};DrawTextW(dc,titles[page],-1,&title,DT_SINGLELINE|DT_NOPREFIX);
        TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);title.top+=metrics.tmHeight;DrawTextW(dc,subtitles[page],-1,&title,DT_SINGLELINE|DT_NOPREFIX);
        RECT icon{area.right-34,area.top+2,area.right-4,area.bottom-2};drawSettingsIcon(dc,icon,page);
    }else if(item->itemID!=static_cast<UINT>(-1)){
        const bool selected=(item->itemState&ODS_SELECTED)!=0;
        HBRUSH brush=CreateSolidBrush(selected?RGB(204,231,252):RGB(255,255,255));FillRect(dc,&area,brush);DeleteObject(brush);
        if(selected){HBRUSH border=CreateSolidBrush(RGB(96,175,234));FrameRect(dc,&area,border);DeleteObject(border);}
        RECT icon{area.left+1,area.top+1,area.left+33,area.bottom-1};drawSettingsIcon(dc,icon,static_cast<int>(item->itemID));
        const wchar_t* titles[]={uiText(L"朗读设置",L"Speech"),uiText(L"违禁词",L"Word filter"),uiText(L"忽略名单",L"Ignored users")};
        RECT label{area.left+35,area.top,area.right-2,area.bottom};SetTextColor(dc,GetSysColor(COLOR_WINDOWTEXT));
        DrawTextW(dc,titles[std::min(item->itemID,2U)],-1,&label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        if(item->itemState&ODS_FOCUS){InflateRect(&area,-2,-2);DrawFocusRect(dc,&area);}
    }
    RestoreDC(dc,saved);return TRUE;
}
