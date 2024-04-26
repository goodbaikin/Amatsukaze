#pragma once

/**
* Amtasukaze OS Utility
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#pragma once

#include "common.h"
#include <vector>
#include <string>

#include "CoreUtils.hpp"

extern FILE* g_DllHandle;

std::string GetModulePath();

std::string GetModuleDirectory();

std::string SearchExe(const std::string& name);

//std::wstring GetDirectoryPath(const std::wstring& name) {
//	wchar_t buf[AMT_MAX_PATH] = { 0 };
//	std::copy(name.begin(), name.end(), buf);
//	PathRemoveFileSpecW(buf);
//	return buf;
//}

bool DirectoryExists(const std::string& dirName_in);

// dirpath�� �I�[\\�Ȃ�
// pattern�� "*.*" �Ƃ�
// �f�B���N�g�������܂܂Ȃ��t�@�C�������X�g���Ԃ�
std::vector<std::string> GetDirectoryFiles(const std::string& dirpath, const std::string& pattern);

// ���݂̃X���b�h�ɐݒ肳��Ă���R�A�����擾
int GetProcessorCount();

