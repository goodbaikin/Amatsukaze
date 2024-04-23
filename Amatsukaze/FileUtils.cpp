/**
* Amatsukaze core utility
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "FileUtils.h"
#include <filesystem>

std::string GetFullPathName(const char* lpFileName) {
    return std::filesystem::absolute(lpFileName).string();
}

int rmdirT(const char* dirname) {
    try {
        std::filesystem::remove(dirname);
    } catch (...) {
        return 1;
    }
    return 0;
}

int mkdirT(const char* dirname) {
    try {
        std::filesystem::create_directories(dirname);
    } catch(...) {
        return 1;
    }
    return 0;
}

int removeT(const char* dirname) {
    try {
        std::filesystem::remove(dirname);
    } catch (...) {
        return 1;
    }
    return 0;
}

void PrintFileAll(const tstring& path) {
    File file(path, _T("rb"));
    int sz = (int)file.size();
    if (sz == 0) return;
    auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[sz]);
    auto rsz = file.read(MemoryChunk(buf.get(), sz));
    fwrite(buf.get(), 1, strnlen((char*)buf.get(), rsz), stderr);
    if (buf[rsz - 1] != '\n') {
        // 改行で終わっていないときは改行する
        fprintf(stderr, "\n");
    }
}

