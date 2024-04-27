/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "OSUtil.h"
#include "StringUtils.h"
#include <limits.h>
#include <libgen.h>
#include <filesystem>
#include <regex>

// Linux��ModulePath��/usr/lib��/usr/local/lib�ɂȂ��Ă��܂��̂�
// ���s�t�@�C��������f�B���N�g���ȉ��Ƀn�[�h�R�[�h
std::string GetModulePath() {
    char arg1[20];
    char exepath[PATH_MAX + 1] = {0};

    sprintf( arg1, "/proc/%d/exe", getpid() );
    readlink( arg1, exepath, PATH_MAX );
    char* basepath = dirname(dirname(exepath));
    return (std::filesystem::path(basepath) / "lib/libamatsukaze.so").c_str();
}

std::string GetModuleDirectory() {
    std::string modulePathStr = GetModulePath();
    char* modulePath = strdup(modulePathStr.c_str());
    char* path = dirname(modulePath);
    
    std::string ret(path);
    free(modulePath);
    return ret;
}

std::string SearchExe(const std::string& name) {
   return name;
}

//std::wstring GetDirectoryPath(const std::wstring& name) {
//	wchar_t buf[AMT_MAX_PATH] = { 0 };
//	std::copy(name.begin(), name.end(), buf);
//	PathRemoveFileSpecW(buf);
//	return buf;
//}

bool DirectoryExists(const std::string& dirName_in) {
   return std::filesystem::exists(dirName_in);
}

// dirpath�� �I�[\\�Ȃ�
// pattern�� "*.*" �Ƃ�
// �f�B���N�g�������܂܂Ȃ��t�@�C�������X�g���Ԃ�
std::vector<std::string> GetDirectoryFiles(const std::string& dirpath, const std::string& pattern) {
    std::vector<std::string> files;

    for (const auto & entry : std::filesystem::directory_iterator(dirpath)) {
        if (std::regex_match(entry.path().string(), std::regex(pattern))) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

// ���݂̃X���b�h�ɐݒ肳��Ă���R�A�����擾
int GetProcessorCount() {
    cpu_set_t mask;
    if (!sched_getaffinity(gettid(), sizeof(cpu_set_t), &mask)) {
        return CPU_COUNT(&mask);
    }

    return 8; // ���s������K���Ȓl�ɂ��Ă���
}
