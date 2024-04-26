#pragma once

/*
* ARIB Caption
* ���̃\�[�X�R�[�h��TVCaptionMod2��Caption.dll�̃R�[�h�𗬗p���Ă��܂��B
* ���C�Z���X�̓I���W�i���ł̈ȉ����C�Z���X�ɏ]���܂��B
------���p�J�n------
��EpgDataCap_Bon�ATSEpgView_Sample�ANetworkRemocon�ACaption�ATSEpgViewServer��
�\�[�X�̎�舵���ɂ���
�@����GPL�Ƃ��ɂ͂��Ȃ��̂Ŏ��R�ɉ��ς��Ă�����č\��Ȃ��ł��B
�@���ς��Č��J����ꍇ�͉��ϕ����̃\�[�X���炢�͈ꏏ�Ɍ��J���Ă��������B
�@�i�����ł͂Ȃ��̂ŕʂɌ��J���Ȃ��Ă������ł��j
�@EpgDataCap.dll�̎g�����̎Q�l�ɂ��Ă��炤�Ƃ��������B

��EpgDataCap.dll�ACaption.dll�̎�舵���ɂ���
    �t���[�\�t�g�ɑg�ݍ��ޏꍇ�͓��ɐ����݂͐��܂���B�������Adll�̓I���W�i���̂܂�
    �g�ݍ���ł��������B
    ����dll���g�p�������Ƃɂ���Ĕ����������ɂ��ĕۏ؂͈�؍s���܂���B
�@���p�A�V�F�A�E�F�A�Ȃǂɑg�ݍ��ނ͕̂s�ł��B
------���p�I��------
*/
#pragma once

#include "common.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <openssl/md5.h>

#include "windows.h"
#include "CaptionDef.h"
#include "StreamUtils.h"
#include "../TranscodeSetting.h"

inline bool operator==(const CLUT_DAT_DLL &a, const CLUT_DAT_DLL &b) {
    return a.ucR == b.ucR && a.ucG == b.ucG && a.ucB == b.ucB && a.ucAlpha == b.ucAlpha;
}

struct CaptionFormat {

    enum STYLE {
        UNDERLINE = 1,
        SHADOW = 2,
        BOLD = 4,
        ITALIC = 8,

        BOTTOM = 0x10,
        RIGHT = 0x20,
        TOP = 0x40,
        LEFT = 0x80,
    };

    static int GetStyle(const CAPTION_CHAR_DATA_DLL &style);

    int pos;
    float charW, charH;
    float width, height;
    CLUT_DAT_DLL textColor;
    CLUT_DAT_DLL backColor;
    int style;
    int sizeMode;

    bool IsUnderline() const;
    bool IsShadow() const;
    bool IsBold() const;
    bool IsItalic() const;
    bool IsHighLightBottom() const;
    bool IsHighLightRight() const;
    bool IsHighLightTop() const;
    bool IsHighLightLeft() const;
    int GetFlushMode() const;
};

struct CaptionLine {
    std::u16string text;
    int planeW;
    int planeH;
    float posX;
    float posY;
    std::vector<CaptionFormat> formats;

    void Write(const File& file) const;

    static std::unique_ptr<CaptionLine> Read(const File& file);
};

struct CaptionItem {
    int64_t PTS;
    int langIndex;
    int waitTime;
    // null���ƃN���A
    std::unique_ptr<CaptionLine> line;

    void Write(const File& file) const;

    static CaptionItem Read(const File& file);
};

// ���p�u���\�������X�g
// �L����JISX0213 1��1��̂����O���t���p�ӂ���Ă���\�����\���������Ȃ��̂���
extern const char16_t* HALF_F_LIST;
extern const char16_t* HALF_T_LIST;
extern const char16_t* HALF_R_LIST;

bool CalcMD5FromDRCSPattern(std::vector<char>& hash, const DRCS_PATTERN_DLL *pPattern);

void SaveDRCSImage(const tstring& filename, const DRCS_PATTERN_DLL* pData);

int StrlenWoLoSurrogate(LPCWSTR str);

struct DRCSOutInfo {
    tstring filename;
    double elapsed;
};

class CaptionDLLParser : public AMTObject {
public:
    CaptionDLLParser(AMTContext& ctx);

    // �ŏ��̂P������������
    CaptionItem ProcessCaption(int64_t PTS, int langIndex,
        const CAPTION_DATA_DLL* capList, int capCount, DRCS_PATTERN_DLL* pDrcsList, int drcsCount);

    virtual DRCSOutInfo getDRCSOutPath(int64_t PTS, const std::string& md5) = 0;

private:

    // �g�k��̕����T�C�Y�𓾂�
    static void GetCharSize(float *pCharW, float *pCharH, float *pDirW, float *pDirH, const CAPTION_CHAR_DATA_DLL &charData);

    void AddText(CaptionLine& line, const std::u16string& text,
        float charW, float charH, float width, float height, const CAPTION_CHAR_DATA_DLL &style);

    // �����{����1�s������������
    std::unique_ptr<CaptionLine> ShowCaptionData(int64_t PTS,
        const CAPTION_DATA_DLL &caption, const DRCS_PATTERN_DLL *pDrcsList, uint32_t drcsCount);
};



