/*
* ARIB String
* このソースコードは大部分がTvTestのコードです。
* https://github.com/DBCTRADO/TVTest
* GPLで利用しています。
*/
#pragma once

#include <memory>

#ifndef _WIN32
#include <iconv.h>
#endif

namespace aribstring {

// 他の部分は全部設定なし（Ansi）だが、とりあえずこのファイルだけUnicodeを使うようにする

#ifndef _UNICODE
#define __HACK_UNICODE__
#define _UNICODE
#endif

typedef const wchar_t* LPCTSTR;
typedef wchar_t TCHAR;

static const bool abCharSizeTable[] =
{
    false,	// CODE_UNKNOWN					不明なグラフィックセット(非対応)
    true,	// CODE_KANJI					Kanji
    false,	// CODE_ALPHANUMERIC			Alphanumeric
    false,	// CODE_HIRAGANA				Hiragana
    false,	// CODE_KATAKANA				Katakana
    false,	// CODE_MOSAIC_A				Mosaic A
    false,	// CODE_MOSAIC_B				Mosaic B
    false,	// CODE_MOSAIC_C				Mosaic C
    false,	// CODE_MOSAIC_D				Mosaic D
    false,	// CODE_PROP_ALPHANUMERIC		Proportional Alphanumeric
    false,	// CODE_PROP_HIRAGANA			Proportional Hiragana
    false,	// CODE_PROP_KATAKANA			Proportional Katakana
    false,	// CODE_JIS_X0201_KATAKANA		JIS X 0201 Katakana
    true,	// CODE_JIS_KANJI_PLANE_1		JIS compatible Kanji Plane 1
    true,	// CODE_JIS_KANJI_PLANE_2		JIS compatible Kanji Plane 2
    true,	// CODE_ADDITIONAL_SYMBOLS		Additional symbols
    true,	// CODE_DRCS_0					DRCS-0
    false,	// CODE_DRCS_1					DRCS-1
    false,	// CODE_DRCS_2					DRCS-2
    false,	// CODE_DRCS_3					DRCS-3
    false,	// CODE_DRCS_4					DRCS-4
    false,	// CODE_DRCS_5					DRCS-5
    false,	// CODE_DRCS_6					DRCS-6
    false,	// CODE_DRCS_7					DRCS-7
    false,	// CODE_DRCS_8					DRCS-8
    false,	// CODE_DRCS_9					DRCS-9
    false,	// CODE_DRCS_10					DRCS-10
    false,	// CODE_DRCS_11					DRCS-11
    false,	// CODE_DRCS_12					DRCS-12
    false,	// CODE_DRCS_13					DRCS-13
    false,	// CODE_DRCS_14					DRCS-14
    false,	// CODE_DRCS_15					DRCS-15
    false,	// CODE_MACRO					Macro
};

class CAribString {
public:
    enum CHAR_SIZE {
        SIZE_SMALL,		// 小型
        SIZE_MEDIUM,	// 中型
        SIZE_NORMAL,	// 標準
        SIZE_MICRO,		// 超小型
        SIZE_HIGH_W,	// 縦倍
        SIZE_WIDTH_W,	// 横倍
        SIZE_W,			// 縦横倍
        SIZE_SPECIAL_1,	// 特殊1
        SIZE_SPECIAL_2	// 特殊2
    };

    struct FormatInfo {
        DWORD Pos;
        CHAR_SIZE Size;
        BYTE CharColorIndex;
        BYTE BackColorIndex;
        BYTE RasterColorIndex;
        bool operator==(const FormatInfo &o) {
            return Pos == o.Pos
                && Size == o.Size
                && CharColorIndex == o.CharColorIndex
                && BackColorIndex == o.BackColorIndex
                && RasterColorIndex == o.RasterColorIndex;
        }
        bool operator!=(const FormatInfo &o) { return !(*this == o); }
    };
    typedef std::vector<FormatInfo> FormatList;

    class IDRCSMap {
    public:
        virtual ~IDRCSMap() {}
        virtual LPCWSTR GetString(WORD Code) = 0;
    };

    static const DWORD AribToString(LPWSTR lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen) {
        // ARIB STD-B24 Part1 → Shift-JIS / Unicode変換
        CAribString WorkObject;

        return WorkObject.AribToStringInternal(lpszDst, dwDstLen, pSrcData, dwSrcLen);
    }
    static const DWORD CaptionToString(LPWSTR lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen, FormatList *pFormatList = NULL, IDRCSMap *pDRCSMap = NULL) {
        CAribString WorkObject;

        return WorkObject.AribToStringInternal(lpszDst, dwDstLen, pSrcData, dwSrcLen, true, pFormatList, pDRCSMap);
    }

private:
    enum CODE_SET {
        CODE_UNKNOWN,				// 不明なグラフィックセット(非対応)
        CODE_KANJI,					// Kanji
        CODE_ALPHANUMERIC,			// Alphanumeric
        CODE_HIRAGANA,				// Hiragana
        CODE_KATAKANA,				// Katakana
        CODE_MOSAIC_A,				// Mosaic A
        CODE_MOSAIC_B,				// Mosaic B
        CODE_MOSAIC_C,				// Mosaic C
        CODE_MOSAIC_D,				// Mosaic D
        CODE_PROP_ALPHANUMERIC,		// Proportional Alphanumeric
        CODE_PROP_HIRAGANA,			// Proportional Hiragana
        CODE_PROP_KATAKANA,			// Proportional Katakana
        CODE_JIS_X0201_KATAKANA,	// JIS X 0201 Katakana
        CODE_JIS_KANJI_PLANE_1,		// JIS compatible Kanji Plane 1
        CODE_JIS_KANJI_PLANE_2,		// JIS compatible Kanji Plane 2
        CODE_ADDITIONAL_SYMBOLS,	// Additional symbols
        CODE_DRCS_0,				// DRCS-0
        CODE_DRCS_1,				// DRCS-1
        CODE_DRCS_2,				// DRCS-2
        CODE_DRCS_3,				// DRCS-3
        CODE_DRCS_4,				// DRCS-4
        CODE_DRCS_5,				// DRCS-5
        CODE_DRCS_6,				// DRCS-6
        CODE_DRCS_7,				// DRCS-7
        CODE_DRCS_8,				// DRCS-8
        CODE_DRCS_9,				// DRCS-9
        CODE_DRCS_10,				// DRCS-10
        CODE_DRCS_11,				// DRCS-11
        CODE_DRCS_12,				// DRCS-12
        CODE_DRCS_13,				// DRCS-13
        CODE_DRCS_14,				// DRCS-14
        CODE_DRCS_15,				// DRCS-15
        CODE_MACRO					// Macro
    };

    CODE_SET m_CodeG[4];
    CODE_SET *m_pLockingGL;
    CODE_SET *m_pLockingGR;
    CODE_SET *m_pSingleGL;

    BYTE m_byEscSeqCount;
    BYTE m_byEscSeqIndex;
    bool m_bIsEscSeqDrcs;

    CHAR_SIZE m_CharSize;
    BYTE m_CharColorIndex;
    BYTE m_BackColorIndex;
    BYTE m_RasterColorIndex;
    BYTE m_DefPalette;
    BYTE m_RPC;
    FormatList *m_pFormatList;
    IDRCSMap *m_pDRCSMap;

    bool m_bCaption;

    const DWORD AribToStringInternal(LPWSTR lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen,
        const bool bCaption = false, FormatList *pFormatList = NULL, IDRCSMap *pDRCSMap = NULL) {
        if (pSrcData == NULL || lpszDst == NULL || dwDstLen == 0)
            return 0UL;

        // 状態初期設定
        m_byEscSeqCount = 0U;
        m_pSingleGL = NULL;

        m_CodeG[0] = CODE_KANJI;
        m_CodeG[1] = CODE_ALPHANUMERIC;
        m_CodeG[2] = CODE_HIRAGANA;
        m_CodeG[3] = bCaption ? CODE_MACRO : CODE_KATAKANA;

        m_pLockingGL = &m_CodeG[0];
        m_pLockingGR = &m_CodeG[2];
#ifdef BONTSENGINE_1SEG_SUPPORT
        if (bCaption) {
            m_CodeG[1] = CODE_DRCS_1;
            m_pLockingGL = &m_CodeG[1];
            m_pLockingGR = &m_CodeG[0];
        }
#endif

        m_CharSize = SIZE_NORMAL;
        if (bCaption) {
            m_CharColorIndex = 7;
            m_BackColorIndex = 8;
            m_RasterColorIndex = 8;
        } else {
            m_CharColorIndex = 0;
            m_BackColorIndex = 0;
            m_RasterColorIndex = 0;
        }
        m_DefPalette = 0;
        m_RPC = 1;

        m_bCaption = bCaption;
        m_pFormatList = pFormatList;
        m_pDRCSMap = pDRCSMap;

        return ProcessString(lpszDst, dwDstLen, pSrcData, dwSrcLen);
    }

    const DWORD ProcessString(LPWSTR lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen) {
        DWORD dwSrcPos = 0UL, dwDstPos = 0UL;
        int Length;

        while (dwSrcPos < dwSrcLen && dwDstPos < dwDstLen - 1) {
            if (!m_byEscSeqCount) {
                // GL/GR領域
                if ((pSrcData[dwSrcPos] >= 0x21U) && (pSrcData[dwSrcPos] <= 0x7EU)) {
                    // GL領域
                    const CODE_SET CurCodeSet = (m_pSingleGL) ? *m_pSingleGL : *m_pLockingGL;
                    m_pSingleGL = NULL;

                    if (abCharSizeTable[CurCodeSet]) {
                        // 2バイトコード
                        if ((dwSrcLen - dwSrcPos) < 2UL)
                            break;

                        Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, ((WORD)pSrcData[dwSrcPos + 0] << 8) | (WORD)pSrcData[dwSrcPos + 1], CurCodeSet);
                        if (Length < 0)
                            break;
                        dwDstPos += Length;
                        dwSrcPos++;
                    } else {
                        // 1バイトコード
                        Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)pSrcData[dwSrcPos], CurCodeSet);
                        if (Length < 0)
                            break;
                        dwDstPos += Length;
                    }
                } else if ((pSrcData[dwSrcPos] >= 0xA1U) && (pSrcData[dwSrcPos] <= 0xFEU)) {
                    // GR領域
                    const CODE_SET CurCodeSet = *m_pLockingGR;

                    if (abCharSizeTable[CurCodeSet]) {
                        // 2バイトコード
                        if ((dwSrcLen - dwSrcPos) < 2UL) break;

                        Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, ((WORD)(pSrcData[dwSrcPos + 0] & 0x7FU) << 8) | (WORD)(pSrcData[dwSrcPos + 1] & 0x7FU), CurCodeSet);
                        if (Length < 0)
                            break;
                        dwDstPos += Length;
                        dwSrcPos++;
                    } else {
                        // 1バイトコード
                        Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)(pSrcData[dwSrcPos] & 0x7FU), CurCodeSet);
                        if (Length < 0)
                            break;
                        dwDstPos += Length;
                    }
                } else {
                    // 制御コード
                    switch (pSrcData[dwSrcPos]) {
                    case 0x0D:	// APR
                        lpszDst[dwDstPos++] = '\r';
                        if (dwDstPos + 1 < dwDstLen)
                            lpszDst[dwDstPos++] = '\n';
                        break;
                    case 0x0F:	LockingShiftGL(0U);					break;	// LS0
                    case 0x0E:	LockingShiftGL(1U);					break;	// LS1
                    case 0x19:	SingleShiftGL(2U);					break;	// SS2
                    case 0x1D:	SingleShiftGL(3U);					break;	// SS3
                    case 0x1B:	m_byEscSeqCount = 1U;				break;	// ESC
                    case 0x20:	// SP
                        if (IsSmallCharMode()) {
                            lpszDst[dwDstPos++] = _T(' ');
                        } else {
#ifdef _UNICODE
                            lpszDst[dwDstPos++] = L'　';
#else
                            if (dwDstPos + 2 < dwDstLen) {
                                lpszDst[dwDstPos++] = "　"[0];
                                lpszDst[dwDstPos++] = "　"[1];
                            } else {
                                lpszDst[dwDstPos++] = ' ';
                            }
#endif
                        }
                        break;
                    case 0xA0:	lpszDst[dwDstPos++] = _T(' ');	break;	// SP

                    case 0x80:
                    case 0x81:
                    case 0x82:
                    case 0x83:
                    case 0x84:
                    case 0x85:
                    case 0x86:
                    case 0x87:
                        m_CharColorIndex = (m_DefPalette << 4) | (pSrcData[dwSrcPos] & 0x0F);
                        SetFormat(dwDstPos);
                        break;

                    case 0x88:	// SSZ 小型
                        m_CharSize = SIZE_SMALL;
                        SetFormat(dwDstPos);
                        break;
                    case 0x89:	// MSZ 中型
                        m_CharSize = SIZE_MEDIUM;
                        SetFormat(dwDstPos);
                        break;
                    case 0x8A:	// NSZ 標準
                        m_CharSize = SIZE_NORMAL;
                        SetFormat(dwDstPos);
                        break;
                    case 0x8B:	// SZX 指定サイズ
                        if (++dwSrcPos >= dwSrcLen)
                            break;
                        switch (pSrcData[dwSrcPos]) {
                        case 0x60:	m_CharSize = SIZE_MICRO;		break;	// 超小型
                        case 0x41:	m_CharSize = SIZE_HIGH_W;		break;	// 縦倍
                        case 0x44:	m_CharSize = SIZE_WIDTH_W;		break;	// 横倍
                        case 0x45:	m_CharSize = SIZE_W;			break;	// 縦横倍
                        case 0x6B:	m_CharSize = SIZE_SPECIAL_1;	break;	// 特殊1
                        case 0x64:	m_CharSize = SIZE_SPECIAL_2;	break;	// 特殊2
                        }
                        SetFormat(dwDstPos);
                        break;

                    case 0x0C: //CS
                        lpszDst[dwDstPos++] = '\f';
                        break;
                    case 0x16:	dwSrcPos++;		break;	// PAPF
                    case 0x1C:	dwSrcPos += 2;	break;	// APS
                    case 0x90:	// COL
                        if (++dwSrcPos >= dwSrcLen)
                            break;
                        if (pSrcData[dwSrcPos] == 0x20) {
                            m_DefPalette = pSrcData[++dwSrcPos] & 0x0F;
                        } else {
                            switch (pSrcData[dwSrcPos] & 0xF0) {
                            case 0x40:
                                m_CharColorIndex = pSrcData[dwSrcPos] & 0x0F;
                                break;
                            case 0x50:
                                m_BackColorIndex = pSrcData[dwSrcPos] & 0x0F;
                                break;
                            }
                            SetFormat(dwDstPos);
                        }
                        break;
                    case 0x91:	dwSrcPos++;		break;	// FLC
                    case 0x93:	dwSrcPos++;		break;	// POL
                    case 0x94:	dwSrcPos++;		break;	// WMM
                    case 0x95:	// MACRO
                        do {
                            if (++dwSrcPos >= dwSrcLen)
                                break;
                        } while (pSrcData[dwSrcPos] != 0x4F);
                        break;
                    case 0x97:	dwSrcPos++;		break;	// HLC
                    case 0x98:	// RPC
                        if (++dwSrcPos >= dwSrcLen)
                            break;
                        m_RPC = pSrcData[dwSrcPos] & 0x3F;
                        break;
                    case 0x9B:	//CSI
                        for (Length = 0; ++dwSrcPos < dwSrcLen && pSrcData[dwSrcPos] <= 0x3B; Length++);
                        if (dwSrcPos < dwSrcLen) {
                            if (pSrcData[dwSrcPos] == 0x69) {	// ACS
                                if (Length != 2)
                                    goto End;
                                if (pSrcData[dwSrcPos - 2] >= 0x32) {
                                    while (++dwSrcPos < dwSrcLen && pSrcData[dwSrcPos] != 0x9B);
                                    dwSrcPos += 3;
                                }
                            }
                        }
                        break;
                    case 0x9D:	// TIME
                        if (++dwSrcPos >= dwSrcLen)
                            break;
                        if (pSrcData[dwSrcPos] == 0x20) {
                            dwSrcPos++;
                        } else {
                            while (pSrcData[dwSrcPos] < 0x40 || pSrcData[dwSrcPos] > 0x43)
                                dwSrcPos++;
                        }
                        break;
                    default:	break;	// 非対応
                    }
                }
            } else {
                // エスケープシーケンス処理
                ProcessEscapeSeq(pSrcData[dwSrcPos]);
            }

            dwSrcPos++;
        }

    End:
        // 終端文字
        lpszDst[dwDstPos] = _T('\0');

        return dwDstPos;
    }

    inline const int ProcessCharCode(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode, const CODE_SET CodeSet) {
        int Length;

        switch (CodeSet) {
        case CODE_KANJI:
        case CODE_JIS_KANJI_PLANE_1:
        case CODE_JIS_KANJI_PLANE_2:
            // 漢字コード出力
            Length = PutKanjiChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_ALPHANUMERIC:
        case CODE_PROP_ALPHANUMERIC:
            // 英数字コード出力
            Length = PutAlphanumericChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_HIRAGANA:
        case CODE_PROP_HIRAGANA:
            // ひらがなコード出力
            Length = PutHiraganaChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_PROP_KATAKANA:
        case CODE_KATAKANA:
            // カタカナコード出力
            Length = PutKatakanaChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_JIS_X0201_KATAKANA:
            // JISカタカナコード出力
            Length = PutJisKatakanaChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_ADDITIONAL_SYMBOLS:
            // 追加シンボルコード出力
            Length = PutSymbolsChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_MACRO:
            Length = PutMacroChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_DRCS_0:
            Length = PutDRCSChar(lpszDst, dwDstLen, wCode);
            break;

        case CODE_DRCS_1:
        case CODE_DRCS_2:
        case CODE_DRCS_3:
        case CODE_DRCS_4:
        case CODE_DRCS_5:
        case CODE_DRCS_6:
        case CODE_DRCS_7:
        case CODE_DRCS_8:
        case CODE_DRCS_9:
        case CODE_DRCS_10:
        case CODE_DRCS_11:
        case CODE_DRCS_12:
        case CODE_DRCS_13:
        case CODE_DRCS_14:
        case CODE_DRCS_15:
            Length = PutDRCSChar(lpszDst, dwDstLen, ((CodeSet - CODE_DRCS_0 + 0x40) << 8) | wCode);
            break;

        default:
#ifdef _UNICODE
            lpszDst[0] = u'□';
            Length = 1;
#else
            if (dwDstLen < 2)
                return -1;
            lpszDst[0] = "□"[0];
            lpszDst[1] = "□"[1];
            Length = 2;
#endif
        }

        if (m_RPC > 1 && Length == 1 && dwDstLen > 1) {
            DWORD Count = std::min((DWORD)m_RPC - 1, dwDstLen - Length);
            while (Count-- > 0)
                lpszDst[Length++] = lpszDst[0];
        }
        m_RPC = 1;

        return Length;
    }

    inline const int PutKanjiChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        if (wCode >= 0x7521)
            return PutSymbolsChar(lpszDst, dwDstLen, wCode);

        // JIS → Shift_JIS漢字コード変換
        BYTE First = (BYTE)(wCode >> 8), Second = (BYTE)(wCode & 0x00FF);
        First -= 0x21;
        if ((First & 0x01) == 0) {
            Second += 0x1F;
            if (Second >= 0x7F)
                Second++;
        } else {
            Second += 0x7E;
        }
        First >>= 1;
        if (First >= 0x1F)
            First += 0xC1;
        else
            First += 0x81;

#ifdef _UNICODE
  #ifdef _WIN32
        // Shift_JIS → UNICODE
        if (dwDstLen < 1)
            return -1;
        char cShiftJIS[2];
        cShiftJIS[0] = (char)First;
        cShiftJIS[1] = (char)Second;
        // Shift_JIS = Code page 932
        int Length = ::MultiByteToWideChar(932, MB_PRECOMPOSED, cShiftJIS, 2, lpszDst, dwDstLen);
        if (Length == 0) {
            lpszDst[0] = u'□';
            return 1;
        }
        return Length;
  #else
        // Shift_JIS → UNICODE
        if (dwDstLen < 1)
            return -1;
        char cShiftJIS[2];
        cShiftJIS[0] = (char)First;
        cShiftJIS[1] = (char)Second;

        char* inbuf = cShiftJIS;
        size_t inbytes = 2;
        char* outbuf = (char*)lpszDst;
        size_t outbytes = dwDstLen;

        iconv_t ic;
        if ((ic = iconv_open("SJIS", "UTF-16")) == (iconv_t) -1) {
            return -1;
        }
        if (iconv(ic, &inbuf, &inbytes, &outbuf, &outbytes) == (size_t) -1) {
            return -1;
        }
        iconv_close(ic);

        return dwDstLen - outbytes;
  #endif
#else
        // Shift_JIS → Shift_JIS
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = (char)First;
        lpszDst[1] = (char)Second;
        return 2;
#endif
    }

    inline const int PutAlphanumericChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        // 英数字文字コード変換
        static const LPCWSTR acAlphanumericTable =
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　！”＃＄％＆’（）＊＋，－．／")
            _T(u"０１２３４５６７８９：；＜＝＞？")
            _T(u"＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ")
            _T(u"ＰＱＲＳＴＵＶＷＸＹＺ［￥］＾＿")
            _T(u"　ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ")
            _T(u"ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝￣　");

#ifdef _UNICODE
        if (dwDstLen < 1)
            return -1;
        lpszDst[0] = acAlphanumericTable[wCode];

        return 1;
#else
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = acAlphanumericTable[wCode * 2U + 0U];
        lpszDst[1] = acAlphanumericTable[wCode * 2U + 1U];

        return 2;
#endif
    }

    inline const int PutHiraganaChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        // ひらがな文字コード変換
        static const LPCWSTR acHiraganaTable =
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　ぁあぃいぅうぇえぉおかがきぎく")
            _T(u"ぐけげこごさざしじすずせぜそぞた")
            _T(u"だちぢっつづてでとどなにぬねのは")
            _T(u"ばぱひびぴふぶぷへべぺほぼぽまみ")
            _T(u"むめもゃやゅゆょよらりるれろゎわ")
            _T(u"ゐゑをん　　　ゝゞー。「」、・　");

#ifdef _UNICODE
        if (dwDstLen < 1)
            return -1;
        lpszDst[0] = acHiraganaTable[wCode];

        return 1;
#else
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = acHiraganaTable[wCode * 2U + 0U];
        lpszDst[1] = acHiraganaTable[wCode * 2U + 1U];

        return 2;
#endif
    }

    inline const int PutKatakanaChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        // カタカナ文字コード変換
        static const LPCWSTR acKatakanaTable =
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　ァアィイゥウェエォオカガキギク")
            _T(u"グケゲコゴサザシジスズセゼソゾタ")
            _T(u"ダチヂッツヅテデトドナニヌネノハ")
            _T(u"バパヒビピフブプヘベペホボポマミ")
            _T(u"ムメモャヤュユョヨラリルレロヮワ")
            _T(u"ヰヱヲンヴヵヶヽヾー。「」、・　");

#ifdef _UNICODE
        if (dwDstLen < 1)
            return -1;
        lpszDst[0] = acKatakanaTable[wCode];

        return 1;
#else
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = acKatakanaTable[wCode * 2U + 0U];
        lpszDst[1] = acKatakanaTable[wCode * 2U + 1U];

        return 2;
#endif
    }

    inline const int PutJisKatakanaChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        // JISカタカナ文字コード変換
        static const LPCWSTR acJisKatakanaTable =
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　。「」、・ヲァィゥェォャュョッ")
            _T(u"ーアイウエオカキクケコサシスセソ")
            _T(u"タチツテトナニヌネノハヒフヘホマ")
            _T(u"ミムメモヤユヨラリルレロワン゛゜")
            _T(u"　　　　　　　　　　　　　　　　")
            _T(u"　　　　　　　　　　　　　　　　");

#ifdef _UNICODE
        if (dwDstLen < 1)
            return -1;
        lpszDst[0] = acJisKatakanaTable[wCode];

        return 1;
#else
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = acJisKatakanaTable[wCode * 2U + 0U];
        lpszDst[1] = acJisKatakanaTable[wCode * 2U + 1U];

        return 2;
#endif
    }

    inline const int PutSymbolsChar(char16_t *lpszDst, const DWORD dwDstLen, const WORD wCode) {
        // 追加シンボル文字コード変換(とりあえず必要そうなものだけ)
        static const LPCWSTR aszSymbolsTable1[] =
        {
            _T(u"[HV]"),		_T(u"[SD]"),		_T(u"[Ｐ]"),		_T(u"[Ｗ]"),		_T(u"[MV]"),		_T(u"[手]"),		_T(u"[字]"),		_T(u"[双]"),			// 0x7A50 - 0x7A57	90/48 - 90/55
            _T(u"[デ]"),		_T(u"[Ｓ]"),		_T(u"[二]"),		_T(u"[多]"),		_T(u"[解]"),		_T(u"[SS]"),		_T(u"[Ｂ]"),		_T(u"[Ｎ]"),			// 0x7A58 - 0x7A5F	90/56 - 90/63
            _T(u"■"),		_T(u"●"),		_T(u"[天]"),		_T(u"[交]"),		_T(u"[映]"),		_T(u"[無]"),		_T(u"[料]"),		_T(u"[年齢制限]"),	// 0x7A60 - 0x7A67	90/64 - 90/71
            _T(u"[前]"),		_T(u"[後]"),		_T(u"[再]"),		_T(u"[新]"),		_T(u"[初]"),		_T(u"[終]"),		_T(u"[生]"),		_T(u"[販]"),			// 0x7A68 - 0x7A6F	90/72 - 90/79
            _T(u"[声]"),		_T(u"[吹]"),		_T(u"[PPV]"),	_T(u"(秘)"),		_T(u"ほか")															// 0x7A70 - 0x7A74	90/80 - 90/84
        };

#ifndef USE_UNICODE_CHAR
        static const LPCWSTR aszSymbolsTable2[] =
        {
            _T(u"→"),		_T(u"←"),		_T(u"↑"),		_T(u"↓"),		_T(u"○"),		_T(u"●"),		_T(u"年"),		_T(u"月"),			// 0x7C21 - 0x7C28	92/01 - 92/08
            _T(u"日"),		_T(u"円"),		_T(u"㎡"),		_T(u"立方ｍ"),	_T(u"㎝"),		_T(u"平方㎝"),	_T(u"立方㎝"),	_T(u"０."),			// 0x7C29 - 0x7C30	92/09 - 92/16
            _T(u"１."),		_T(u"２."),		_T(u"３."),		_T(u"４."),		_T(u"５."),		_T(u"６."),		_T(u"７."),		_T(u"８."),			// 0x7C31 - 0x7C38	92/17 - 92/24
            _T(u"９."),		_T(u"氏"),		_T(u"副"),		_T(u"元"),		_T(u"故"),		_T(u"前"),		_T(u"新"),		_T(u"０,"),			// 0x7C39 - 0x7C40	92/25 - 92/32
            _T(u"１,"),		_T(u"２,"),		_T(u"３,"),		_T(u"４,"),		_T(u"５,"),		_T(u"６,"),		_T(u"７,"),		_T(u"８,"),			// 0x7C41 - 0x7C48	92/33 - 92/40
            _T(u"９,"),		_T(u"(社)"),		_T(u"(財)"),		_T(u"(有)"),		_T(u"(株)"),		_T(u"(代)"),		_T(u"(問)"),		_T(u"＞"),			// 0x7C49 - 0x7C50	92/41 - 92/48
            _T(u"＜"),		_T(u"【"),		_T(u"】"),		_T(u"◇"),		_T(u"^2"),		_T(u"^3"),		_T(u"(CD)"),		_T(u"(vn)"),			// 0x7C51 - 0x7C58	92/49 - 92/56
            _T(u"(ob)"),		_T(u"(cb)"),		_T(u"(ce"),		_T(u"mb)"),		_T(u"(hp)"),		_T(u"(br)"),		_T(u"(p)"),		_T(u"(s)"),			// 0x7C59 - 0x7C60	92/57 - 92/64
            _T(u"(ms)"),		_T(u"(t)"),		_T(u"(bs)"),		_T(u"(b)"),		_T(u"(tb)"),		_T(u"(tp)"),		_T(u"(ds)"),		_T(u"(ag)"),			// 0x7C61 - 0x7C68	92/65 - 92/72
            _T(u"(eg)"),		_T(u"(vo)"),		_T(u"(fl)"),		_T(u"(ke"),		_T(u"y)"),		_T(u"(sa"),		_T(u"x)"),		_T(u"(sy"),			// 0x7C69 - 0x7C70	92/73 - 92/80
            _T(u"n)"),		_T(u"(or"),		_T(u"g)"),		_T(u"(pe"),		_T(u"r)"),		_T(u"(R)"),		_T(u"(C)"),		_T(u"(箏)"),			// 0x7C71 - 0x7C78	92/81 - 92/88
            _T(u"DJ"),		_T(u"[演]"),		_T(u"Fax")																							// 0x7C79 - 0x7C7B	92/89 - 92/91
        };
#else
        static const LPCWSTR aszSymbolsTable2[] =
        {
            _T(u"→"),		_T(u"←"),		_T(u"↑"),		_T(u"↓"),		_T(u"○"),		_T(u"●"),		_T(u"年"),		_T(u"月"),			// 0x7C21 - 0x7C28	92/01 - 92/08
            _T(u"日"),		_T(u"円"),		_T(u"㎡"),		_T(u"㎥"),		_T(u"㎝"),		_T(u"㎠"),		_T(u"㎤"),		_T(u"０."),			// 0x7C29 - 0x7C30	92/09 - 92/16
            _T(u"⒈"),		_T(u"⒉"),		_T(u"⒊"),		_T(u"⒋"),		_T(u"⒌"),		_T(u"⒍"),		_T(u"⒎"),		_T(u"⒏"),			// 0x7C31 - 0x7C38	92/17 - 92/24
            _T(u"⒐"),		_T(u"氏"),		_T(u"副"),		_T(u"元"),		_T(u"故"),		_T(u"前"),		_T(u"新"),		_T(u"０,"),			// 0x7C39 - 0x7C40	92/25 - 92/32
            _T(u"１,"),		_T(u"２,"),		_T(u"３,"),		_T(u"４,"),		_T(u"５,"),		_T(u"６,"),		_T(u"７,"),		_T(u"８,"),			// 0x7C41 - 0x7C48	92/33 - 92/40
            _T(u"９,"),		_T(u"(社)"),		_T(u"(財)"),		_T(u"(有)"),		_T(u"(株)"),		_T(u"(代)"),		_T(u"(問)"),		_T(u"▶"),			// 0x7C49 - 0x7C50	92/41 - 92/48
            _T(u"◀"),		_T(u"〖"),		_T(u"〗"),		_T(u"◇"),		_T(u"^2"),		_T(u"^3"),		_T(u"(CD)"),		_T(u"(vn)"),			// 0x7C51 - 0x7C58	92/49 - 92/56
            _T(u"(ob)"),		_T(u"(cb)"),		_T(u"(ce"),		_T(u"mb)"),		_T(u"(hp)"),		_T(u"(br)"),		_T(u"(p)"),		_T(u"(s)"),			// 0x7C59 - 0x7C60	92/57 - 92/64
            _T(u"(ms)"),		_T(u"(t)"),		_T(u"(bs)"),		_T(u"(b)"),		_T(u"(tb)"),		_T(u"(tp)"),		_T(u"(ds)"),		_T(u"(ag)"),			// 0x7C61 - 0x7C68	92/65 - 92/72
            _T(u"(eg)"),		_T(u"(vo)"),		_T(u"(fl)"),		_T(u"(ke"),		_T(u"y)"),		_T(u"(sa"),		_T(u"x)"),		_T(u"(sy"),			// 0x7C69 - 0x7C70	92/73 - 92/80
            _T(u"n)"),		_T(u"(or"),		_T(u"g)"),		_T(u"(pe"),		_T(u"r)"),		_T(u"Ⓡ"),		_T(u"Ⓒ"),		_T(u"(箏)"),			// 0x7C71 - 0x7C78	92/81 - 92/88
            _T(u"DJ"),		_T(u"[演]"),		_T(u"Fax")																							// 0x7C79 - 0x7C7B	92/89 - 92/91
        };
#endif

#ifndef USE_UNICODE_CHAR
        static const LPCWSTR aszSymbolsTable3[] =
        {
            _T(u"(月)"),		_T(u"(火)"),		_T(u"(水)"),		_T(u"(木)"),		_T(u"(金)"),		_T(u"(土)"),		_T(u"(日)"),		_T(u"(祝)"),			// 0x7D21 - 0x7D28	93/01 - 93/08
            _T(u"㍾"),		_T(u"㍽"),		_T(u"㍼"),		_T(u"㍻"),		_T(u"№"),		_T(u"℡"),		_T(u"(〒)"),		_T(u"○"),			// 0x7D29 - 0x7D30	93/09 - 93/16
            _T(u"〔本〕"),	_T(u"〔三〕"),	_T(u"〔二〕"),	_T(u"〔安〕"),	_T(u"〔点〕"),	_T(u"〔打〕"),	_T(u"〔盗〕"),	_T(u"〔勝〕"),		// 0x7D31 - 0x7D38	93/17 - 93/24
            _T(u"〔敗〕"),	_T(u"〔Ｓ〕"),	_T(u"［投］"),	_T(u"［捕］"),	_T(u"［一］"),	_T(u"［二］"),	_T(u"［三］"),	_T(u"［遊］"),		// 0x7D39 - 0x7D40	93/25 - 93/32
            _T(u"［左］"),	_T(u"［中］"),	_T(u"［右］"),	_T(u"［指］"),	_T(u"［走］"),	_T(u"［打］"),	_T(u"㍑"),		_T(u"㎏"),			// 0x7D41 - 0x7D48	93/33 - 93/40
            _T(u"Hz"),		_T(u"ha"),		_T(u"km"),		_T(u"平方km"),	_T(u"hPa"),		_T(u"・"),		_T(u"・"),		_T(u"1/2"),			// 0x7D49 - 0x7D50	93/41 - 93/48
            _T(u"0/3"),		_T(u"1/3"),		_T(u"2/3"),		_T(u"1/4"),		_T(u"3/4"),		_T(u"1/5"),		_T(u"2/5"),		_T(u"3/5"),			// 0x7D51 - 0x7D58	93/49 - 93/56
            _T(u"4/5"),		_T(u"1/6"),		_T(u"5/6"),		_T(u"1/7"),		_T(u"1/8"),		_T(u"1/9"),		_T(u"1/10"),		_T(u"晴れ"),			// 0x7D59 - 0x7D60	93/57 - 93/64
            _T(u"曇り"),		_T(u"雨"),		_T(u"雪"),		_T(u"△"),		_T(u"▲"),		_T(u"▽"),		_T(u"▼"),		_T(u"◆"),			// 0x7D61 - 0x7D68	93/65 - 93/72
            _T(u"・"),		_T(u"・"),		_T(u"・"),		_T(u"◇"),		_T(u"◎"),		_T(u"!!"),		_T(u"!?"),		_T(u"曇/晴"),		// 0x7D69 - 0x7D70	93/73 - 93/80
            _T(u"雨"),		_T(u"雨"),		_T(u"雪"),		_T(u"大雪"),		_T(u"雷"),		_T(u"雷雨"),		_T(u"　"),		_T(u"・"),			// 0x7D71 - 0x7D78	93/81 - 93/88
            _T(u"・"),		_T(u"♪"),		_T(u"℡")																							// 0x7D79 - 0x7D7B	93/89 - 93/91
        };
#else
        static const LPCWSTR aszSymbolsTable3[] =
        {
            _T(u"㈪"),		_T(u"㈫"),		_T(u"㈬"),		_T(u"㈭"),		_T(u"㈮"),		_T(u"㈯"),		_T(u"㈰"),		_T(u"㈷"),			// 0x7D21 - 0x7D28	93/01 - 93/08
            _T(u"㍾"),		_T(u"㍽"),		_T(u"㍼"),		_T(u"㍻"),		_T(u"№"),		_T(u"℡"),		_T(u"(〒)"),		_T(u"○"),			// 0x7D29 - 0x7D30	93/09 - 93/16
            _T(u"〔本〕"),	_T(u"〔三〕"),	_T(u"〔二〕"),	_T(u"〔安〕"),	_T(u"〔点〕"),	_T(u"〔打〕"),	_T(u"〔盗〕"),	_T(u"〔勝〕"),		// 0x7D31 - 0x7D38	93/17 - 93/24
            _T(u"〔敗〕"),	_T(u"〔Ｓ〕"),	_T(u"［投］"),	_T(u"［捕］"),	_T(u"［一］"),	_T(u"［二］"),	_T(u"［三］"),	_T(u"［遊］"),		// 0x7D39 - 0x7D40	93/25 - 93/32
            _T(u"［左］"),	_T(u"［中］"),	_T(u"［右］"),	_T(u"［指］"),	_T(u"［走］"),	_T(u"［打］"),	_T(u"㍑"),		_T(u"㎏"),			// 0x7D41 - 0x7D48	93/33 - 93/40
            _T(u"㎐"),		_T(u"㏊"),		_T(u"㎞"),		_T(u"㎢"),		_T(u"㍱"),		_T(u"・"),		_T(u"・"),		_T(u"1/2"),			// 0x7D49 - 0x7D50	93/41 - 93/48
            _T(u"0/3"),		_T(u"⅓"),		_T(u"⅔"),		_T(u"1/4"),		_T(u"3/4"),		_T(u"⅕"),		_T(u"⅖"),		_T(u"⅗"),			// 0x7D51 - 0x7D58	93/49 - 93/56
            _T(u"⅘"),		_T(u"⅙"),		_T(u"⅚"),		_T(u"1/7"),		_T(u"1/8"),		_T(u"1/9"),		_T(u"1/10"),		_T(u"☀"),			// 0x7D59 - 0x7D60	93/57 - 93/64
            _T(u"☁"),		_T(u"☂"),		_T(u"☃"),		_T(u"⌂"),		_T(u"▲"),		_T(u"▽"),		_T(u"▼"),		_T(u"♦"),			// 0x7D61 - 0x7D68	93/65 - 93/72
            _T(u"♥"),		_T(u"♣"),		_T(u"♠"),		_T(u"◇"),		_T(u"☉"),		_T(u"!!"),		_T(u"⁉"),		_T(u"曇/晴"),		// 0x7D69 - 0x7D70	93/73 - 93/80
            _T(u"雨"),		_T(u"雨"),		_T(u"雪"),		_T(u"大雪"),		_T(u"雷"),		_T(u"雷雨"),		_T(u"　"),		_T(u"・"),			// 0x7D71 - 0x7D78	93/81 - 93/88
            _T(u"・"),		_T(u"♬"),		_T(u"☎")																							// 0x7D79 - 0x7D7B	93/89 - 93/91
        };
#endif

#ifndef USE_UNICODE_CHAR
        static const LPCWSTR aszSymbolsTable4[] =
        {
            _T(u"Ⅰ"),		_T(u"Ⅱ"),		_T(u"Ⅲ"),		_T(u"Ⅳ"),		_T(u"Ⅴ"),		_T(u"Ⅵ"),		_T(u"Ⅶ"),		_T(u"Ⅷ"),			// 0x7E21 - 0x7E28	94/01 - 94/08
            _T(u"Ⅸ"),		_T(u"Ⅹ"),		_T(u"XI"),		_T(u"XⅡ"),		_T(u"⑰"),		_T(u"⑱"),		_T(u"⑲"),		_T(u"⑳"),			// 0x7E29 - 0x7E30	94/09 - 94/16
            _T(u"(1)"),		_T(u"(2)"),		_T(u"(3)"),		_T(u"(4)"),		_T(u"(5)"),		_T(u"(6)"),		_T(u"(7)"),		_T(u"(8)"),			// 0x7E31 - 0x7E38	94/17 - 94/24
            _T(u"(9)"),		_T(u"(10)"),		_T(u"(11)"),		_T(u"(12)"),		_T(u"(21)"),		_T(u"(22)"),		_T(u"(23)"),		_T(u"(24)"),			// 0x7E39 - 0x7E40	94/25 - 94/32
            _T(u"(A)"),		_T(u"(B)"),		_T(u"(C)"),		_T(u"(D)"),		_T(u"(E)"),		_T(u"(F)"),		_T(u"(G)"),		_T(u"(H)"),			// 0x7E41 - 0x7E48	94/33 - 94/40
            _T(u"(I)"),		_T(u"(J)"),		_T(u"(K)"),		_T(u"(L)"),		_T(u"(M)"),		_T(u"(N)"),		_T(u"(O)"),		_T(u"(P)"),			// 0x7E49 - 0x7E50	94/41 - 94/48
            _T(u"(Q)"),		_T(u"(R)"),		_T(u"(S)"),		_T(u"(T)"),		_T(u"(U)"),		_T(u"(V)"),		_T(u"(W)"),		_T(u"(X)"),			// 0x7E51 - 0x7E58	94/49 - 94/56
            _T(u"(Y)"),		_T(u"(Z)"),		_T(u"(25)"),		_T(u"(26)"),		_T(u"(27)"),		_T(u"(28)"),		_T(u"(29)"),		_T(u"(30)"),			// 0x7E59 - 0x7E60	94/57 - 94/64
            _T(u"①"),		_T(u"②"),		_T(u"③"),		_T(u"④"),		_T(u"⑤"),		_T(u"⑥"),		_T(u"⑦"),		_T(u"⑧"),			// 0x7E61 - 0x7E68	94/65 - 94/72
            _T(u"⑨"),		_T(u"⑩"),		_T(u"⑪"),		_T(u"⑫"),		_T(u"⑬"),		_T(u"⑭"),		_T(u"⑮"),		_T(u"⑯"),			// 0x7E69 - 0x7E70	94/73 - 94/80
            _T(u"①"),		_T(u"②"),		_T(u"③"),		_T(u"④"),		_T(u"⑤"),		_T(u"⑥"),		_T(u"⑦"),		_T(u"⑧"),			// 0x7E71 - 0x7E78	94/81 - 94/88
            _T(u"⑨"),		_T(u"⑩"),		_T(u"⑪"),		_T(u"⑫"),		_T(u"(31)")															// 0x7E79 - 0x7E7D	94/89 - 94/93
        };
#else
        static const LPCWSTR aszSymbolsTable4[] =
        {
            _T(u"Ⅰ"),		_T(u"Ⅱ"),		_T(u"Ⅲ"),		_T(u"Ⅳ"),		_T(u"Ⅴ"),		_T(u"Ⅵ"),		_T(u"Ⅶ"),		_T(u"Ⅷ"),			// 0x7E21 - 0x7E28	94/01 - 94/08
            _T(u"Ⅸ"),		_T(u"Ⅹ"),		_T(u"XI"),		_T(u"XⅡ"),		_T(u"⑰"),		_T(u"⑱"),		_T(u"⑲"),		_T(u"⑳"),			// 0x7E29 - 0x7E30	94/09 - 94/16
            _T(u"⑴"),		_T(u"⑵"),		_T(u"⑶"),		_T(u"⑷"),		_T(u"⑸"),		_T(u"⑹"),		_T(u"⑺"),		_T(u"⑻"),			// 0x7E31 - 0x7E38	94/17 - 94/24
            _T(u"⑼"),		_T(u"⑽"),		_T(u"⑾"),		_T(u"⑿"),		_T(u"㉑"),		_T(u"㉒"),		_T(u"㉓"),		_T(u"㉔"),			// 0x7E39 - 0x7E40	94/25 - 94/32
            _T(u"(A)"),		_T(u"(B)"),		_T(u"(C)"),		_T(u"(D)"),		_T(u"(E)"),		_T(u"(F)"),		_T(u"(G)"),		_T(u"(H)"),			// 0x7E41 - 0x7E48	94/33 - 94/40
            _T(u"(I)"),		_T(u"(J)"),		_T(u"(K)"),		_T(u"(L)"),		_T(u"(M)"),		_T(u"(N)"),		_T(u"(O)"),		_T(u"(P)"),			// 0x7E49 - 0x7E50	94/41 - 94/48
            _T(u"(Q)"),		_T(u"(R)"),		_T(u"(S)"),		_T(u"(T)"),		_T(u"(U)"),		_T(u"(V)"),		_T(u"(W)"),		_T(u"(X)"),			// 0x7E51 - 0x7E58	94/49 - 94/56
            _T(u"(Y)"),		_T(u"(Z)"),		_T(u"㉕"),		_T(u"㉖"),		_T(u"㉗"),		_T(u"㉘"),		_T(u"㉙"),		_T(u"㉚"),			// 0x7E59 - 0x7E60	94/57 - 94/64
            _T(u"①"),		_T(u"②"),		_T(u"③"),		_T(u"④"),		_T(u"⑤"),		_T(u"⑥"),		_T(u"⑦"),		_T(u"⑧"),			// 0x7E61 - 0x7E68	94/65 - 94/72
            _T(u"⑨"),		_T(u"⑩"),		_T(u"⑪"),		_T(u"⑫"),		_T(u"⑬"),		_T(u"⑭"),		_T(u"⑮"),		_T(u"⑯"),			// 0x7E69 - 0x7E70	94/73 - 94/80
            _T(u"❶"),		_T(u"❷"),		_T(u"❸"),		_T(u"❹"),		_T(u"❺"),		_T(u"❻"),		_T(u"❼"),		_T(u"❽"),			// 0x7E71 - 0x7E78	94/81 - 94/88
            _T(u"❾"),		_T(u"❿"),		_T(u"⓫"),		_T(u"⓬"),		_T(u"㉛")															// 0x7E79 - 0x7E7D	94/89 - 94/93
        };
#endif
        static const LPCWSTR aszKanjiTable1[] = {
            _T(u"㐂"),		_T(u"𠅘"),		_T(u"份"),		_T(u"仿"),		_T(u"侚"),		_T(u"俉"),		_T(u"傜"),		_T(u"儞"),			// 0x7521 - 0x7528
            _T(u"冼"),		_T(u"㔟"),		_T(u"匇"),		_T(u"卡"),		_T(u"卬"),		_T(u"詹"),		_T(u"𠮷"),		_T(u"呍"),			// 0x7529 - 0x7530
            _T(u"咖"),		_T(u"咜"),		_T(u"咩"),		_T(u"唎"),		_T(u"啊"),		_T(u"噲"),		_T(u"囤"),		_T(u"圳"),			// 0x7531 - 0x7538
            _T(u"圴"),		_T(u"塚"),		_T(u"墀"),		_T(u"姤"),		_T(u"娣"),		_T(u"婕"),		_T(u"寬"),		_T(u"﨑"),			// 0x7539 - 0x7540
            _T(u"㟢"),		_T(u"庬"),		_T(u"弴"),		_T(u"彅"),		_T(u"德"),		_T(u"怗"),		_T(u"・"),		_T(u"愰"),			// 0x7541 - 0x7548
            _T(u"昤"),		_T(u"曈"),		_T(u"曙"),		_T(u"曺"),		_T(u"曻"),		_T(u"・"),		_T(u"・"),		_T(u"椑"),			// 0x7549 - 0x7551
            _T(u"椻"),		_T(u"橅"),		_T(u"檑"),		_T(u"櫛"),		_T(u"𣏌"),		_T(u"𣏾"),		_T(u"𣗄"),		_T(u"毱"),			// 0x7550 - 0x7558
            _T(u"泠"),		_T(u"洮"),		_T(u"海"),		_T(u"涿"),		_T(u"淊"),		_T(u"淸"),		_T(u"渚"),		_T(u"潞"),			// 0x7559 - 0x7560
            _T(u"濹"),		_T(u"灤"),		_T(u"𤋮"),		_T(u"𤋮"),		_T(u"煇"),		_T(u"燁"),		_T(u"爀"),		_T(u"玟"),			// 0x7561 - 0x7568
            _T(u"玨"),		_T(u"珉"),		_T(u"珖"),		_T(u"琛"),		_T(u"琡"),		_T(u"琢"),		_T(u"琦"),		_T(u"琪"),			// 0x7569 - 0x7570
            _T(u"琬"),		_T(u"琹"),		_T(u"瑋"),		_T(u"㻚"),		_T(u"畵"),		_T(u"疁"),		_T(u"睲"),		_T(u"䂓"),			// 0x7571 - 0x7578
            _T(u"磈"),		_T(u"磠"),		_T(u"祇"),		_T(u"禮"),		_T(u"・"),		_T(u"・"),											// 0x7579 - 0x757E
        };
        static const LPCWSTR aszKanjiTable2[] = {
            _T(u"・"),		_T(u"秚"),		_T(u"稞"),		_T(u"筿"),		_T(u"簱"),		_T(u"䉤"),		_T(u"綋"),		_T(u"羡"),			// 0x7621 - 0x7628
            _T(u"脘"),		_T(u"脺"),		_T(u"舘"),		_T(u"芮"),		_T(u"葛"),		_T(u"蓜"),		_T(u"蓬"),		_T(u"蕙"),			// 0x7629 - 0x7630
            _T(u"藎"),		_T(u"蝕"),		_T(u"蟬"),		_T(u"蠋"),		_T(u"裵"),		_T(u"角"),		_T(u"諶"),		_T(u"跎"),			// 0x7631 - 0x7638
            _T(u"辻"),		_T(u"迶"),		_T(u"郝"),		_T(u"鄧"),		_T(u"鄭"),		_T(u"醲"),		_T(u"鈳"),		_T(u"銈"),			// 0x7639 - 0x7640
            _T(u"錡"),		_T(u"鍈"),		_T(u"閒"),		_T(u"雞"),		_T(u"餃"),		_T(u"饀"),		_T(u"髙"),		_T(u"鯖"),			// 0x7641 - 0x7648
            _T(u"鷗"),		_T(u"麴"),		_T(u"麵"),																							// 0x7649 - 0x764B
        };

        // シンボルを変換する
        LPCWSTR pszSrc;
        if ((wCode >= 0x7521) && (wCode <= 0x757E)) {
            pszSrc = aszKanjiTable1[wCode - 0x7521];
        } else if ((wCode >= 0x7621) && (wCode <= 0x764B)) {
            pszSrc = aszKanjiTable2[wCode - 0x7621];
        } else if ((wCode >= 0x7A50U) && (wCode <= 0x7A74U)) {
            pszSrc = aszSymbolsTable1[wCode - 0x7A50U];
        } else if ((wCode >= 0x7C21U) && (wCode <= 0x7C7BU)) {
            pszSrc = aszSymbolsTable2[wCode - 0x7C21U];
        } else if ((wCode >= 0x7D21U) && (wCode <= 0x7D7BU)) {
            pszSrc = aszSymbolsTable3[wCode - 0x7D21U];
        } else if ((wCode >= 0x7E21U) && (wCode <= 0x7E7DU)) {
            pszSrc = aszSymbolsTable4[wCode - 0x7E21U];
        } else {
            pszSrc = _T(u"□");
        }
        DWORD Length = std::char_traits<char16_t>::length(pszSrc);
        if (dwDstLen < Length)
            return -1;
        memcpy(lpszDst, pszSrc, Length * sizeof(TCHAR));

        return Length;
    }

    inline const int PutMacroChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        static const BYTE Macro[16][20] = {
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x4A, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x31, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x34, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x33, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x20, 0x41, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x20, 0x43, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x20, 0x44, 0x1B, 0x29, 0x20, 0x45, 0x1B, 0x2A, 0x20, 0x46, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x20, 0x47, 0x1B, 0x29, 0x20, 0x48, 0x1B, 0x2A, 0x20, 0x49, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x20, 0x4A, 0x1B, 0x29, 0x20, 0x4B, 0x1B, 0x2A, 0x20, 0x4C, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x20, 0x4D, 0x1B, 0x29, 0x20, 0x4E, 0x1B, 0x2A, 0x20, 0x4F, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x43, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x44, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x31, 0x1B, 0x29, 0x30, 0x1B, 0x2A, 0x4A, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
            { 0x1B, 0x28, 0x4A, 0x1B, 0x29, 0x32, 0x1B, 0x2A, 0x20, 0x41, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
        };

        if ((wCode & 0xF0) == 0x60)
            return ProcessString(lpszDst, dwDstLen, Macro[wCode & 0x0F], strlen((LPCSTR)Macro[wCode & 0x0F]));
        return 0;
    }

    inline const int PutDRCSChar(LPWSTR lpszDst, const DWORD dwDstLen, const WORD wCode) {
        if (m_pDRCSMap) {
            LPCWSTR pszSrc = m_pDRCSMap->GetString(wCode);
            if (pszSrc) {
                int Length = std::char_traits<char16_t>::length(pszSrc);
                if (dwDstLen < (DWORD)Length)
                    return -1;
                memcpy(lpszDst, pszSrc, Length * sizeof(TCHAR));
                return Length;
            }
        }
#ifdef _UNICODE
        if (dwDstLen < 1)
            return -1;
        lpszDst[0] = L'□';
        return 1;
#else
        if (dwDstLen < 2)
            return -1;
        lpszDst[0] = "□"[0];
        lpszDst[1] = "□"[1];
        return 2;
#endif
    }


    inline void ProcessEscapeSeq(const BYTE byCode) {
        // エスケープシーケンス処理
        switch (m_byEscSeqCount) {
            // 1バイト目
        case 1U:
            switch (byCode) {
                // Invocation of code elements
            case 0x6EU: LockingShiftGL(2U);	m_byEscSeqCount = 0U;	return;		// LS2
            case 0x6FU: LockingShiftGL(3U);	m_byEscSeqCount = 0U;	return;		// LS3
            case 0x7EU: LockingShiftGR(1U);	m_byEscSeqCount = 0U;	return;		// LS1R
            case 0x7DU: LockingShiftGR(2U);	m_byEscSeqCount = 0U;	return;		// LS2R
            case 0x7CU: LockingShiftGR(3U);	m_byEscSeqCount = 0U;	return;		// LS3R

                // Designation of graphic sets
            case 0x24U:
            case 0x28U: m_byEscSeqIndex = 0U;		break;
            case 0x29U: m_byEscSeqIndex = 1U;		break;
            case 0x2AU: m_byEscSeqIndex = 2U;		break;
            case 0x2BU: m_byEscSeqIndex = 3U;		break;
            default: m_byEscSeqCount = 0U;		return;		// エラー
            }
            break;

            // 2バイト目
        case 2U:
            if (DesignationGSET(m_byEscSeqIndex, byCode)) {
                m_byEscSeqCount = 0U;
                return;
            }

            switch (byCode) {
            case 0x20: m_bIsEscSeqDrcs = true;	break;
            case 0x28: m_bIsEscSeqDrcs = true;	m_byEscSeqIndex = 0U;	break;
            case 0x29: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 1U;	break;
            case 0x2A: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 2U;	break;
            case 0x2B: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 3U;	break;
            default: m_byEscSeqCount = 0U;		return;		// エラー
            }
            break;

            // 3バイト目
        case 3U:
            if (!m_bIsEscSeqDrcs) {
                if (DesignationGSET(m_byEscSeqIndex, byCode)) {
                    m_byEscSeqCount = 0U;
                    return;
                }
            } else {
                if (DesignationDRCS(m_byEscSeqIndex, byCode)) {
                    m_byEscSeqCount = 0U;
                    return;
                }
            }

            if (byCode == 0x20U) {
                m_bIsEscSeqDrcs = true;
            } else {
                // エラー
                m_byEscSeqCount = 0U;
                return;
            }
            break;

            // 4バイト目
        case 4U:
            DesignationDRCS(m_byEscSeqIndex, byCode);
            m_byEscSeqCount = 0U;
            return;
        }

        m_byEscSeqCount++;
    }


    inline void LockingShiftGL(const BYTE byIndexG) {
        // LSx
        m_pLockingGL = &m_CodeG[byIndexG];
    }

    inline void LockingShiftGR(const BYTE byIndexG) {
        // LSxR
        m_pLockingGR = &m_CodeG[byIndexG];
    }

    inline void SingleShiftGL(const BYTE byIndexG) {
        // SSx
        m_pSingleGL = &m_CodeG[byIndexG];
    }

    inline const bool DesignationGSET(const BYTE byIndexG, const BYTE byCode) {
        // Gのグラフィックセットを割り当てる
        switch (byCode) {
        case 0x42U: m_CodeG[byIndexG] = CODE_KANJI;				return true;	// Kanji
        case 0x4AU: m_CodeG[byIndexG] = CODE_ALPHANUMERIC;		return true;	// Alphanumeric
        case 0x30U: m_CodeG[byIndexG] = CODE_HIRAGANA;			return true;	// Hiragana
        case 0x31U: m_CodeG[byIndexG] = CODE_KATAKANA;			return true;	// Katakana
        case 0x32U: m_CodeG[byIndexG] = CODE_MOSAIC_A;			return true;	// Mosaic A
        case 0x33U: m_CodeG[byIndexG] = CODE_MOSAIC_B;			return true;	// Mosaic B
        case 0x34U: m_CodeG[byIndexG] = CODE_MOSAIC_C;			return true;	// Mosaic C
        case 0x35U: m_CodeG[byIndexG] = CODE_MOSAIC_D;			return true;	// Mosaic D
        case 0x36U: m_CodeG[byIndexG] = CODE_PROP_ALPHANUMERIC;	return true;	// Proportional Alphanumeric
        case 0x37U: m_CodeG[byIndexG] = CODE_PROP_HIRAGANA;		return true;	// Proportional Hiragana
        case 0x38U: m_CodeG[byIndexG] = CODE_PROP_KATAKANA;		return true;	// Proportional Katakana
        case 0x49U: m_CodeG[byIndexG] = CODE_JIS_X0201_KATAKANA;	return true;	// JIS X 0201 Katakana
        case 0x39U: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_1;	return true;	// JIS compatible Kanji Plane 1
        case 0x3AU: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_2;	return true;	// JIS compatible Kanji Plane 2
        case 0x3BU: m_CodeG[byIndexG] = CODE_ADDITIONAL_SYMBOLS;	return true;	// Additional symbols
        default: return false;		// 不明なグラフィックセット
        }
    }

    inline const bool DesignationDRCS(const BYTE byIndexG, const BYTE byCode) {
        // DRCSのグラフィックセットを割り当てる
        if (byCode >= 0x40 && byCode <= 0x4F) {		// DRCS
            m_CodeG[byIndexG] = (CODE_SET)(CODE_DRCS_0 + (byCode - 0x40));
        } else if (byCode == 0x70) {				// Macro
            m_CodeG[byIndexG] = CODE_MACRO;
        } else {
            return false;
        }
        return true;
    }

    inline const bool IsSmallCharMode() const {
        return m_CharSize == SIZE_SMALL || m_CharSize == SIZE_MEDIUM || m_CharSize == SIZE_MICRO;
    }

    bool SetFormat(DWORD Pos) {
        if (!m_pFormatList)
            return false;

        FormatInfo Format;
        Format.Pos = Pos;
        Format.Size = m_CharSize;
        Format.CharColorIndex = m_CharColorIndex;
        Format.BackColorIndex = m_BackColorIndex;
        Format.RasterColorIndex = m_RasterColorIndex;

        if (!m_pFormatList->empty()) {
            if (m_pFormatList->back().Pos == Pos) {
                m_pFormatList->back() = Format;
                return true;
            }
        }
        m_pFormatList->push_back(Format);
        return true;
    }
};

#ifdef __HACK_UNICODE__
#undef __HACK_UNICODE__
#undef _UNICODE
#endif

} // namespace aribstring

static std::u16string GetAribString(MemoryChunk mc) {
    int bufsize = (int)mc.length + 1;
    auto buf = std::unique_ptr<char16_t[]>(new char16_t[bufsize]);
    int dstLen = aribstring::CAribString::AribToString(buf.get(), bufsize, mc.data, (int)mc.length);
    return std::u16string(buf.get(), buf.get() + dstLen);
}
