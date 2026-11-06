#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace word_filter {
inline bool matches(const std::wstring& text,const std::vector<std::wstring>& words) {
    if(text.empty()) return false;
    for(const auto& word:words) {
        if(!word.empty() && FindStringOrdinal(FIND_FROMSTART,text.data(),static_cast<int>(text.size()),
            word.data(),static_cast<int>(word.size()),TRUE)>=0) return true;
    }
    return false;
}
}
