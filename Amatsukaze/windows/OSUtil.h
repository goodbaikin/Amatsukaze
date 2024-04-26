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
#include <Windows.h>
#include "Shlwapi.h"

#include <vector>
#include <string>

#include "CoreUtils.hpp"

extern HMODULE g_DllHandle;

std::wstring GetModulePath();

std::wstring GetModuleDirectory();

std::wstring SearchExe(const std::wstring& name);

//std::wstring GetDirectoryPath(const std::wstring& name) {
//	wchar_t buf[AMT_MAX_PATH] = { 0 };
//	std::copy(name.begin(), name.end(), buf);
//	PathRemoveFileSpecW(buf);
//	return buf;
//}

bool DirectoryExists(const std::wstring& dirName_in);

// dirpath�� �I�[\\�Ȃ�
// pattern�� "*.*" �Ƃ�
// �f�B���N�g�������܂܂Ȃ��t�@�C�������X�g���Ԃ�
std::vector<std::wstring> GetDirectoryFiles(const std::wstring& dirpath, const std::wstring& pattern);

// ���݂̃X���b�h�ɐݒ肳��Ă���R�A�����擾
int GetProcessorCount();

