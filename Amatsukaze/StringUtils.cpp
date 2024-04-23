/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include <algorithm>
#include "StringUtils.h"

size_t strlenT(const wchar_t* string) {
    return wcslen(string);
}
size_t strlenT(const char* string) {
    return strlen(string);
}

int stricmpT(const char* string1, const char* string2) {
    return strcmp(string1, string2);
}

FILE* fsopenT(const char* FileName, const char* Mode, int ShFlag) {
    return fopen(FileName, Mode);
}

const char* string_internal::MakeArg(MakeArgContext& ctx, const char* value) { return value; }
const char* string_internal::MakeArg(MakeArgContext& ctx, const std::string& value) { return value.c_str(); }

string_internal::StringBuilderBase::StringBuilderBase() {}

MemoryChunk string_internal::StringBuilderBase::getMC() {
    return buffer.get();
}

void string_internal::StringBuilderBase::clear() {
    buffer.clear();
}

#ifdef _MSC_VER
/* static */ std::wstring to_tstring(const std::wstring& str, uint32_t codepage) {
    return str;
}

/* static */ std::wstring to_tstring(const std::string& str, uint32_t codepage) {
    return to_wstring(str, codepage);
}
#else
#include <iconv.h>

std::string to_string(const tstring& str, uint32_t codepage) {
    return std::string(str.c_str());
}
/* static */ tstring to_tstring(const tstring& str) {
    return str;
}
std::u16string to_u16string(const tstring& tstr, uint32_t codepage) {
    iconv_t ic;
    if ((ic = iconv_open("UTF-8", "UTF-16")) == (iconv_t) -1) {
        perror("iconv_open");
    }

    char* inbuf = strdup(tstr.c_str());
    size_t inbytes = tstr.length();
    size_t outbytes = inbytes * 4;
    std::unique_ptr<char[]> outbuf[outbytes];
    char* out = outbuf->get();
    if (iconv(ic, &inbuf, &inbytes, &out, &outbytes) == (size_t) -1) {
        perror("iconv");
    }
    iconv_close(ic);
    return std::u16string((char16_t*)out);
}
#endif

std::string StringBuilder::str() const {
    auto mc = buffer.get();
    return std::string(
        reinterpret_cast<const char*>(mc.data),
        reinterpret_cast<const char*>(mc.data + mc.length));
}

StringLiner::StringLiner() : searchIdx(0) {}

void StringLiner::AddBytes(MemoryChunk utf8) {
    buffer.add(utf8);
    while (SearchLineBreak());
}

void StringLiner::Flush() {
    if (buffer.size() > 0) {
        OnTextLine(buffer.ptr(), (int)buffer.size(), 0);
        buffer.clear();
    }
}

bool StringLiner::SearchLineBreak() {
    const uint8_t* ptr = buffer.ptr();
    for (int i = searchIdx; i < buffer.size(); ++i) {
        if (ptr[i] == '\n') {
            int len = i;
            int brlen = 1;
            if (len > 0 && ptr[len - 1] == '\r') {
                --len; ++brlen;
            }
            OnTextLine(ptr, len, brlen);
            buffer.trimHead(i + 1);
            searchIdx = 0;
            return true;
        }
    }
    searchIdx = (int)buffer.size();
    return false;
}

std::vector<char> utf8ToString(const uint8_t* ptr, int sz) {
#ifdef _WIN32
    int dstlen = MultiByteToWideChar(
        CP_UTF8, 0, (const char*)ptr, sz, nullptr, 0);
    std::vector<wchar_t> w(dstlen);
    MultiByteToWideChar(
        CP_UTF8, 0, (const char*)ptr, sz, w.data(), (int)w.size());
    dstlen = WideCharToMultiByte(
        CP_ACP, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::vector<char> ret(dstlen);
    WideCharToMultiByte(CP_ACP, 0,
        w.data(), (int)w.size(), ret.data(), (int)ret.size(), nullptr, nullptr);
    return ret;
#else
    std::vector<char> ret(sz);
    memcpy(ret.data(), ptr, sz);
    return ret;
#endif
}

bool starts_with(const std::wstring& str, const std::wstring& test) {
    return str.compare(0, test.size(), test) == 0;
}
bool starts_with(const std::string& str, const std::string& test) {
    return str.compare(0, test.size(), test) == 0;
}

bool ends_with(const tstring & value, const tstring & ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/* static */ tstring pathNormalize(tstring path) {
    if (path.size() != 0) {
        // バックスラッシュはスラッシュに変換
        std::replace(path.begin(), path.end(), _T('\\'), _T('/'));
        // 最後のスラッシュは取る
        if (path.back() == _T('/')) {
            path.pop_back();
        }
    }
    return path;
}

/* static */ tstring pathGetDirectory(const tstring& path) {
    size_t lastsplit = path.rfind(_T('/'));
    size_t namebegin = (lastsplit == tstring::npos)
        ? 0
        : lastsplit;
    return path.substr(0, namebegin);
}

/* static */ tstring pathRemoveExtension(const tstring& path) {
    const tchar* exts[] = { _T(".mp4"), _T(".mkv"), _T(".m2ts"), _T(".ts"), nullptr };
    const tchar* c_path = path.c_str();
    for (int i = 0; exts[i]; ++i) {
        size_t extlen = strlenT(exts[i]);
        if (path.size() > extlen) {
            if (stricmpT(c_path + (path.size() - extlen), exts[i]) == 0) {
                return path.substr(0, path.size() - extlen);
            }
        }
    }
    return path;
}

/* static */ tstring pathToOS(const tstring& path) {
#ifdef _WIN32
    tstring ret = path;
    std::replace(ret.begin(), ret.end(), _T('/'), _T('\\'));
    return ret;
#else
    return path;
#endif
}
