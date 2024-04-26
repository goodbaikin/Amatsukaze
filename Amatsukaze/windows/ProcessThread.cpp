/**
* Sub process and thread utility
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "ProcessThread.h"
#include "rgy_thread_affinity.h"

SubProcess::SubProcess(const tstring& args, const bool disablePowerThrottoling) :
    pi_(PROCESS_INFORMATION()),
    stdErrPipe_(),
    stdOutPipe_(),
    stdInPipe_(),
    exitCode_(0),
    thSetPowerThrottling() {
    STARTUPINFOW si = STARTUPINFOW();

    si.cb = sizeof(si);
    si.hStdError = stdErrPipe_.writeHandle;
    si.hStdOutput = stdOutPipe_.writeHandle;
    si.hStdInput = stdInPipe_.readHandle;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // �K�v�Ȃ��n���h���͌p���𖳌���
    if (SetHandleInformation(stdErrPipe_.readHandle, HANDLE_FLAG_INHERIT, 0) == 0 ||
        SetHandleInformation(stdOutPipe_.readHandle, HANDLE_FLAG_INHERIT, 0) == 0 ||
        SetHandleInformation(stdInPipe_.writeHandle, HANDLE_FLAG_INHERIT, 0) == 0) {
        THROW(RuntimeException, "failed to set handle information");
    }

    // Priority Class�͖������Ă��Ȃ��̂ŁA�f�t�H���g����ƂȂ�
    // �f�t�H���g����́A�e�v���Z�X�i���̃v���Z�X�j��NORMAL_PRIORITY_CLASS�ȏ��
    // �����Ă���ꍇ�́ANORMAL_PRIORITY_CLASS
    // IDLE_PRIORITY_CLASS�����BELOW_NORMAL_PRIORITY_CLASS��
    // �����Ă���ꍇ�́A����Priority Class���p�������
    // Priority Class�͎q�v���Z�X�Ɍp�������Ώۂł͂Ȃ����A
    // NORMAL_PRIORITY_CLASS�ȉ��ł͎����p������邱�Ƃɒ���

    if (CreateProcessW(NULL, const_cast<tchar*>(args.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi_) == 0) {
        THROW(RuntimeException, "�v���Z�X�N���Ɏ��s�Bexe�̃p�X���m�F���Ă��������B");
    }

    // �q�v���Z�X�p�̃n���h���͕K�v�Ȃ��̂ŕ���
    stdErrPipe_.closeWrite();
    stdOutPipe_.closeWrite();
    stdInPipe_.closeRead();

    if (disablePowerThrottoling) {
        runSetPowerThrottling();
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
    if (WriteFile(stdInPipe_.writeHandle, mc.data, (DWORD)mc.length, &bytesWritten, NULL) == 0) {
        THROW(RuntimeException, "failed to write to stdin pipe");
    }
    if (bytesWritten != mc.length) {
        THROW(RuntimeException, "failed to write to stdin pipe (bytes written mismatch)");
    }
}
size_t SubProcess::readErr(MemoryChunk mc) {
    return readGeneric(mc, stdErrPipe_.readHandle);
}
size_t SubProcess::readOut(MemoryChunk mc) {
    return readGeneric(mc, stdOutPipe_.readHandle);
}
void SubProcess::finishWrite() {
    stdInPipe_.closeWrite();
}

void SubProcess::runSetPowerThrottling() {
    if (pi_.hProcess) {
        thSetPowerThrottling = std::make_unique<RGYThreadSetPowerThrottoling>(pi_.dwProcessId);
        thSetPowerThrottling->run(RGYThreadPowerThrottlingMode::Disabled);
    }
}
int SubProcess::join() {
    if (pi_.hProcess != NULL) {
        if (thSetPowerThrottling) {
            thSetPowerThrottling->abortThread();
        }
        // �q�v���Z�X�̏I����҂�
        WaitForSingleObject(pi_.hProcess, INFINITE);
        // �I���R�[�h�擾
        GetExitCodeProcess(pi_.hProcess, &exitCode_);

        CloseHandle(pi_.hProcess);
        CloseHandle(pi_.hThread);
        pi_.hProcess = NULL;
    }
    return exitCode_;
}

SubProcess::Pipe::Pipe() {
    // �p����L���ɂ��č쐬
    SECURITY_ATTRIBUTES sa = SECURITY_ATTRIBUTES();
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (CreatePipe(&readHandle, &writeHandle, &sa, 0) == 0) {
        THROW(RuntimeException, "failed to create pipe");
    }
}
SubProcess::Pipe::~Pipe() {
    closeRead();
    closeWrite();
}
void SubProcess::Pipe::closeRead() {
    if (readHandle != NULL) {
        CloseHandle(readHandle);
        readHandle = NULL;
    }
}
void SubProcess::Pipe::closeWrite() {
    if (writeHandle != NULL) {
        CloseHandle(writeHandle);
        writeHandle = NULL;
    }
}

size_t SubProcess::readGeneric(MemoryChunk mc, HANDLE readHandle) {
    if (mc.length > 0xFFFFFFFF) {
        THROW(RuntimeException, "buffer too large");
    }
    DWORD bytesRead = 0;
    while (true) {
        if (ReadFile(readHandle, mc.data, (DWORD)mc.length, &bytesRead, NULL) == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                return 0;
            }
            THROW(RuntimeException, "failed to read from pipe");
        }
        // �p�C�v��WriteFile�Ƀ[����n����ReadFile���[���ŋA���Ă���̂Ń`�F�b�N
        if (bytesRead != 0) {
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
    * �I�������̗���
    * finishWrite()
    * -> �q�v���Z�X���I�����m
    * -> �q�v���Z�X���I��
    * -> stdout,stderr�̏������݃n���h���������I�ɕ���
    * -> SubProcess.readGeneric()��EOFException��Ԃ�
    * -> DrainThread����O���L���b�`���ďI��
    * -> DrainThread��join()������
    * -> EventBaseSubProcess��join()������
    * -> �v���Z�X�͏I�����Ă���̂�SubProcess�̃f�X�g���N�^�͂����Ɋ���
    */
    try {
        finishWrite();
    } catch (RuntimeException&) {
        // �q�v���Z�X���G���[�I�����Ă���Ə������݂Ɏ��s���邪��������
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
        if (bytesRead == 0) { // �I��
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
        // �ϊ�����ꍇ�͂����ŏo��
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
    if (bufferLines > 0 || isUtf8) { // �K�v������ꍇ�̂�
        (isErr ? errLiner : outLiner).AddBytes(mc);
    }
    if (!isUtf8) {
        // �ϊ����Ȃ��ꍇ�͂����ł����ɏo��
        fwrite(mc.data, mc.length, 1, SUBPROC_OUT);
        fflush(SUBPROC_OUT);
    }
}


CPUInfo::CPUInfo() {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        THROW(RuntimeException, "GetLogicalProcessorInformationEx��ERROR_INSUFFICIENT_BUFFER��Ԃ��Ȃ�����");
    }
    std::unique_ptr<uint8_t[]> buf = std::unique_ptr<uint8_t[]>(new uint8_t[length]);
    uint8_t* ptr = buf.get();
    uint8_t* end = ptr + length;
    if (GetLogicalProcessorInformationEx(
        RelationAll, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)ptr, &length) == 0) {
        THROW(RuntimeException, "GetLogicalProcessorInformationEx�Ɏ��s");
    }
    while (ptr < end) {
        auto info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)ptr;
        switch (info->Relationship) {
        case RelationCache:
            // �K�v�Ȃ̂�L2,L3�̂�
            if (info->Cache.Level == 2 || info->Cache.Level == 3) {
                data[(info->Cache.Level == 2) ? PROC_TAG_L2 : PROC_TAG_L3].push_back(info->Cache.GroupMask);
            }
            break;
        case RelationGroup:
            for (int i = 0; i < info->Group.ActiveGroupCount; ++i) {
                GROUP_AFFINITY af = GROUP_AFFINITY();
                af.Group = i;
                af.Mask = info->Group.GroupInfo[i].ActiveProcessorMask;
                data[PROC_TAG_GROUP].push_back(af);
            }
            break;
        case RelationNumaNode:
            data[PROC_TAG_NUMA].push_back(info->NumaNode.GroupMask);
            break;
        case RelationProcessorCore:
            if (info->Processor.GroupCount != 1) {
                THROW(RuntimeException, "GetLogicalProcessorInformationEx�ŗ\�����Ȃ��f�[�^");
            }
            data[PROC_TAG_CORE].push_back(info->Processor.GroupMask[0]);
            break;
        default:
            break;
        }
        ptr += info->Size;
    }
}
const GROUP_AFFINITY* CPUInfo::GetData(PROCESSOR_INFO_TAG tag, int* count) {
    *count = (int)data[tag].size();
    return data[tag].data();
}

extern "C" __declspec(dllexport) void* CPUInfo_Create(AMTContext * ctx) {
    try {
        return new CPUInfo();
    } catch (const Exception& exception) {
        ctx->setError(exception);
    }
    return nullptr;
}
extern "C" __declspec(dllexport) void CPUInfo_Delete(CPUInfo * ptr) { delete ptr; }
extern "C" __declspec(dllexport) const GROUP_AFFINITY * CPUInfo_GetData(CPUInfo * ptr, int tag, int* count) {
    return ptr->GetData((PROCESSOR_INFO_TAG)tag, count);
}

bool SetCPUAffinity(int group, uint64_t mask) {
    if (mask == 0) {
        return true;
    }
    GROUP_AFFINITY gf = GROUP_AFFINITY();
    gf.Group = group;
    gf.Mask = (KAFFINITY)mask;
    bool result = (SetThreadGroupAffinity(GetCurrentThread(), &gf, nullptr) != FALSE);
    // �v���Z�X�������̃O���[�v�ɂ܂������Ă�Ɓ��̓G���[�ɂȂ�炵��
    SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)mask);
    return result;
}
