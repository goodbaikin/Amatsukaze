/*====================================================================
*	ロゴパターン			logo.h
* 
* [ロゴデータファイル構造]
* 
* 	"<logo file x.xx>"	// ファイルヘッダ文字列：バージョン情報(28byte)
* 	+----
* 	|	ファイルに含まれるロゴデータの数(4byte, BigEndian)
* 	+----
* 	|	LOGO_HEADER		// データヘッダ
* 	+----
* 	|
* 	:	LOGO_PIXEL[h*w]	// ピクセル情報：サイズはLOGO_HEADERのw,hから算出
* 	:
* 	+----
* 	|	LOGO_HEADER
* 	+----
* 	|
* 	:	LOGO_PIXEL[h*w]
* 	:
* 
*===================================================================*/
#ifndef ___LOGO_H
#define ___LOGO_H

#ifndef _WIN32
#include "linux/windows.h"
#endif
#include <stdint.h>

/* ロゴヘッダ文字列 */
#define LOGO_FILE_HEADER_STR "<logo data file ver0.1>\0\0\0\0\0"
#define LOGO_FILE_HEADER_STR_SIZE  28

/*--------------------------------------------------------------------
*	LOGO_FILE_HEADER 構造体
*		ファイルヘッダ．
*		バージョン情報と含まれるデータ数
*-------------------------------------------------------------------*/
typedef struct {
	char str[LOGO_FILE_HEADER_STR_SIZE];
	union{
		uint32_t l;
		unsigned char c[4];
	} logonum;
} LOGO_FILE_HEADER;

#define SWAP_ENDIAN(x) (((x&0xff)<<24)|((x&0xff00)<<8)|((x&0xff0000)>>8)|((x&0xff000000)>>24))

/* 不透明度最大値 */
#define LOGO_MAX_DP   1000

/* ロゴ名最大文字数（終端\0含む） */
#define LOGO_MAX_NAME 32
#define LOGO_MAX_NAME_V02 256

/*--------------------------------------------------------------------
*	LOGO_HEADER 構造体
*		ロゴの基本的な情報を記録
*-------------------------------------------------------------------*/
typedef struct {
	char     name[LOGO_MAX_NAME]; 	/* 名称                   */
	short    x, y;      			/* 基本位置               */
	short    h, w;      			/* ロゴ高さ・幅           */
	short    fi, fo;    			/* デフォルトのFadeIn/Out */
	short    st, ed;    			/* デフォルトの開始・終了  */
} LOGO_HEADER;

typedef struct {
	char     name[LOGO_MAX_NAME_V02]; 	/* 名称                   */
	short    x, y;      				/* 基本位置               */
	short    h, w;      				/* ロゴ高さ・幅           */
	short    fi, fo;    				/* デフォルトのFadeIn/Out */
	short    st, ed;    				/* デフォルトの開始・終了  */
	char     reserved[240];
} LOGO_HEADER_V02;

/*--------------------------------------------------------------------
*	LOGO_PIXEL 構造体
*		ロゴの各ピクセルごとの情報を記録
*-------------------------------------------------------------------*/
typedef struct {
	short dp_y;		/* 不透明度（輝度）            */
	short y;		/* 輝度              0〜4096   */
	short dp_cb;	/* 不透明度（青）              */
	short cb;		/* 色差（青）    -2048〜2048   */
	short dp_cr;	/* 不透明度（赤）              */
	short cr;		/* 色差（赤）    -2048〜2048   */
} LOGO_PIXEL;

/*--------------------------------------------------------------------
*	ロゴデータのサイズ（ヘッダ無し）
*-------------------------------------------------------------------*/
#define LOGO_PIXELSIZE(ptr)  \
	(((LOGO_HEADER *)ptr)->h*((LOGO_HEADER *)ptr)->w*sizeof(LOGO_PIXEL))

/*--------------------------------------------------------------------
*	ロゴデータ全体のサイズ
*-------------------------------------------------------------------*/
#define LOGO_DATASIZE(ptr) (sizeof(LOGO_HEADER)+LOGO_PIXELSIZE(ptr))

/* ロゴ表示判定用パラメータ初期値 */
#define DEF_LOGO_RATE_TH_LOGO   50		/* ロゴ表示と判断する結果閾値(0-100) */
#define DEF_LOGO_NUM_FADEIN     0		/* フェードインフレーム数 */
#define DEF_LOGO_NUM_FADEOUT    0		/* フェードアウトフレーム数 */
#define DEF_LOGO_NUM_CUTLEFT    500		/* 開始直後のロゴ検出しないフレーム数 */
#define DEF_LOGO_NUM_CUTRIGHT   150		/* 終了直前のロゴ検出しないフレーム数*/
#define DEF_LOGO_NUM_ONWIDTH    25		/* ロゴ表示ON/OFF切り替え最低フレーム数 */
#define DEF_LOGO_NUM_ONLEVEL    21		/* ロゴ表示ON/OFF切り替えunclearレベル */
#define DEF_LOGO_NUM_OFFWIDTH   0		/* ロゴ表示OFF切り替え最低フレーム数(0の時ON値使用) */
#define DEF_LOGO_THRES_YMAX     235		/* 最大輝度値 */
#define DEF_LOGO_THRES_YMIN     16		/* 最小輝度値 */
#define DEF_LOGO_THRES_YEDGE    255		/* エッジ検出を行う最大輝度値 */
#define DEF_LOGO_THRES_YDIF     40		/* エッジ検出を行う最低輝度差x16 */
#define DEF_LOGO_THRES_YSETDIF  222		/* エッジ検出を行う輝度値でydif値変更用 */
#define DEF_LOGO_THRES_YOFFEDG  255		/* エッジ検出でロゴなし判定をしない輝度値 */
#define DEF_LOGO_NUM_AREASET    0		/* エリア検出を行うエリアの設定 */
#define DEF_LOGO_NUM_CLRRATE    8		/* 不明確ロゴを無効と判断するフレーム割合 */
#define DEF_LOGO_AUTO_FADE      8		/* 自動検出フェード */
#define DEF_LOGO_AUTO_YEDGE     220		/* BS11検出時のエッジ検出を行う最大輝度値 */
#define DEF_LOGO_AUTO_YDIFMIN   24      /* ydifを自動調整する場合の最低値 */

/* 設定する閾値格納 */
typedef struct {
	short	rate_th_logo;			/* ロゴ表示と判断する結果閾値(0-100) */
	short	num_fadein;				/* フェードインフレーム数 */
	short	num_fadeout;			/* フェードアウトフレーム数 */
	short	num_cutleft;			/* 開始直後のロゴ検出しないフレーム数 */
	short	num_cutright;			/* 終了直前のロゴ検出しないフレーム数*/
	short	num_onwidth;			/* ロゴ表示ON/OFF切り替え最低フレーム数 */
	short	num_onlevel;			/* ロゴ表示ON/OFF切り替えunclearレベル */
	short	num_offwidth;			/* ロゴ表示OFF切り替え最低フレーム数 */
	short	thres_ymax;				/* 最大輝度値 */
	short	thres_ymin;				/* 最小輝度値 */
	short	thres_yedge;			/* エッジ検出を行う最大輝度値 */
	short	thres_ydif;				/* エッジ検出を行う最低輝度差x16 */
	short	thres_ysetdif;			/* エッジ検出を行う輝度値でydif値変更用 */
	short	thres_yoffedg;			/* エッジ検出でロゴなし判定をしない輝度値 */
	short	num_areaset;			/* エリア検出を行うエリアの設定 */
	short	num_clrrate;			/* 不明確ロゴを無効と判断するフレーム割合 */

	short	auto_fade;				/* 自動検出フェード */
	short	auto_bs11;				/* BS11自動検出（thres_yedge自動設定） */
} LOGO_THRESREC;



/* ロゴデータ */
typedef struct {
	LOGO_FILE_HEADER lfh;
	LOGO_HEADER      lgh;
	LOGO_PIXEL       *ptr;
} LOGO_READREC;


#define LOGO_AREANUM 7		/* エリア検出のエリア最大数 */

/* エリア検出内部演算用 */
typedef struct {
	long     total_area;			/* エリア検出で使用する最低画素数 */
	long     total_area1;			/* 背景エリア取得箇所の総数 */
	long     total_area2;			/* ロゴエリア取得箇所の総数 */
	long     xmin;					/* エリア検出で使用するエリア座標 */
	long     xmax;					/* エリア検出で使用するエリア座標 */
	long     ymin;					/* エリア検出で使用するエリア座標 */
	long     ymax;					/* エリア検出で使用するエリア座標 */
} LOGO_PARAMAREAREC;

/* 内部演算用ロゴデータ */
typedef struct {
	short    yx, yy;      			/* 基本位置               */
	short    yh, yw;      			/* ロゴ高さ・幅           */
	short    *dp_y;					/* ロゴ透過度 */
	short    *y;					/* ロゴ輝度   */

	char     *dif_y_col;			/* エッジ検出（横） */
	char     *dif_y_row;			/* エッジ検出（縦） */
	short    *area_y;				/* ロゴエリア情報（0:中間 1:背景 2:ロゴ） */
	short    most_logo_y;			/* 最も多いロゴの輝度値 */
	short    thres_dp_y;			/* ロゴ透過度閾値 */

	/* ロゴ内容確認用 */
	long     total_dif;				/* エッジ検出箇所の総数 */
	long     total_dif_c1;			/* エッジ検出箇所の総数（インターレース分離用） */
	long     total_dif_c2;			/* エッジ検出箇所の総数（インターレース分離用） */

	long     num_local;				/* エリア検出ローカルエリア数 */
	short    scale_area2;			/* エリア検出時のロゴエリア輝度 */
	LOGO_PARAMAREAREC area[LOGO_AREANUM];		/* エリア検出で使用するエリア情報 */
} LOGO_PARAMREC;


/* フェード判断用ステップ数 */
#define LOGO_FADE_STEP 31			/* 最大フェード計算ステップ数（フェード値＋１） */
#define LOGO_FADE_OFST 2			/* ヒストグラム前後保持数 */
#define LOGO_FADE_MAXLEVEL LOGO_FADE_STEP+(LOGO_FADE_OFST*2)+1	/* フェードヒストグラム最大保持数 */

/* エリア検出用の１画像分のロゴ演算データ格納 */
typedef struct {
	long	hista_areaoff[256];		/* エリア検出用ロゴなし輝度総和 */
	long	hista_areaon[256];		/* エリア検出用ロゴあり輝度総和 */
	long	hista_areacal[256];		/* エリア検出用ロゴ予想輝度総和 */
	long	num_hista_off;			/* エリア検出用ロゴなし輝度回数 */
	long	num_hista_on;			/* エリア検出用ロゴあり輝度回数 */

	short	rate_areavalid;			/* エリア検出ロゴ有効領域割合 */
	short	rate_arealogo;			/* エリア検出ロゴ結果 */
	short	diff_arealogo;			/* エリア検出ロゴ有無の輝度差（全体） */
	short	diff1_arealogo;			/* エリア検出ロゴ有無の輝度差（ロゴなしエリア） */
	short	diff2_arealogo;			/* エリア検出ロゴ有無の輝度差（ロゴありエリア） */
	short	vari_arealogo_off;		/* エリア検出ロゴ有無の周辺分散（ロゴなしエリア） */
	short	vari_arealogo_cal;		/* エリア検出ロゴ有無の周辺分散（ロゴ計算エリア） */
	short	vari_arealogo_on;		/* エリア検出ロゴ有無の周辺分散（ロゴありエリア） */
} LOGO_CALCAREAREC;

/* １画像分のロゴ演算データ格納 */
typedef struct {
	long	hist_y[LOGO_FADE_MAXLEVEL];	/* フェード用輝度ヒストグラム */
	long	fade_calcstep;				/* フェード用輝度ヒストグラムの計算用ステップ数 */
	
	long	cnt_logooff;			/* ロゴなしと判断した回数 */
	long	cnt_logoon;				/* ロゴありと判断した回数 */
	long	cnt_logomv;				/* ロゴ検出不可と判断した回数 */
	long	cnt_logovc;				/* ロゴ検出有効カウント数 */

	long	cntf_logooff;			/* ロゴなしと判断した回数（小振幅） */
	long	cntf_logoon;			/* ロゴありと判断した回数（小振幅） */
	long	cntf_logost;			/* ロゴ振幅なし判断した回数（小振幅） */
	long	cntf_logovc;			/* ロゴ検出有効カウント数（小振幅） */
	long	cntopp_logoon;			/* ロゴありと判断した回数（画素反転時） */

	long	sum_areaoff;			/* 小振幅フェード用ロゴなし輝度総和 */
	long	sum_areaon;				/* 小振幅フェード用ロゴあり輝度総和 */
	long	sum_areadif;			/* 小振幅フェード用ロゴ理想輝度差の総和 */
	long	sum_areanum;			/* 小振幅フェード用ロゴ判断回数 */

	long	cnt_offedg;				/* 不明確なロゴなし判断回数 */
	long	cntf_offedg;			/* 不明確なロゴなし判断回数（小振幅） */
	long	cnts_logooff;			/* 微妙なロゴ影部分をロゴなしと判断した回数 */
	long	cnts_logoon;			/* 微妙なロゴ影部分をロゴありと判断した回数 */

	long	total_dif;				/* エッジ検出箇所の総数 */

	long	vc_cntd_logooff;		/* ロゴなしと判断した回数 */
	long	vc_cntd_logoon;			/* ロゴありと判断した回数 */
	long	vc_cntf_logooff;		/* ロゴなしと判断した回数（小振幅） */
	long	vc_cntf_logoon;			/* ロゴありと判断した回数（小振幅） */
	long	vc_cntd_voff;			/* ロゴなしと判断した有効行数 */
	long	vc_cntd_von;			/* ロゴありと判断した有効行数 */
	long	vc_cntf_voff;			/* ロゴなしと判断した有効行数（小振幅） */
	long	vc_cntf_von;			/* ロゴありと判断した有効行数（小振幅） */

	short	rate_logoon;			/* ロゴ検出結果(0-100) */
	short	rate_fade;				/* ロゴフェード状態検出結果(0-100) */
	short	flag_nosample;			/* ロゴサンプル不足検出不可(0-1) */
	short	flag_nofadesm;			/* ロゴフェードサンプル不足(0-1) */
	short	rank_unclear;			/* ロゴ検出不安定度(0-3,9) */

	short	rate_arealogo;			/* エリア検出ロゴ結果 */
	LOGO_CALCAREAREC area[LOGO_AREANUM];	/* 各エリア検出ロゴ演算情報 */

	short	rate_areaavg_t;			/* 小振幅フェード用ロゴ検出結果(0-100) */
	long	sum_areanum_t;			/* 小振幅フェード用ロゴ検出回数 */
} LOGO_CALCREC;


/* 有効データ数測定用 */
typedef struct {
	long	vc_cntd_logooff;		/* ロゴなしと判断した回数 */
	long	vc_cntd_logoon;			/* ロゴありと判断した回数 */
	long	vc_cntf_logooff;		/* ロゴなしと判断した回数（小振幅） */
	long	vc_cntf_logoon;			/* ロゴありと判断した回数（小振幅） */
	long	vc_cntd_voff;			/* ロゴなしと判断した有効行数 */
	long	vc_cntd_von;			/* ロゴありと判断した有効行数 */
	long	vc_cntf_voff;			/* ロゴなしと判断した有効行数（小振幅） */
	long	vc_cntf_von;			/* ロゴありと判断した有効行数（小振幅） */
	long	cntd_last_on;
	long	cntd_last_off;
	long	cntf_last_on;
	long	cntf_last_off;
} LOGO_VCCALCREC;


/* ロゴ表示期間の最大組数 */
#define LOGO_FIND_MAX 256

/* 最終データに必要な情報 */
typedef struct {
	long	frm_rise;				/* 各開始フレーム */
	long	frm_fall;				/* 各終了フレーム */
	long	frm_rise_l;				/* 各開始フレーム候補開始 */
	long	frm_rise_r;				/* 各開始フレーム候補終了 */
	long	frm_fall_l;				/* 各終了フレーム候補開始 */
	long	frm_fall_r;				/* 各終了フレーム候補終了 */
	char	fade_rise;				/* 各開始フェードイン状態(0 or fadein) */
	char	fade_fall;				/* 各終了フェードアウト状態(0 or fadeout) */
	char	intl_rise;				/* 各開始インターレース状態(0:ALL 1:TOP 2:BTM) */
	char	intl_fall;				/* 各終了インターレース状態(0:ALL 1:TOP 2:BTM) */
} LOGO_OUTSUBREC;

/* 全フレーム演算に必要なデータ格納 */
typedef struct {
	long	num_frames;				/* フレーム数 */
	char	*rate_logoon;			/* 各フレームロゴ検出結果(0-100) */
	char	*rate_fade;				/* 各フレームロゴフェード状態検出結果(0-100) */
	char	*flag_nosample;			/* 各フレームロゴサンプル不足検出不可(b0-1:0-1)  */
	char	*flag_nofadesm;			/* 各フレームロゴフェードサンプル不足検出不可(b0-1:0-1)  */
	char	*rate_logooni1;			/* 各フレームロゴ検出結果インターレース奇数(0-100) */
	char	*rate_logooni2;			/* 各フレームロゴ検出結果インターレース偶数(0-100) */
	char	*rate_fadei1;			/* 各フレームロゴフェード状態検出結果(0-100) */
	char	*rate_fadei2;			/* 各フレームロゴフェード状態検出結果(0-100) */
	char	*rank_unclear;			/* 各フレームロゴ検出不安定度(0-3,9)  */
	char	*flag_logoon;			/* 各フレームロゴ検出結果２進数表記(0-1) */

	long	frm_leftstart;			/* 有効開始フレーム */
	long	frm_rightend;			/* 有効終了フレーム */
	LOGO_OUTSUBREC workres[LOGO_FIND_MAX];	/* ロゴ表示期間結果の作業領域 */

	/* 最終出力 */
	long	num_find;					/* ロゴ表示期間の組数 */
	LOGO_OUTSUBREC res[LOGO_FIND_MAX];	/* ロゴ表示期間の結果 */
} LOGO_FRAMEREC;


/* データ一式 */
typedef struct {
	LOGO_READREC	readdat;
	LOGO_PARAMREC	paramdat;
	LOGO_CALCREC	calcdat1;
	LOGO_CALCREC	calcdat2;
	LOGO_FRAMEREC	framedat;
	LOGO_THRESREC	thresdat;		/* 閾値設定値 */
} LOGO_DATASET;


/* 最終結果 */
typedef struct {
	long	num_find;					/* ロゴ表示期間の組数 */
	LOGO_OUTSUBREC res[LOGO_FIND_MAX];	/* ロゴ表示期間の結果 */
} LOGO_RESULTOUTREC;

#define FILE_BUFSIZE 512

// max number of logo data
#define LOGONUM_MAX 100
// "-logo"オプションで取得するファイル名最大数
#define FILELISTNUM_MAX LOGONUM_MAX
// "-oanum"オプションの初期値
#define DEF_MLOGO_OANUM  90


// ロゴデータの拡張子
#define EXTNAME_LOGODATA   ".lgd"
#define EXTNAME_LOGOPARAM  ".logoframe.txt"

// logoframe初期読み込みオプションファイル名
#define INIFILE_NAME       "logoframe.ini"
// logoframe用セクション名
#define SECTION_LOGOFRAME  "[logoframe]"

// フォルダ区切り記号
#ifdef _WIN32
#define DELIMITER_DIR     '\\'
#define DELIMITER_STRDIR  "\\"
#else
#define DELIMITER_DIR     '/'
#define DELIMITER_STRDIR  "/"
#endif

// 閾値パラメータ更新保存用（更新フラグ）
typedef struct {
	char	up_num_fadein;
	char	up_num_fadeout;
	char	up_num_cutleft;
	char	up_num_cutright;
	char	up_num_onwidth;
	char	up_num_onlevel;
	char	up_num_offwidth;
	char	up_thres_ymax;
	char	up_thres_ymin;
	char	up_thres_yedge;
	char	up_thres_ydif;
	char	up_thres_ysetdif;
	char	up_thres_yoffedg;
	char	up_num_areaset;
	char	up_num_clrrate;
	char	up_auto_fade;
	char	up_auto_bs11;
} MLOGO_UPTHRESREC;

// 閾値パラメータ更新保存用
typedef struct {
	LOGO_THRESREC		dat;
	MLOGO_UPTHRESREC	up;
} MLOGO_THRESREC;


// 複数ロゴデータ格納
typedef struct {
	// 呼び出し元でも参照する値
	short dispoff;									// オプション "-nodisp"
	short paramoff;									// オプション "-nodispparam"
	short num_deflogo;								// 定義ロゴ数

	// logoframe複数ロゴ対応で使用するオプション
	short oanum;									// オプション "-oanum"
	short oasel;									// オプション "-oasel"
	short oamask;									// オプション "-oamask"
	short outform;									// オプション "-outform"
	char* opt_logofilename;							// オプション "-logo"
	char* opt_logoparamfile;						// オプション "-logoparam"
	char* opt_outfile;								// オプション "-o"
	char* opt_anafile;								// オプション "-oa"
	char* opt_ana2file;								// オプション "-oa2"
	char* all_logofilename[LOGONUM_MAX];			// ロゴファイル名リスト

	FILE*             fpo_ana2[LOGONUM_MAX];		// デバッグ用ファイル保持用
	LOGO_DATASET*     all_logodata[LOGONUM_MAX];	// 各ロゴデータ格納
	MLOGO_THRESREC    thres_arg;					// 閾値（引数指定）
	LOGO_RESULTOUTREC logoresult;					// 全体検出結果格納

	long  image_frames;								// 画像データフレーム数
	long  total_frame[LOGONUM_MAX];					// ロゴ表示期間フレーム合計
	short total_valid[LOGONUM_MAX];					// 有効ロゴ判別
	short priority_list[LOGONUM_MAX];				// ロゴ優先順位リスト
	short num_detect;								// CM検出有効ロゴ数
	short num_others;								// CM検出外ロゴ数
	short num_disable;								// 無効判断ロゴ数
} MLOGO_DATASET;

// logo function
void MultLogoInit(MLOGO_DATASET* pml);
void MultLogoFree(MLOGO_DATASET* pml);
int  MultLogoOptionAdd(MLOGO_DATASET* pml, const char* strcmd, const char* strval);
int  MultLogoOptionFile(MLOGO_DATASET* pml, const char* fname);
int  MultLogoOptionOrgFile(MLOGO_DATASET* pml);
int  MultLogoSetup(MLOGO_DATASET* pml, int num_frames);
void MultLogoDisplayParam(MLOGO_DATASET* pml);
void MultLogoCalc(MLOGO_DATASET* pml, const BYTE *data, int pitch, int nframe, int height);
void MultLogoFind(MLOGO_DATASET* pml);
extern int  MultLogoWrite(MLOGO_DATASET* pml);


#endif