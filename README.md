# Amatsukaze

Automated MPEG2-TS Transcoder

## これは何？

TSファイルをエンコードしてmp4やmkvにするソフトです。

[rigaya/Amatsukaze](https://github.com/rigaya/Amatsukaze) をフォークして Linux 上で動くようにしました。

## ファイルの配置
とりあえずのところ、 amatsukaze の実行ファイルが入っているディレクトリの横の lib/libamatsukaze.so を モジュールとして読み込むように実装しています。
また lib/plugins64 以下に存在する AviSynth モジュールは自動的に読み込まれるようになっています。ロゴ消しの機能を使いたい場合は、[tobitti0/delogo-AviSynthPlus-Linux](https://github.com/tobitti0/delogo-AviSynthPlus-Linux)をビルドして、libdelogo.so をここに配置するようにしてください。
この辺の設定で改善希望があれば Issue を上げてください。

```
- bin
	- amatsukaze
	- tsreplace
	- x264
	- ...
- lib
	- libamatsukaze.so
  - plugins64
    - libdelogo.so
```

## 使用方法
使い方は AmatsukazeCLI と同じです。 GUI の機能は実装していません。

```
amatsukaze <オプション> -i <input.ts> -o <output.mp4>
オプション []はデフォルト値 
  -i|--input  <パス>  入力ファイルパス
  -o|--output <パス>  出力ファイルパス
  -s|--serviceid <数値> 処理するサービスIDを指定[]
  -w|--work   <パス>  一時ファイルパス[./]
  -et|--encoder-type <タイプ>  使用エンコーダタイプ[x264]
                      対応エンコーダ: x264,x265,QSVEnc,NVEnc,VCEEnc,SVT-AV1
  -e|--encoder <パス> エンコーダパス[x264.exe]
  -eo|--encoder-option <オプション> エンコーダへ渡すオプション[]
                      入力ファイルの解像度、アスペクト比、インタレースフラグ、
                      フレームレート、カラーマトリクス等は自動で追加されるので不要
  --sar w:h           SAR比の上書き (SVT-AV1使用時のみ有効)
  -b|--bitrate a:b:f  ビットレート計算式 映像ビットレートkbps = f*(a*s+b)
                      sは入力映像ビットレート、fは入力がH264の場合は入力されたfだが、
                      入力がMPEG2の場合はf=1とする
                      指定がない場合はビットレートオプションを追加しない
  -bcm|--bitrate-cm <float>   CM判定されたところのビットレート倍率
  --cm-quality-offset <float> CM判定されたところの品質オフセット
  --2pass             2passエンコード
  --splitsub          メイン以外のフォーマットは結合しない
  -aet|--audio-encoder-type <タイプ> 音声エンコーダ[]                      対応エンコーダ: neroAac, qaac, fdkaac, opusenc
                      指定しなければ音声はエンコードしない
  -ae|--audio-encoder <パス> 音声エンコーダ[]  -aeo|--audio-encoder-option <オプション> 音声エンコーダへ渡すオプション[]
  -fmt|--format <フォーマット> 出力フォーマット[mp4]
                      対応フォーマット: mp4,mkv,m2ts,ts
  --use-mkv-when-sub-exists 字幕がある場合にはmkv出力を強制する。
  -m|--muxer  <パス>  L-SMASHのmuxerまたはmkvmergeまたはtsMuxeRへのパス[muxer.exe]
  -t|--timelineeditor  <パス>  timelineeditorへのパス（MP4でVFR出力する場合に必要）[timelineeditor.exe]
  --mp4box <パス>     mp4boxへのパス（MP4で字幕処理する場合に必要）[mp4box.exe]
  --mkvmerge <パス>   mkvmergeへのパス（--use-mkv-when-sub-exists使用時に必要）[mkvmerge.exe]
  -f|--filter <パス>  フィルタAvisynthスクリプトへのパス[]
  -pf|--postfilter <パス>  ポストフィルタAvisynthスクリプトへのパス[]
  --mpeg2decoder <デコーダ>  MPEG2用デコーダ[default]
                      使用可能デコーダ: default,QSV,CUVID
  --h264decoder <デコーダ>  H264用デコーダ[default]
                      使用可能デコーダ: default,QSV,CUVID
  --chapter           チャプター・CM解析を行う
  --subtitles         字幕を処理する
  --nicojk            ニコニコ実況コメントを追加する
  --logo <パス>       ロゴファイルを指定（いくつでも指定可能）
  --erase-logo <パス> ロゴ消し用追加ロゴファイル。ロゴ消しに適用されます。（いくつでも指定可能）
  --drcs <パス>       DRCSマッピングファイルパス
  --ignore-no-drcsmap マッピングにないDRCS外字があっても処理を続行する
  --ignore-no-logo    ロゴが見つからなくても処理を続行する
  --ignore-nicojk-error ニコニコ実況取得でエラーが発生しても処理を続行する
  --no-delogo         ロゴ消しをしない（デフォルトはロゴがある場合は消します）
  --parallel-logo-analysis 並列ロゴ解析
  --loose-logo-detection ロゴ検出判定しきい値を低くします
  --max-fade-length <数値> ロゴの最大フェードフレーム数[16]
  --chapter-exe <パス> chapter_exe.exeへのパス
  --jls <パス>         join_logo_scp.exeへのパス
  --jls-cmd <パス>    join_logo_scpのコマンドファイルへのパス
  --jls-option <オプション>    join_logo_scpのコマンドファイルへのパス
  --trimavs <パス>    CMカット用Trim AVSファイルへのパス。メインファイルのCMカット出力でのみ使用される。
  --nicoass <パス>     NicoConvASSへのパス
  -om|--cmoutmask <数値> 出力マスク[1]
                      1 : 通常
                      2 : CMをカット
                      4 : CMのみ出力
                      ORも可 例) 6: 本編とCMを分離
  --nicojk18          ニコニコ実況コメントをnicojk18サーバから取得
  --nicojklog         ニコニコ実況コメントをNicoJKログフォルダから取得
                      (NicoConvASSを -nicojk 1 で呼び出します)
  --nicojkmask <数値> ニコニコ実況コメントマスク[1]
                      1 : 1280x720不透明
                      2 : 1280x720半透明
                      4 : 1920x1080不透明
                      8 : 1920x1080半透明
                      ORも可 例) 15: すべて出力
  --no-remove-tmp     一時ファイルを削除せずに残す
                      デフォルトは60fpsタイミングで生成
  --timefactor <数値>  x265やNVEncで疑似VFRレートコントロールするときの時間レートファクター[0.25]
  --pmt-cut <数値>:<数値>  PMT変更でCM認識するときの最大CM認識時間割合。全再生時間に対する割合で指定する。
                      例えば 0.1:0.2 とすると開始10%までにPMT変更があった場合はそのPMT変更までをCM認識する。
                      また終わりから20%までにPMT変更があった場合も同様にCM認識する。[0:0]
  -j|--json   <パス>  出力結果情報をJSON出力する場合は出力ファイルパスを指定[]
  --mode <モード>     処理モード[ts]
                      ts : MPGE2-TSを入力する通常エンコードモード
                      cm : エンコードまで行わず、CM解析までで終了するモード
                      drcs : マッピングのないDRCS外字画像だけ出力するモード
                      probe_subtitles : 字幕があるか判定
                      probe_audio : 音声フォーマットを出力
  --resource-manager <入力パイプ>:<出力パイプ> リソース管理ホストとの通信パイプ
  --affinity <グループ>:<マスク> CPUアフィニティ
                      グループはプロセッサグループ（64論理コア以下のシステムでは0のみ）
  --max-frames        probe_*モード時のみ有効。TSを見る時間を映像フレーム数で指定[9000]
  --dump              処理途中のデータをダンプ（デバッグ用）
```

## 未実装および未検証の機能
- 字幕処理：未検証
- ニコニコ実況コメント：未検証
- ロゴ作成： utvideo が Linux に対応していないため未実装

## ビルド方法
ビルドツール
- g++
- make

必要ライブラリ

- ffmpeg
- avisynth


初めに TVCaptionModLinux をビルド、インストールする必要があります。その後 Amatsukaze ディレクトリに入ってビルドしてください。