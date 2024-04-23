#include <iconv.h>

#include "windows.h"
#include "CaptionDef.h"
#include "ARIB8CharDecode.h"
#include "CaptionMain.h"
#include "Caption.h"
#include <string.h>

typedef enum{
	MODE_ACP, MODE_UTF16, MODE_UTF8
} CP_MODE;

#define SWITCH_STREAM_MAX 8

static DWORD g_dwIndex;
static CCaptionMain* g_sysList[SWITCH_STREAM_MAX];
static char* g_pPoolList[SWITCH_STREAM_MAX];
static DWORD g_dwPoolSizeList[SWITCH_STREAM_MAX];
static CP_MODE g_charModeList[SWITCH_STREAM_MAX];

static CCaptionMain* g_sys;
static char* g_pPool;
static DWORD g_dwPoolSize;
static CP_MODE g_charMode;

CCaptionMain::CCaptionMain(void)
	: m_ucDgiGroup(0xFF)
	, m_iLastCounter(-1)
	, m_bAnalyz(TRUE)
	, m_dwNowReadSize(0)
	, m_dwNeedSize(0)
{
	for( size_t i=0; i<LANG_TAG_MAX; i++ ){
		//ucLangTag==0xFFは当該タグが存在しないことを示す
		m_LangTagList[i].ucLangTag = 0xFF;
	}
}


DWORD WINAPI GetCaptionDataCP(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL || (g_charMode != MODE_ACP && g_charMode != MODE_UTF8) ){
		return CP_ERR_NOT_INIT;
	}
	DWORD dwRet = g_sys->GetCaptionData(ucLangTag, ppList, pdwListCount);

	if( dwRet != FALSE ){
		//変換後の文字列を格納できるだけの領域を確保
		DWORD dwNeedSize = 0;
		for( size_t i=0; i<*pdwListCount; i++ ){
			for( size_t j=0; j<(*ppList)[i].dwListCount; j++ ){
				dwNeedSize += std::char_traits<char16_t>::length( static_cast<LPCWSTR>((*ppList)[i].pstCharList[j].pszDecode) ) + 1;
			}
		}
		if( g_charMode == MODE_ACP ){
			if( g_dwPoolSize < dwNeedSize*2+1 ){
				delete[] g_pPool;
				g_dwPoolSize = dwNeedSize*2+1;
				g_pPool = new char[g_dwPoolSize];
			}
		}else{
			if( g_dwPoolSize < dwNeedSize*4+1 ){
				delete[] g_pPool;
				g_dwPoolSize = dwNeedSize*4+1;
				g_pPool = new char[g_dwPoolSize];
			}
		}

		//必要なコードページに変換してバッファを入れ替える
		DWORD dwPoolCount = 0;
		for( DWORD i=0; i<*pdwListCount; i++ ){
			for( DWORD j=0; j<(*ppList)[i].dwListCount; j++ ){
				if( dwPoolCount >= g_dwPoolSize ){
					dwPoolCount = g_dwPoolSize - 1;
				}
				LPCWSTR pszSrc = static_cast<LPCWSTR>((*ppList)[i].pstCharList[j].pszDecode);
				char *pszDest = g_pPool + dwPoolCount;

				char* inbuf = (char*)pszSrc;
				size_t inbytes = strlen(inbuf);
				char* outbuf = (char*)pszDest;
				size_t outbytes = g_dwPoolSize - dwPoolCount;

				iconv_t ic;
				if ((ic = iconv_open(
						g_charMode==MODE_ACP ? "MS-ANSI" : "UTF-8", "UTF-16")
					) == (iconv_t) -1) {
					return -1;
				}
				if (iconv(ic, &inbuf, &inbytes, &outbuf, &outbytes) == (size_t) -1) {
					return -1;
				}
				iconv_close(ic);

				int nWritten = (g_dwPoolSize - dwPoolCount) - outbytes;
				if( nWritten > 0 ){
					dwPoolCount += nWritten;
				}else{
					pszDest[0] = '\0';
					dwPoolCount++;
				}
				(*ppList)[i].pstCharList[j].pszDecode = pszDest;
			}
		}
	}

	return dwRet;
}

DWORD WINAPI InitializeCPW()
{
	if( g_sys != NULL ){
		return CP_ERR_INIT;
	}
	g_sys = new CCaptionMain;
	g_charMode = MODE_UTF16;
	return TRUE;
}


DWORD WINAPI GetCaptionDataCPW(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL || g_charMode != MODE_UTF16 ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->GetCaptionData(ucLangTag, ppList, pdwListCount);
}

DWORD CCaptionMain::AddPESPacket(LPCBYTE pbBuff, DWORD dwSize)
{
	return ParseCaption(pbBuff, dwSize);
}

DWORD CCaptionMain::ParseCaption(LPCBYTE pbBuff, DWORD dwSize)
{
	if( pbBuff == NULL || dwSize < 3 ){
		return ERR_INVALID_PACKET;
	}
	//字幕か文字スーパーであるか
	if( pbBuff[0] != 0x80 && pbBuff[0] != 0x81 ){
		return ERR_INVALID_PACKET;
	}
	if( pbBuff[1] != 0xFF ){
		return ERR_INVALID_PACKET;
	}
	unsigned char ucHeadSize = pbBuff[2]&0x0F;
	
	DWORD dwStartPos = 3+ucHeadSize;

	if( dwSize < dwStartPos + 5 ){
		return ERR_INVALID_PACKET;
	}
	unsigned char ucDataGroupID = pbBuff[dwStartPos]>>2;
	unsigned char ucVersion = pbBuff[dwStartPos]&0x03; //運用されない
	dwStartPos+=3;
	unsigned short usDataGroupSize = ((unsigned short)(pbBuff[dwStartPos]))<<8 | pbBuff[dwStartPos+1];
	dwStartPos+=2;
	if( dwSize < dwStartPos + usDataGroupSize ){
		return ERR_INVALID_PACKET;
	}
	unsigned char ucDgiGroup = ucDataGroupID&0xF0;
	unsigned char ucID = ucDataGroupID&0x0F;
	
	DWORD dwRet = TRUE;
	if( ucDgiGroup == m_ucDgiGroup && ucID == 0 ){
		//組が前回とおなじ字幕管理は再送とみなす
		dwRet = CP_NO_ERR_TAG_INFO;
	}else if( ucDgiGroup != m_ucDgiGroup && 1 <= ucID && ucID <= LANG_TAG_MAX ){
		//組が字幕管理と異なる字幕は処理しない
		dwRet = TRUE;
	}else if( 1 <= ucID && ucID <= LANG_TAG_MAX && m_LangTagList[ucID-1].ucLangTag != ucID-1 ){
		//リストにない字幕も処理しない
		dwRet = TRUE;
	}else if( ucID == 0 ){
		//字幕管理
		m_ucDgiGroup = ucDgiGroup;
		for( size_t i=0; i<LANG_TAG_MAX; i++ ){
			//字幕データもクリアする
			m_LangTagList[i].ucLangTag = 0xFF;
		}
		m_CaptionList[0].clear();
		m_DRCList[0].clear();
		m_DRCMap[0].Clear();
		//usDataGroupSizeはCRC_16の2バイト分を含まないので-2すべきでない
		dwRet = ParseCaptionManagementData(pbBuff+dwStartPos,usDataGroupSize/*-2*/, &m_CaptionList[0], &m_DRCList[0], &m_DRCMap[0]);
		if( dwRet == TRUE ){
			dwRet = CP_CHANGE_VERSION;
		}
	}else if( 1 <= ucID && ucID <= LANG_TAG_MAX ){
		//字幕データ
		m_CaptionList[ucID] = m_CaptionList[0];
		m_DRCList[ucID] = m_DRCList[0];
		m_DRCMap[ucID] = m_DRCMap[0];
		//未定義の表示書式(0b1111)はCプロファイル(14)とみなす
		dwRet = ParseCaptionData(pbBuff+dwStartPos,usDataGroupSize/*-2*/, &m_CaptionList[ucID],
		                         &m_DRCList[ucID], &m_DRCMap[ucID], m_LangTagList[ucID-1].ucFormat-1);
		if( dwRet == TRUE ){
			dwRet = CP_NO_ERR_CAPTION_1 + ucID - 1;
		}else{
			//::OutputDebugString(TEXT(__FUNCTION__) TEXT("(): Unsupported Caption Data!\n"));
			m_CaptionList[ucID].clear();
			m_DRCList[ucID].clear();
			m_DRCMap[ucID].Clear();
			dwRet = TRUE;
		}
	}else{
		//未サポート
		dwRet = TRUE;
	}
	return dwRet;
}

DWORD CCaptionMain::ParseCaptionData(LPCBYTE pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList,
                                     vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode)
{
	if( pbBuff == NULL || dwSize < 4 ){
		return ERR_INVALID_PACKET;
	}
	
	DWORD dwRet = TRUE;
	
	DWORD dwPos = 0;
	unsigned char ucTMD = pbBuff[dwPos]>>6;
	dwPos++;

	unsigned char ucOTMHH = 0;
	unsigned char ucOTMMM = 0;
	unsigned char ucOTMSS = 0;
	unsigned char ucOTMSSS = 0;
	
	if( ucTMD == 0x01 || ucTMD == 0x02 ){
		if( dwSize < 5+4 ){
			return ERR_INVALID_PACKET;
		}
		//STM
		ucOTMHH = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMMM = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMSS = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMSSS = (pbBuff[dwPos]&0xF0>>4)*100 + (pbBuff[dwPos]&0x0F)*10 + (pbBuff[dwPos+1]&0xF0>>4);
		dwPos+=2;
	}
	UINT uiUnitSize = ((UINT)(pbBuff[dwPos]))<<16 | ((UINT)(pbBuff[dwPos+1]))<<8 | pbBuff[dwPos+2];
	dwPos+=3;
	if( dwSize >= dwPos+uiUnitSize ){
		//字幕データ
		DWORD dwReadSize = 0;
		while( dwReadSize<uiUnitSize && dwRet==TRUE ){
			DWORD dwSize = 0;
			dwRet = ParseUnitData(pbBuff+dwPos+dwReadSize, uiUnitSize-dwReadSize,&dwSize, pCaptionList, pDRCList, pDRCMap, wSWFMode);
			dwReadSize+=dwSize;
		}

	}
	return dwRet;
}

DWORD CCaptionMain::ParseCaptionManagementData(LPCBYTE pbBuff, DWORD dwSize, vector<CAPTION_DATA>* pCaptionList,
                                               vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap)
{
	if( pbBuff == NULL || dwSize < 2 ){
		return ERR_INVALID_PACKET;
	}
	DWORD dwRet = TRUE;
	
	DWORD dwPos = 0;
	unsigned char ucTMD = pbBuff[dwPos]>>6;
	dwPos++;
	unsigned char ucOTMHH = 0;
	unsigned char ucOTMMM = 0;
	unsigned char ucOTMSS = 0;
	unsigned char ucOTMSSS = 0;
	if( ucTMD == 0x02 ){
		if( dwSize < 5+2 ){
			return ERR_INVALID_PACKET;
		}
		//OTM
		ucOTMHH = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMMM = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMSS = (pbBuff[dwPos]&0xF0>>4)*10 + (pbBuff[dwPos]&0x0F);
		dwPos++;
		ucOTMSSS = (pbBuff[dwPos]&0xF0>>4)*100 + (pbBuff[dwPos]&0x0F)*10 + (pbBuff[dwPos+1]&0xF0>>4);
		dwPos+=2;
	}
	unsigned char ucLangNum = pbBuff[dwPos];
	dwPos++;
	
	for( unsigned char i=0; i<ucLangNum; i++ ){
		if( dwSize < dwPos+6 ){
			return ERR_INVALID_PACKET;
		}
		LANG_TAG_INFO_DLL Item;
		Item.ucLangTag = pbBuff[dwPos]>>5;
		Item.ucDMF = pbBuff[dwPos]&0x0F;
		dwPos++;
		if( Item.ucDMF == 0x0C || Item.ucDMF == 0x0D || Item.ucDMF == 0x0E ){
			Item.ucDC = pbBuff[dwPos];
			dwPos++;
		}
		Item.szISOLangCode[0] = pbBuff[dwPos];
		Item.szISOLangCode[1] = pbBuff[dwPos+1];
		Item.szISOLangCode[2] = pbBuff[dwPos+2];
		Item.szISOLangCode[3] = 0x00;
		dwPos+=3;
		Item.ucFormat = pbBuff[dwPos]>>4;
		Item.ucTCS = (pbBuff[dwPos]&0x0C)>>2;
		Item.ucRollupMode = pbBuff[dwPos]&0x03;
		dwPos++;
		if( Item.ucLangTag < LANG_TAG_MAX ){
			m_LangTagList[Item.ucLangTag] = Item;
		}
	}
	if( dwSize < dwPos+3 ){
		return ERR_INVALID_PACKET;
	}
	UINT uiUnitSize = ((UINT)(pbBuff[dwPos]))<<16 | ((UINT)(pbBuff[dwPos+1]))<<8 | pbBuff[dwPos+2];
	dwPos+=3;
	if( dwSize >= dwPos+uiUnitSize ){
		//字幕データ
		DWORD dwReadSize = 0;
		while( dwReadSize<uiUnitSize && dwRet==TRUE ){
			DWORD dwSize = 0;
			dwRet = ParseUnitData(pbBuff+dwPos+dwReadSize, uiUnitSize-dwReadSize,&dwSize, pCaptionList, pDRCList, pDRCMap, 9);
			dwReadSize+=dwSize;
		}
	}
	return dwRet;
}

DWORD WINAPI GetDRCSPatternCP(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount)
{
	if( g_sys == NULL ){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->GetDRCSPattern(ucLangTag, ppList, pdwListCount);
}

BOOL CCaptionMain::GetDRCSPattern(unsigned char ucLangTag, DRCS_PATTERN_DLL** ppList, DWORD* pdwListCount)
{
	if( ppList == NULL || pdwListCount == NULL ){
		return FALSE;
	}

	if( ucLangTag < LANG_TAG_MAX &&
		m_LangTagList[ucLangTag].ucLangTag == ucLangTag &&
		m_DRCList[ucLangTag+1].size() > 0 )
	{
		const vector<DRCS_PATTERN> &List = m_DRCList[ucLangTag+1];

		//まずバッファを作る
		m_pDRCList.resize(List.size());

		DWORD dwCount = 0;
		vector<DRCS_PATTERN>::const_iterator it = List.begin();
		for( ; it != List.end(); ++it ){
			//UCSにマップされていない=字幕文に一度も現れていない
			wchar_t wc = m_DRCMap[ucLangTag+1].GetUCS(it->wDRCCode);
			if( wc != L'\0' ){
				m_pDRCList[dwCount].dwDRCCode = it->wDRCCode;
				m_pDRCList[dwCount].dwUCS = wc;
				m_pDRCList[dwCount].wGradation = it->wGradation;
				m_pDRCList[dwCount].wReserved = 0;
				m_pDRCList[dwCount].dwReserved = 0;
				m_pDRCList[dwCount].bmiHeader = it->bmiHeader;
				m_pDRCList[dwCount].pbBitmap = it->bBitmap;
				dwCount++;
			}
		}
		*pdwListCount = dwCount;
		*ppList = &m_pDRCList[0];
		return TRUE;
	}
	return FALSE;
}

DWORD WINAPI AddPESPacketCP(BYTE* pbBuff, DWORD dwSize)
{
	if (g_sys == NULL){
		return CP_ERR_NOT_INIT;
	}
	return g_sys->AddPESPacket(pbBuff, dwSize);
}

DWORD CCaptionMain::ParseUnitData(LPCBYTE pbBuff, DWORD dwSize, DWORD* pdwReadSize, vector<CAPTION_DATA>* pCaptionList,
                                  vector<DRCS_PATTERN>* pDRCList, CDRCMap* pDRCMap, WORD wSWFMode)
{
	if( pbBuff == NULL || dwSize < 5 || pdwReadSize == NULL ){
		return FALSE;
	}
	if( pbBuff[0] != 0x1F ){
		return FALSE;
	}

	// odaru modified.
	UINT uiUnitSize = ((UINT)(pbBuff[2]))<<16 | ((UINT)(pbBuff[3]))<<8 | pbBuff[4];

	if( dwSize < 5+uiUnitSize ){
		return FALSE;
	}
	if( pbBuff[1] != 0x20 ){
		//字幕文(本文)以外
		if( pbBuff[1] == 0x30 || pbBuff[1] == 0x31 ){
			//DRCS処理
			if( uiUnitSize > 0 ){
				if( CARIB8CharDecode::DRCSHeaderparse(pbBuff+5, uiUnitSize, pDRCList, pbBuff[1]==0x31?TRUE:FALSE ) == FALSE ){
					//::OutputDebugString(TEXT(__FUNCTION__) TEXT("(): Unsupported DRCS!\n"));
					//return FALSE;
				}
			}
		}
		// odaru modified.
		//*pdwReadSize = dwSize;
		*pdwReadSize = uiUnitSize + 5;
		return TRUE;
	}
	
	
	if( uiUnitSize > 0 ){
		if( m_cDec.Caption(pbBuff+5, uiUnitSize, pCaptionList, pDRCMap, wSWFMode) == FALSE ){
			return FALSE;
		}
	}
	*pdwReadSize = 5+uiUnitSize;

	return TRUE;
}

DWORD CCaptionMain::GetCaptionData(unsigned char ucLangTag, CAPTION_DATA_DLL** ppList, DWORD* pdwListCount)
{
	if( ppList == NULL || pdwListCount == NULL ){
		return FALSE;
	}

	if( ucLangTag < LANG_TAG_MAX &&
		m_LangTagList[ucLangTag].ucLangTag == ucLangTag &&
		m_CaptionList[ucLangTag+1].size() > 0 )
	{
		const vector<CAPTION_DATA> &List = m_CaptionList[ucLangTag+1];

		//まずバッファを作る
		DWORD dwCapCharPoolNeed = 0;
		for( size_t i=0; i<List.size(); i++ ){
			dwCapCharPoolNeed += (DWORD)List[i].CharList.size();
		}
		m_pCapList.resize(List.size());
		m_pCapCharPool.resize(dwCapCharPoolNeed == 0 ? 1 : dwCapCharPoolNeed);

		DWORD dwCapCharPoolCount = 0;
		vector<CAPTION_DATA>::const_iterator it = List.begin();
		CAPTION_DATA_DLL *pCap = &m_pCapList[0];
		for( ; it != List.end(); ++it,++pCap ){
			pCap->dwListCount = (DWORD)it->CharList.size();
			pCap->pstCharList = &m_pCapCharPool[0] + dwCapCharPoolCount;
			dwCapCharPoolCount += (DWORD)it->CharList.size();

			vector<CAPTION_CHAR_DATA>::const_iterator jt = it->CharList.begin();
			CAPTION_CHAR_DATA_DLL *pCapChar = pCap->pstCharList;
			for( ; jt != it->CharList.end(); ++jt,++pCapChar ){
				pCapChar->pszDecode = jt->strDecode.c_str();
				pCapChar->wCharSizeMode = (DWORD)jt->emCharSizeMode;
				pCapChar->stCharColor = jt->stCharColor;
				pCapChar->stBackColor = jt->stBackColor;
				pCapChar->stRasterColor = jt->stRasterColor;
				pCapChar->bUnderLine = jt->bUnderLine;
				pCapChar->bShadow = jt->bShadow;
				pCapChar->bBold = jt->bBold;
				pCapChar->bItalic = jt->bItalic;
				pCapChar->bFlushMode = jt->bFlushMode;
				pCapChar->wCharW = jt->wCharW;
				pCapChar->wCharH = jt->wCharH;
				pCapChar->wCharHInterval = jt->wCharHInterval;
				pCapChar->wCharVInterval = jt->wCharVInterval;
				pCapChar->bHLC = jt->bHLC;
				pCapChar->bPRA = jt->bPRA;
				pCapChar->bAlignment = 0;
			}

			pCap->bClear = it->bClear;
			pCap->wSWFMode = it->wSWFMode;
			pCap->wClientX = it->wClientX;
			pCap->wClientY = it->wClientY;
			pCap->wClientW = it->wClientW;
			pCap->wClientH = it->wClientH;
			pCap->wPosX = it->wPosX;
			pCap->wPosY = it->wPosY;
			pCap->dwWaitTime = it->dwWaitTime;
			pCap->wAlignment = 0;
		}
		*pdwListCount = (DWORD)List.size();
		*ppList = &m_pCapList[0];
		return TRUE;
	}
	return FALSE;
}
