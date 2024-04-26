/**
* Amtasukaze Compile Target
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#define _USE_MATH_DEFINES
#include "AmatsukazeCLI.hpp"

// Avisynth�t�B���^�f�o�b�O�p
// #include "TextOut.cpp"

bool g_av_initialized = false;

#ifdef _WIN32
#include "LogoGUISupport.hpp"
HMODULE g_DllHandle;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) g_DllHandle = hModule;
    return TRUE;
}

extern "C" __declspec(dllexport) void InitAmatsukazeDLL() {
    // FFMPEG���C�u����������
    av_register_all();
#if ENABLE_FFMPEG_FILTER
    avfilter_register_all();
#endif
}

static void init_console() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
}
#endif

// CM��͗p�i�{�f�o�b�O�p�j�C���^�[�t�F�[�X
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors) {
    // ���ڃ����N���Ă���̂�vectors���i�[����K�v�͂Ȃ�
    AVS_linkage = vectors;
    if (g_av_initialized == false) {
        // FFMPEG���C�u����������
        av_register_all();
#if ENABLE_FFMPEG_FILTER
        avfilter_register_all();
#endif
        g_av_initialized = true;
    }

    env->AddFunction("AMTSource", "s[filter]s[outqp]b[threads]i", av::CreateAMTSource, 0);

    env->AddFunction("AMTAnalyzeLogo", "cs[maskratio]i", logo::AMTAnalyzeLogo::Create, 0);
    env->AddFunction("AMTEraseLogo", "ccs[logof]s[mode]i[maxfade]i", logo::AMTEraseLogo::Create, 0);

    env->AddFunction("AMTDecimate", "c[duration]s", AMTDecimate::Create, 0);

    env->AddFunction("AMTExec", "cs", AMTExec, 0);
    env->AddFunction("AMTOrderedParallel", "c+", AMTOrderedParallel::Create, 0);

    return "Amatsukaze plugin";
}
