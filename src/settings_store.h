#pragma once
const wchar_t* uiText(const wchar_t* zh,const wchar_t* en){return englishUI.load()?en:zh;}
struct SettingsDraft {
    Settings settings;
    bool english=false;
    std::vector<std::wstring> words;
    std::set<std::string> ignored;
};
SettingsDraft currentDraft() {
    std::lock_guard<std::mutex> lock(mu);
    return {cfg,englishUI.load(),blockedWords,ignoredUsers};
}
bool sameSpeechSettings(const Settings& a,const Settings& b) {
    return a.enabled==b.enabled && a.self==b.self && a.names==b.names && a.filter==b.filter && a.rate==b.rate && a.volume==b.volume;
}
struct TemporaryFile {
    std::wstring path;
    ~TemporaryFile(){if(!path.empty())DeleteFileW(path.c_str());}
    bool createNear(const std::wstring& destination) {
        wchar_t name[MAX_PATH]{};
        if(!GetTempFileNameW(std::filesystem::path(destination).parent_path().c_str(),L"tts",0,name))return false;
        path=name;return true;
    }
};
bool commitDraft(const SettingsDraft& draft,std::wstring& error) {
    // Called without mu held. Stage both files before changing live state.
    std::lock_guard<std::mutex> lock(mu);
    auto nextWords=draft.words;auto nextIgnored=draft.ignored;
    const bool changedWords=draft.words!=blockedWords;
    const bool cancelSpeech=!sameSpeechSettings(cfg,draft.settings) || changedWords || draft.ignored!=ignoredUsers;
    auto failed=[&](){error=uiText(L"保存失败，修改未应用。请检查配置目录权限或文件占用。",L"Save failed. Changes were not applied. Check folder permissions or file locks.");return false;};
    TemporaryFile configTemp,wordsTemp,backup;
    if(!configTemp.createNear(ini))return failed();
    DWORD attributes=GetFileAttributesW(ini.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES && !CopyFileW(ini.c_str(),configTemp.path.c_str(),FALSE))return failed();
    if(!SetFileAttributesW(configTemp.path.c_str(),FILE_ATTRIBUTE_NORMAL))return failed();
    auto put=[&](const wchar_t* key,int value){auto text=std::to_wstring(value);return WritePrivateProfileStringW(L"tts",key,text.c_str(),configTemp.path.c_str())!=0;};
    const auto& c=draft.settings;
    bool ok=put(L"enabled",c.enabled) && put(L"self",c.self) && put(L"names",c.names) && put(L"filter",c.filter) && put(L"rate",c.rate) && put(L"volume",c.volume);
    ok=ok && WritePrivateProfileStringW(L"ui",L"language",draft.english?L"en":L"zh-CN",configTemp.path.c_str());
    std::wstring section;size_t index=0;
    for(const auto& uid:draft.ignored){section+=L"user"+std::to_wstring(index++)+L"="+wide(uid.c_str());section.push_back(0);}
    section.push_back(0);section.push_back(0);
    ok=ok && WritePrivateProfileSectionW(L"ignored_users",section.c_str(),configTemp.path.c_str());
    if(!ok)return failed();
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,configTemp.path.c_str());
    bool previousWords=false;
    if(changedWords) {
        if(!wordsTemp.createNear(wordPath) || !backup.createNear(wordPath))return failed();
        DeleteFileW(backup.path.c_str());
        std::string bytes="\xEF\xBB\xBF";
        for(const auto& word:draft.words)bytes+=utf8(word.c_str())+"\r\n";
        HANDLE file=CreateFileW(wordsTemp.path.c_str(),GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return failed();
        DWORD written=0;ok=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) && written==bytes.size();
        if(ok)ok=FlushFileBuffers(file)!=0;CloseHandle(file);if(!ok)return failed();
        previousWords=GetFileAttributesW(wordPath.c_str())!=INVALID_FILE_ATTRIBUTES;
        ok=previousWords?ReplaceFileW(wordPath.c_str(),wordsTemp.path.c_str(),backup.path.c_str(),0,nullptr,nullptr)!=0
                        :MoveFileExW(wordsTemp.path.c_str(),wordPath.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
        if(!ok)return failed();
    }
    if(!MoveFileExW(configTemp.path.c_str(),ini.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        bool restored=true;
        if(changedWords)restored=previousWords?MoveFileExW(backup.path.c_str(),wordPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0:DeleteFileW(wordPath.c_str())!=0;
        if(!restored){error=uiText(L"设置保存失败，词库回退失败。旧词库备份：",L"Settings save and word-list rollback failed. Previous word-list backup: ")+backup.path;backup.path.clear();return false;}
        return failed();
    }
    // Invalidate profile caches after replacing the config file.
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,ini.c_str());
    cfg=draft.settings;blockedWords=std::move(nextWords);ignoredUsers=std::move(nextIgnored);englishUI.store(draft.english);
    if(cancelSpeech)clearLocked();
    return true;
}
bool saveEditorWords(const std::wstring& text,bool enabled,std::wstring& error) {
    auto draft=currentDraft();
    if(!parseEditorWords(text,draft.words,error))return false;
    draft.settings.filter=enabled;
    return commitDraft(draft,error);
}
