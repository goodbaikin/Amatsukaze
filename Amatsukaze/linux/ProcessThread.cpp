/**
* Sub process and thread utility
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "ProcessThread.h"
#include "rgy_thread_affinity.h"

#include <sys/wait.h>

std::vector<std::string> split(const std::string& src, const char* delim = " ") {
    std::vector<std::string> vec;
    std::string::size_type len = src.length();

    for (std::string::size_type i = 0, n; i < len; i = n + 1) {
        n = src.find_first_of(delim, i);
        if (n == std::string::npos) {
            n = len;
        }
        std::string trimmed = src.substr(i, n - i);
        if (trimmed.length() > 0) {
            vec.push_back(trimmed);
        }
    }

    std::vector<std::string> vec2;
    for (int i=0; i<vec.size(); i++) { 
        if (vec[i][0]=='\"' && vec[i].back() != '"') {
            int lastIndex;
            for (lastIndex=i; lastIndex<vec.size(); lastIndex++) {
                if (vec[lastIndex].back() == '"') break;
            }

            std::string str = vec[i].substr(1);
            for (int j=i+1; j<=lastIndex-1; j++) {
                str += " ";
                str += vec[j].c_str();
            }
            str += " ";
            str += vec[lastIndex].substr(0, vec[lastIndex].size()-1);

            vec2.push_back(str);
            i=lastIndex;
        } else if (vec[i][0] == '\"' && vec[i].back() == '\"'){
            vec2.push_back(vec[i].substr(1, vec[i].size()-2));
        } else {
            vec2.push_back(vec[i]);
        }
    }

    return vec2;
}

SubProcess::SubProcess(const tstring& args, const bool disablePowerThrottoling) :
    pi_(PROCESS_INFORMATION()),
    stdErrPipe_(),
    stdOutPipe_(),
    stdInPipe_(),
    exitCode_(0) {

    std::vector<std::string> argsVec;
    const char* cmd;
    const char** argv;
    
    pid_t pid = fork();
    switch (pid) {
    case -1:
        THROW(RuntimeException, "プロセスのフォークに失敗");
    case 0:
        if (
            dup2(stdErrPipe_.fd_[WRITE_HANDLE], 2) < 0
            || dup2(stdOutPipe_.fd_[WRITE_HANDLE], 1) < 0
            || dup2(stdInPipe_.fd_[READ_HANDLE], 0) < 0
        ) {
            THROWF(RuntimeException, "リダイレクトに失敗: %s", strerror(errno));
        }
        stdErrPipe_.closeRead();
        stdOutPipe_.closeRead();
        stdInPipe_.closeWrite();

        argsVec = split(args, " ");
        cmd = argsVec[0].c_str();

        argv = new const char* [argsVec.size() + 1];
        for (int j = 0; j < argsVec.size() + 1; ++j) {
            argv[j] = argsVec[j].c_str();
        }
        argv[argsVec.size()] = NULL;

        execvp(cmd, (char**)argv);
        THROWF(RuntimeException, "プロセス起動に失敗: %s", strerror(errno));
        break;
    default:
        pi_.hProcess = (int)pid;
        stdErrPipe_.closeWrite();
        stdOutPipe_.closeWrite();
        stdInPipe_.closeRead();
        break;
    } 
}
SubProcess::~SubProcess() {
    join();
}
void SubProcess::write(MemoryChunk mc) {
    if (mc.length > 0xFFFFFFFF) {
        THROW(RuntimeException, "buffer too large");
    }
    DWORD bytesWritten = 0;
    if (WriteFile(stdInPipe_.fd_[WRITE_HANDLE], mc.data, (DWORD)mc.length, &bytesWritten, NULL) == 0) {
        THROW(RuntimeException, "failed to write to stdin pipe");
    }
    if (bytesWritten != mc.length) {
        THROW(RuntimeException, "failed to write to stdin pipe (bytes written mismatch)");
    }
}
size_t SubProcess::readErr(MemoryChunk mc) {
    return readGeneric(mc, stdErrPipe_.fd_[READ_HANDLE]);
}
size_t SubProcess::readOut(MemoryChunk mc) {
    return readGeneric(mc, stdOutPipe_.fd_[READ_HANDLE]);
}
void SubProcess::finishWrite() {
    stdInPipe_.closeWrite();
}

int SubProcess::join() {
    if (pi_.hProcess > 0) {
        // 子プロセスの終了を待つ
        siginfo_t info;
        waitid(P_PID, pi_.hProcess, &info, WEXITED);
        // 終了コード取得
        exitCode_ = info.si_status;

        pi_.hProcess = -1;
    }
    return exitCode_;
}

SubProcess::Pipe::Pipe() {
    if (pipe(fd_) < 0) {
        THROW(RuntimeException, "failed to create pipe");
    }
}
SubProcess::Pipe::~Pipe() {
    closeRead();
    closeWrite();
}
void SubProcess::Pipe::closeRead() {
    if (fd_[READ_HANDLE] != -1) {
        close(fd_[READ_HANDLE]);
        fd_[READ_HANDLE] = -1;
    }
}
void SubProcess::Pipe::closeWrite() {
    if (fd_[WRITE_HANDLE] != -1) {
        close(fd_[WRITE_HANDLE]);
        fd_[WRITE_HANDLE] = -1;
    }
}

size_t SubProcess::readGeneric(MemoryChunk mc, HANDLE readHandle) {
    if (mc.length > 0xFFFFFFFF) {
        THROW(RuntimeException, "buffer too large");
    }
    DWORD bytesRead = 0;
    while (true) {
        if (!ReadFile(readHandle, mc.data, (DWORD)mc.length, &bytesRead, NULL)) {
            THROW(RuntimeException, "failed to read from pipe");
        }
        // パイプはWriteFileにゼロを渡すとReadFileがゼロで帰ってくるのでチェック
        if (bytesRead >= 0) {
            break;
        }
    }
    return bytesRead;
}

EventBaseSubProcess::EventBaseSubProcess(const tstring& args, const bool disablePowerThrottoling)
    : SubProcess(args, disablePowerThrottoling)
    , drainOut(this, false)
    , drainErr(this, true) {
    drainOut.start();
    drainErr.start();
}
EventBaseSubProcess::~EventBaseSubProcess() {
    if (drainOut.isRunning()) {
        THROW(InvalidOperationException, "call join before destroy object ...");
    }
}
int EventBaseSubProcess::join() {
    /*
    * 終了処理の流れ
    * finishWrite()
    * -> 子プロセスが終了検知
    * -> 子プロセスが終了
    * -> stdout,stderrの書き込みハンドルが自動的に閉じる
    * -> SubProcess.readGeneric()がEOFExceptionを返す
    * -> DrainThreadが例外をキャッチして終了
    * -> DrainThreadのjoin()が完了
    * -> EventBaseSubProcessのjoin()が完了
    * -> プロセスは終了しているのでSubProcessのデストラクタはすぐに完了
    */
    try {
        finishWrite();
    } catch (RuntimeException&) {
        // 子プロセスがエラー終了していると書き込みに失敗するが無視する
    }
    drainOut.join();
    drainErr.join();
    return SubProcess::join();
}
bool EventBaseSubProcess::isRunning() { return drainOut.isRunning(); }

EventBaseSubProcess::DrainThread::DrainThread(EventBaseSubProcess* this_, bool isErr)
    : this_(this_)
    , isErr_(isErr) {}
void EventBaseSubProcess::DrainThread::run() {
    this_->drain_thread(isErr_);
}

void EventBaseSubProcess::drain_thread(bool isErr) {
    std::vector<uint8_t> buffer(4 * 1024);
    MemoryChunk mc(buffer.data(), buffer.size());
    while (true) {
        size_t bytesRead = isErr ? readErr(mc) : readOut(mc);
        if (bytesRead == 0) { // 終了
            break;
        }
        onOut(isErr, MemoryChunk(mc.data, bytesRead));
    }
}

StdRedirectedSubProcess::StdRedirectedSubProcess(const tstring& args, const int bufferLines, const bool isUtf8, const bool disablePowerThrottoling)
    : EventBaseSubProcess(args, disablePowerThrottoling)
    , bufferLines(bufferLines)
    , isUtf8(isUtf8)
    , outLiner(this, false)
    , errLiner(this, true) {}

StdRedirectedSubProcess::~StdRedirectedSubProcess() {
    if (isUtf8) {
        outLiner.Flush();
        errLiner.Flush();
    }
}

const std::deque<std::vector<char>>& StdRedirectedSubProcess::getLastLines() {
    outLiner.Flush();
    errLiner.Flush();
    return lastLines;
}

StdRedirectedSubProcess::SpStringLiner::SpStringLiner(StdRedirectedSubProcess* pThis, bool isErr)
    : pThis(pThis), isErr(isErr) {}

void StdRedirectedSubProcess::SpStringLiner::OnTextLine(const uint8_t* ptr, int len, int brlen) {
    pThis->onTextLine(isErr, ptr, len, brlen);
}


void StdRedirectedSubProcess::onTextLine(bool isErr, const uint8_t* ptr, int len, int brlen) {

    std::vector<char> line;
    if (isUtf8) {
        line = utf8ToString(ptr, len);
        // 変換する場合はここで出力
        fwrite(line.data(), line.size(), 1, SUBPROC_OUT);
        fprintf(SUBPROC_OUT, "\n");
        fflush(SUBPROC_OUT);
    } else {
        line = std::vector<char>(ptr, ptr + len);
    }

    if (bufferLines > 0) {
        std::lock_guard<std::mutex> lock(mtx);
        if (lastLines.size() > bufferLines) {
            lastLines.pop_front();
        }
        lastLines.push_back(line);
    }
}

void StdRedirectedSubProcess::onOut(bool isErr, MemoryChunk mc) {
    if (bufferLines > 0 || isUtf8) { // 必要がある場合のみ
        (isErr ? errLiner : outLiner).AddBytes(mc);
    }
    if (!isUtf8) {
        // 変換しない場合はここですぐに出力
        fwrite(mc.data, mc.length, 1, SUBPROC_OUT);
        fflush(SUBPROC_OUT);
    }
}


CPUInfo::CPUInfo() {}

const GROUP_AFFINITY* CPUInfo::GetData(PROCESSOR_INFO_TAG tag, int* count) {
    *count = (int)data[tag].size();
    return data[tag].data();
}

bool SetCPUAffinity(int group, uint64_t mask) {
    if (mask == 0) {
        return true;
    }
    GROUP_AFFINITY gf = GROUP_AFFINITY();
    gf.Group = group;
    gf.Mask = mask;

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);

    int max_cpus = sizeof(mask) * 8;
    for (int cpu_id = 0; mask != 0 && cpu_id < max_cpus; ++cpu_id) {
        if (mask & 1) {
            CPU_SET(cpu_id, &cpu_set);
        }
        mask = mask >> 1;
    }

    int ret = sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpu_set);
    return ret == 0;
}
