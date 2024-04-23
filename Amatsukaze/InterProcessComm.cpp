/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include <deque>
#include "InterProcessComm.h"
#include "PerformanceUtil.h"

/* static */ std::string toJsonString(const std::string& str) {
    if (str.size() == 0) {
        return std::string();
    }
    std::vector<char> utf8(str.begin(), str.end());
    std::vector<char> ret;
    for (char c : utf8) {
        switch (c) {
        case '\"':
            ret.push_back('\\');
            ret.push_back('\"');
            break;
        case '\\':
            ret.push_back('\\');
            ret.push_back('\\');
            break;
        case '/':
            ret.push_back('\\');
            ret.push_back('/');
            break;
        case '\b':
            ret.push_back('\\');
            ret.push_back('b');
            break;
        case '\f':
            ret.push_back('\\');
            ret.push_back('f');
            break;
        case '\n':
            ret.push_back('\\');
            ret.push_back('n');
            break;
        case '\r':
            ret.push_back('\\');
            ret.push_back('r');
            break;
        case '\t':
            ret.push_back('\\');
            ret.push_back('t');
            break;
        default:
            ret.push_back(c);
        }
    }
    return std::string(ret.begin(), ret.end());
}

bool ResourceAllocation::IsFailed() const {
    return gpuIndex == -1;
}

void ResourceManger::write(MemoryChunk mc) const {
    DWORD bytesWritten = 0;
    if (WriteFile(outPipe, mc.data, (DWORD)mc.length, &bytesWritten, NULL) == 0) {
        THROW(RuntimeException, "failed to write to stdin pipe");
    }
    if (bytesWritten != mc.length) {
        THROW(RuntimeException, "failed to write to stdin pipe (bytes written mismatch)");
    }
}

void ResourceManger::read(MemoryChunk mc) const {
    int offset = 0;
    while (offset < mc.length) {
        DWORD bytesRead = 0;
        if (ReadFile(inPipe, mc.data + offset, (int)mc.length - offset, &bytesRead, NULL) == 0) {
            THROW(RuntimeException, "failed to read from pipe");
        }
        offset += bytesRead;
    }
}

void ResourceManger::writeCommand(int cmd) const {
    write(MemoryChunk((uint8_t*)&cmd, 4));
}

/*
   void write(int cmd, const std::string& json) {
       write(MemoryChunk((uint8_t*)&cmd, 4));
       int sz = (int)json.size();
       write(MemoryChunk((uint8_t*)&sz, 4));
       write(MemoryChunk((uint8_t*)json.data(), sz));
   }
*/

/* static */ ResourceAllocation ResourceManger::DefaultAllocation() {
    ResourceAllocation res = { 0, -1, 0 };
    return res;
}

ResourceAllocation ResourceManger::readCommand(int expected) const {
    DWORD bytesRead = 0;
    int32_t cmd;
    ResourceAllocation res;
    read(MemoryChunk((uint8_t*)&cmd, sizeof(cmd)));
    if (cmd != expected) {
        THROW(RuntimeException, "invalid return command");
    }
    read(MemoryChunk((uint8_t*)&res, sizeof(res)));
    return res;
}
ResourceManger::ResourceManger(AMTContext& ctx, HANDLE inPipe, HANDLE outPipe)
    : AMTObject(ctx)
    , inPipe(inPipe)
    , outPipe(outPipe) {}

bool ResourceManger::isInvalidHandle(HANDLE handle) const {
#ifdef _WIN32
    return handle == INVALID_HANDLE_VALUE;
#else
    return handle < 0;
#endif
}

ResourceAllocation ResourceManger::request(PipeCommand phase) const {
    if (isInvalidHandle(inPipe)) {
        return DefaultAllocation();
    }
    writeCommand(phase | HOST_CMD_NoWait);
    return readCommand(phase);
}

// リソース確保できるまで待つ
ResourceAllocation ResourceManger::wait(PipeCommand phase) const {
    if (isInvalidHandle(inPipe)) {
        return DefaultAllocation();
    }
    ResourceAllocation ret = request(phase);
    if (ret.IsFailed()) {
        writeCommand(phase);
        ctx.progress("リソ拏ス待ち ...");
        Stopwatch sw; sw.start();
        ret = readCommand(phase);
        ctx.infoF("リソ拏ス待ち %.2f秒", sw.getAndReset());
    }
    return ret;
}
