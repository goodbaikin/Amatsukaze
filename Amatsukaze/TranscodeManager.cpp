/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "TranscodeManager.h"

AMTSplitter::AMTSplitter(AMTContext& ctx, const ConfigWrapper& setting)
    : TsSplitter(ctx, true, true, setting.isSubtitlesEnabled())
    , setting_(setting)
    , psWriter(ctx)
    , writeHandler(*this)
    , audioFile_(setting.getAudioFilePath(), _T("wb"))
    , waveFile_(setting.getWaveFilePath(), _T("wb"))
    , curVideoFormat_()
    , videoFileCount_(0)
    , videoStreamType_(-1)
    , audioStreamType_(-1)
    , audioFileSize_(0)
    , waveFileSize_(0)
    , srcFileSize_(0) {
    psWriter.setHandler(&writeHandler);
}

StreamReformInfo AMTSplitter::split() {
    readAll();

    // for debug
    printInteraceCount();

    return StreamReformInfo(ctx, videoFileCount_,
        videoFrameList_, audioFrameList_, captionTextList_, streamEventList_, timeList_);
}

int64_t AMTSplitter::getSrcFileSize() const {
    return srcFileSize_;
}

int64_t AMTSplitter::getTotalIntVideoSize() const {
    return writeHandler.getTotalSize();
}
AMTSplitter::StreamFileWriteHandler::StreamFileWriteHandler(TsSplitter& this_)
    : this_(this_), totalIntVideoSize_() {}
/* virtual */ void AMTSplitter::StreamFileWriteHandler::onStreamData(MemoryChunk mc) {
    if (file_ != NULL) {
        file_->write(mc);
        totalIntVideoSize_ += mc.length;
    }
}
void AMTSplitter::StreamFileWriteHandler::open(const tstring& path) {
    totalIntVideoSize_ = 0;
    file_ = std::unique_ptr<File>(new File(path, _T("wb")));
}
void AMTSplitter::StreamFileWriteHandler::close() {
    file_ = nullptr;
}
int64_t AMTSplitter::StreamFileWriteHandler::getTotalSize() const {
    return totalIntVideoSize_;
}

void AMTSplitter::readAll() {
    enum { BUFSIZE = 4 * 1024 * 1024 };
    auto buffer_ptr = std::unique_ptr<uint8_t[]>(new uint8_t[BUFSIZE]);
    MemoryChunk buffer(buffer_ptr.get(), BUFSIZE);
    File srcfile(setting_.getSrcFilePath(), _T("rb"));
    srcFileSize_ = srcfile.size();
    size_t readBytes;
    do {
        readBytes = srcfile.read(buffer);
        inputTsData(MemoryChunk(buffer.data, readBytes));
    } while (readBytes == buffer.length);
}

/* static */ bool AMTSplitter::CheckPullDown(PICTURE_TYPE p0, PICTURE_TYPE p1) {
    switch (p0) {
    case PIC_TFF:
    case PIC_BFF_RFF:
        return (p1 == PIC_TFF || p1 == PIC_TFF_RFF);
    case PIC_BFF:
    case PIC_TFF_RFF:
        return (p1 == PIC_BFF || p1 == PIC_BFF_RFF);
    default: // それ以外はチェック対象外
        return true;
    }
}

void AMTSplitter::printInteraceCount() {

    if (videoFrameList_.size() == 0) {
        ctx.error("フレームがありません");
        return;
    }

    // ラップアラウンドしないPTSを生成
    std::vector<std::pair<int64_t, int>> modifiedPTS;
    int64_t videoBasePTS = videoFrameList_[0].PTS;
    int64_t prevPTS = videoFrameList_[0].PTS;
    for (int i = 0; i < int(videoFrameList_.size()); ++i) {
        int64_t PTS = videoFrameList_[i].PTS;
        int64_t modPTS = prevPTS + int64_t((int32_t(PTS) - int32_t(prevPTS)));
        modifiedPTS.emplace_back(modPTS, i);
        prevPTS = modPTS;
    }

    // PTSでソート
    std::sort(modifiedPTS.begin(), modifiedPTS.end());

#if 0
    // フレームリストを出力
    FILE* framesfp = fopen("frames.txt", "w");
    fprintf(framesfp, "FrameNumber,DecodeFrameNumber,PTS,Duration,FRAME_TYPE,PIC_TYPE,IsGOPStart\n");
    for (int i = 0; i < (int)modifiedPTS.size(); ++i) {
        int64_t PTS = modifiedPTS[i].first;
        int decodeIndex = modifiedPTS[i].second;
        const VideoFrameInfo& frame = videoFrameList_[decodeIndex];
        int PTSdiff = -1;
        if (i < (int)modifiedPTS.size() - 1) {
            int64_t nextPTS = modifiedPTS[i + 1].first;
            const VideoFrameInfo& nextFrame = videoFrameList_[modifiedPTS[i + 1].second];
            PTSdiff = int(nextPTS - PTS);
            if (CheckPullDown(frame.pic, nextFrame.pic) == false) {
                ctx.warn("Flag Check Error: PTS=%lld %s -> %s",
                    PTS, PictureTypeString(frame.pic), PictureTypeString(nextFrame.pic));
            }
        }
        fprintf(framesfp, "%d,%d,%lld,%d,%s,%s,%d\n",
            i, decodeIndex, PTS, PTSdiff, FrameTypeString(frame.type), PictureTypeString(frame.pic), frame.isGopStart ? 1 : 0);
    }
    fclose(framesfp);
#endif

    // PTS間隔を出力
    struct Integer {
        int v;
        Integer() : v(0) {}
    };

    std::array<int, MAX_PIC_TYPE> interaceCounter = { 0 };
    std::map<int, Integer> PTSdiffMap;
    prevPTS = -1;
    for (const auto& ptsIndex : modifiedPTS) {
        int64_t PTS = ptsIndex.first;
        const VideoFrameInfo& frame = videoFrameList_[ptsIndex.second];
        interaceCounter[(int)frame.pic]++;
        if (prevPTS != -1) {
            int PTSdiff = int(PTS - prevPTS);
            PTSdiffMap[PTSdiff].v++;
        }
        prevPTS = PTS;
    }

    ctx.info("[映像フレーム統計情報]");

    int64_t totalTime = modifiedPTS.back().first - videoBasePTS;
    double sec = (double)totalTime / MPEG_CLOCK_HZ;
    int minutes = (int)(sec / 60);
    sec -= minutes * 60;
    ctx.infoF("時間: %d分%.3f秒", minutes, sec);

    ctx.infoF("FRAME=%d DBL=%d TLP=%d TFF=%d BFF=%d TFF_RFF=%d BFF_RFF=%d",
        interaceCounter[0], interaceCounter[1], interaceCounter[2], interaceCounter[3], interaceCounter[4], interaceCounter[5], interaceCounter[6]);

    for (const auto& pair : PTSdiffMap) {
        ctx.infoF("(PTS_Diff,Cnt)=(%d,%d)", pair.first, pair.second.v);
    }
}

// TsSplitter仮想関数 //

/* virtual */ void AMTSplitter::onVideoPesPacket(
    int64_t clock,
    const std::vector<VideoFrameInfo>& frames,
    PESPacket packet) {
    for (const VideoFrameInfo& frame : frames) {
        videoFrameList_.push_back(frame);
        videoFrameList_.back().fileOffset = writeHandler.getTotalSize();
    }
    psWriter.outVideoPesPacket(clock, frames, packet);
}

/* virtual */ void AMTSplitter::onVideoFormatChanged(VideoFormat fmt) {
    ctx.info("[映像フォーマット変更]");

    StringBuilder sb;
    sb.append("サイズ: %dx%d", fmt.width, fmt.height);
    if (fmt.width != fmt.displayWidth || fmt.height != fmt.displayHeight) {
        sb.append(" 表示領域: %dx%d", fmt.displayWidth, fmt.displayHeight);
    }
    int darW, darH; fmt.getDAR(darW, darH);
    sb.append(" (%d:%d)", darW, darH);
    if (fmt.fixedFrameRate) {
        sb.append(" FPS: %d/%d", fmt.frameRateNum, fmt.frameRateDenom);
    } else {
        sb.append(" FPS: VFR");
    }
    ctx.info(sb.str().c_str());

    // ファイル変更
    if (!curVideoFormat_.isBasicEquals(fmt)) {
        // アスペクト比以外も変更されていたらファイルを分ける
        //（StreamReformと条件を合わせなければならないことに注意）
        writeHandler.open(setting_.getIntVideoFilePath(videoFileCount_++));
        psWriter.outHeader(videoStreamType_, audioStreamType_);
    }
    curVideoFormat_ = fmt;

    StreamEvent ev = StreamEvent();
    ev.type = VIDEO_FORMAT_CHANGED;
    ev.frameIdx = (int)videoFrameList_.size();
    streamEventList_.push_back(ev);
}

/* virtual */ void AMTSplitter::onAudioPesPacket(
    int audioIdx,
    int64_t clock,
    const std::vector<AudioFrameData>& frames,
    PESPacket packet) {
    for (const AudioFrameData& frame : frames) {
        FileAudioFrameInfo info = frame;
        info.audioIdx = audioIdx;
        info.codedDataSize = frame.codedDataSize;
        info.waveDataSize = frame.decodedDataSize;
        info.fileOffset = audioFileSize_;
        info.waveOffset = waveFileSize_;
        audioFile_.write(MemoryChunk(frame.codedData, frame.codedDataSize));
        if (frame.decodedDataSize > 0) {
            waveFile_.write(MemoryChunk((uint8_t*)frame.decodedData, frame.decodedDataSize));
        }
        audioFileSize_ += frame.codedDataSize;
        waveFileSize_ += frame.decodedDataSize;
        audioFrameList_.push_back(info);
    }
    if (videoFileCount_ > 0) {
        psWriter.outAudioPesPacket(audioIdx, clock, frames, packet);
    }
}

/* virtual */ void AMTSplitter::onAudioFormatChanged(int audioIdx, AudioFormat fmt) {
    ctx.infoF("[音声%dフォーマット変更]", audioIdx);
    ctx.infoF("チャンネル: %s サンプルレート: %d",
        getAudioChannelString(fmt.channels), fmt.sampleRate);

    StreamEvent ev = StreamEvent();
    ev.type = AUDIO_FORMAT_CHANGED;
    ev.audioIdx = audioIdx;
    ev.frameIdx = (int)audioFrameList_.size();
    streamEventList_.push_back(ev);
}

/* virtual */ void AMTSplitter::onCaptionPesPacket(
    int64_t clock,
    std::vector<CaptionItem>& captions,
    PESPacket packet) {
    for (auto& caption : captions) {
        captionTextList_.emplace_back(std::move(caption));
    }
}

/* virtual */ DRCSOutInfo AMTSplitter::getDRCSOutPath(int64_t PTS, const std::string& md5) {
    DRCSOutInfo info;
    info.elapsed = (videoFrameList_.size() > 0) ? (double)(PTS - videoFrameList_[0].PTS) : -1.0;
    info.filename = setting_.getDRCSOutPath(md5);
    return info;
}

// TsPacketSelectorHandler仮想関数 //

/* virtual */ void AMTSplitter::onPidTableChanged(const PMTESInfo video, const std::vector<PMTESInfo>& audio, const PMTESInfo caption) {
    // ベースクラスの処理
    TsSplitter::onPidTableChanged(video, audio, caption);

    ASSERT(audio.size() > 0);
    videoStreamType_ = video.stype;
    audioStreamType_ = audio[0].stype;

    StreamEvent ev = StreamEvent();
    ev.type = PID_TABLE_CHANGED;
    ev.numAudio = (int)audio.size();
    ev.frameIdx = (int)videoFrameList_.size();
    streamEventList_.push_back(ev);
}

/* virtual */ void AMTSplitter::onTime(int64_t clock, JSTTime time) {
    timeList_.push_back(std::make_pair(clock, time));
}


/* static */ tstring replaceOptions(
    const tstring& options,
    const VideoFormat& fmt,
    const ConfigWrapper& setting,
    const EncodeFileKey key,
    const int serviceID) {
    tstring ret = options;
    ret = str_replace(ret, _T("@IMAGE_WIDTH@"), StringFormat(_T("%d"), fmt.width));
    ret = str_replace(ret, _T("@IMAGE_HEIGHT@"), StringFormat(_T("%d"), fmt.height));
    ret = str_replace(ret, _T("@SERVICE_ID@"), StringFormat(_T("%d"), serviceID));
    ret = str_replace(ret, _T("@AMT_ENCODER@"), to_tstring(encoderToString(setting.getEncoder())));
    ret = str_replace(ret, _T("@AMT_AUDIO_ENCODER@"), to_tstring(audioEncoderToString(setting.getAudioEncoder())));
    ret = str_replace(ret, _T("@AMT_TEMP_DIR@"), setting.getTmpDir());
    ret = str_replace(ret, _T("@AMT_TEMP_VIDEO@"), _T("\"") + setting.getEncVideoFilePath(key) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_AUDIO@"), _T("\"") + setting.getIntAudioFilePath(key, 0, setting.getAudioEncoder()) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_AUDIO_0@"), _T("\"") + setting.getIntAudioFilePath(key, 0, setting.getAudioEncoder()) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_AUDIO_1@"), _T("\"") + setting.getIntAudioFilePath(key, 1, setting.getAudioEncoder()) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_CHAPTER@"), _T("\"") + setting.getTmpChapterPath(key) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_TIMECODE@"), _T("\"") + setting.getAvsTimecodePath(key) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS@"), _T("\"") + setting.getTmpASSFilePath(key, 0) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_0@"), _T("\"") + setting.getTmpASSFilePath(key, 0) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_1@"), _T("\"") + setting.getTmpASSFilePath(key, 1) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_SRT@"), _T("\"") + setting.getTmpSRTFilePath(key, 0) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_SRT_0@"), _T("\"") + setting.getTmpSRTFilePath(key, 0) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_SRT_1@"), _T("\"") + setting.getTmpSRTFilePath(key, 1) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_NICOJK_720S@"), _T("\"") + setting.getTmpNicoJKASSPath(key, NICOJK_720S) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_NICOJK_720T@"), _T("\"") + setting.getTmpNicoJKASSPath(key, NICOJK_720T) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_NICOJK_1080S@"), _T("\"") + setting.getTmpNicoJKASSPath(key, NICOJK_1080S) + _T("\""));
    ret = str_replace(ret, _T("@AMT_TEMP_ASS_NICOJK_1080T@"), _T("\"") + setting.getTmpNicoJKASSPath(key, NICOJK_1080T) + _T("\""));
    return ret;
}
EncoderArgumentGenerator::EncoderArgumentGenerator(
    const ConfigWrapper& setting,
    StreamReformInfo& reformInfo)
    : setting_(setting)
    , reformInfo_(reformInfo) {}

tstring EncoderArgumentGenerator::GenEncoderOptions(
    int numFrames,
    VideoFormat outfmt,
    std::vector<BitrateZone> zones,
    double vfrBitrateScale,
    tstring timecodepath,
    int vfrTimingFps,
    EncodeFileKey key, int pass, int serviceID,
    const EncoderOptionInfo& eoInfo) {
    VIDEO_STREAM_FORMAT srcFormat = reformInfo_.getVideoStreamFormat();
    double srcBitrate = getSourceBitrate(key.video);
    return makeEncoderArgs(
        setting_.getEncoder(),
        setting_.getEncoderPath(),
        replaceOptions(setting_.getOptions(
            numFrames,
            srcFormat, srcBitrate, false, pass, zones, setting_.getEncVideoOptionFilePath(key), vfrBitrateScale, key, eoInfo),
            outfmt, setting_, key, serviceID),
        outfmt,
        timecodepath,
        vfrTimingFps,
        setting_.getFormat(),
        setting_.getEncVideoFilePath(key));
}

// src, target
std::pair<double, double> EncoderArgumentGenerator::printBitrate(AMTContext& ctx, EncodeFileKey key) const {
    double srcBitrate = getSourceBitrate(key.video);
    ctx.infoF("入力映像ビットレート: %d kbps", (int)srcBitrate);
    VIDEO_STREAM_FORMAT srcFormat = reformInfo_.getVideoStreamFormat();
    double targetBitrate = std::numeric_limits<float>::quiet_NaN();
    if (setting_.isAutoBitrate()) {
        targetBitrate = setting_.getBitrate().getTargetBitrate(srcFormat, srcBitrate);
        if (key.cm == CMTYPE_CM) {
            targetBitrate *= setting_.getBitrateCM();
        }
        ctx.infoF("目標映像ビットレート: %d kbps", (int)targetBitrate);
    }
    return std::make_pair(srcBitrate, targetBitrate);
}

double EncoderArgumentGenerator::getSourceBitrate(int fileId) const {
    // ビットレート計算
    const auto& info = reformInfo_.getSrcVideoInfo(fileId);
    return ((double)info.first * 8 / 1000) / ((double)info.second / MPEG_CLOCK_HZ);
}

/* static */ std::vector<BitrateZone> MakeBitrateZones(
    const std::vector<double>& timeCodes,
    const std::vector<EncoderZone>& cmzones,
    const ConfigWrapper& setting,
    const EncoderOptionInfo& eoInfo,
    VideoInfo outvi) {
    std::vector<BitrateZone> bitrateZones;
    if (timeCodes.size() == 0 || setting.isEncoderSupportVFR()) {
        // VFRでない、または、エンコーダがVFRをサポートしている -> VFR用に調整する必要がない
        for (int i = 0; i < (int)cmzones.size(); ++i) {
            bitrateZones.emplace_back(cmzones[i], setting.getBitrateCM(), setting.getCMQualityOffset());
        }
    } else {
        if (setting.isZoneAvailable()) {
            // VFR非対応エンコーダでゾーンに対応していればビットレートゾーン生成
#if 0
            {
                File dump("zone_param.dat", "wb");
                dump.writeArray(frameDurations);
                dump.writeArray(cmzones);
                dump.writeValue(setting.getBitrateCM());
                dump.writeValue(outvi.fps_numerator);
                dump.writeValue(outvi.fps_denominator);
                dump.writeValue(setting.getX265TimeFactor());
                dump.writeValue(0.05);
            }
#endif
            if (auto rcMode = getRCMode(setting.getEncoder(), eoInfo.rcMode); !setting.isAutoBitrate() && rcMode && !rcMode->isBitrateMode) {
                for (int i = 0; i < (int)cmzones.size(); ++i) {
                    bitrateZones.emplace_back(cmzones[i], setting.getBitrateCM(), setting.getCMQualityOffset());
                }
            } else {
                return MakeVFRBitrateZones(
                    timeCodes, cmzones, setting.getBitrateCM(),
                    outvi.fps_numerator, outvi.fps_denominator,
                    setting.getX265TimeFactor(), 0.05); // 全体で5%までの差なら許容する
            }
        }
    }
    return bitrateZones;
}

// ページヒープが機能しているかテスト
void DoBadThing() {
#ifdef _WIN32
    char *p = (char*)HeapAlloc(
        GetProcessHeap(),
        HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
        8);
    memset(p, 'x', 32);
#endif
}
/* static */ void transcodeMain(AMTContext& ctx, const ConfigWrapper& setting) {
#if 0
    MessageBox(NULL, "Debug", "Amatsukaze", MB_OK);
    //DoBadThing();
#endif
    const_cast<ConfigWrapper&>(setting).CreateTempDir();
    setting.dump();

    bool isNoEncode = (setting.getMode() == _T("cm"));

    auto eoInfo = ParseEncoderOption(setting.getEncoder(), setting.getEncoderOptions());
    PrintEncoderInfo(ctx, eoInfo);

    // チェック
    if (!isNoEncode && !setting.isFormatVFRSupported() && eoInfo.afsTimecode) {
        THROW(FormatException, "M2TS/TS出力はVFRをサポートしていません");
    }
    if (setting.getFormat() == FORMAT_TSREPLACE) {
        auto cmtypes = setting.getCMTypes();
        if (cmtypes.size() != 1 || cmtypes[0] != CMTYPE_BOTH) {
            THROW(FormatException, "tsreplaceはCMカットに対応していません");
        }
        if (eoInfo.format != VS_H264 && eoInfo.format != VS_H265) {
            THROW(FormatException, "tsreplaceはH.264/H.265以外には対応していません");
        }
    }

    ResourceManger rm(ctx, setting.getInPipe(), setting.getOutPipe());
    rm.wait(HOST_CMD_TSAnalyze);

    Stopwatch sw;
    sw.start();
    auto splitter = std::unique_ptr<AMTSplitter>(new AMTSplitter(ctx, setting));
    if (setting.getServiceId() > 0) {
        splitter->setServiceId(setting.getServiceId());
    }
    StreamReformInfo reformInfo = splitter->split();
    ctx.infoF("TS解析完了: %.2f秒", sw.getAndReset());
    const int serviceId = splitter->getActualServiceId();
    const int64_t numTotalPackets = splitter->getNumTotalPackets();
    const int64_t numScramblePackets = splitter->getNumScramblePackets();
    const int64_t totalIntVideoSize = splitter->getTotalIntVideoSize();
    const int64_t srcFileSize = splitter->getSrcFileSize();
    splitter = nullptr;

    if (setting.isDumpStreamInfo()) {
        reformInfo.serialize(setting.getStreamInfoPath());
    }

    // スクランブルパケットチェック
    double scrambleRatio = (double)numScramblePackets / (double)numTotalPackets;
    if (scrambleRatio > 0.01) {
        ctx.errorF("%.2f%%のパケットがスクランブル状態です。", scrambleRatio * 100);
        if (scrambleRatio > 0.3) {
            THROW(FormatException, "スクランブルパケットが多すぎます");
        }
    }

    if (!isNoEncode && setting.isIgnoreNoDrcsMap() == false) {
        // DRCSマッピングチェック
        if (ctx.getErrorCount(AMT_ERR_NO_DRCS_MAP) > 0) {
            THROW(NoDrcsMapException, "マッピングにないDRCS外字あり正常に字幕処理できなかったため終了します");
        }
    }

    reformInfo.prepare(setting.isSplitSub(), setting.isEncodeAudio(), setting.getFormat() == FORMAT_TSREPLACE);

    time_t startTime = reformInfo.getFirstFrameTime();

    NicoJK nicoJK(ctx, setting);
    bool nicoOK = false;
    if (!isNoEncode && setting.isNicoJKEnabled()) {
        ctx.info("[ニコニコ実況コメント取得]");
        auto srcDuration = reformInfo.getInDuration() / MPEG_CLOCK_HZ;
        nicoOK = nicoJK.makeASS(serviceId, startTime, (int)srcDuration);
        if (nicoOK) {
            reformInfo.SetNicoJKList(nicoJK.getDialogues());
        } else {
            if (nicoJK.isFail() == false) {
                ctx.info("対応チャンネルがありません");
            } else if (setting.isIgnoreNicoJKError() == false) {
                THROW(RuntimeException, "ニコニコ実況コメント取得に失敗");
            }
        }
    }

    int numVideoFiles = reformInfo.getNumVideoFile();
    int mainFileIndex = reformInfo.getMainVideoFileIndex();
    std::vector<std::unique_ptr<CMAnalyze>> cmanalyze;

    // ソースファイル読み込み用データ保存
    for (int videoFileIndex = 0; videoFileIndex < numVideoFiles; ++videoFileIndex) {
        // ファイル読み込み情報を保存
        auto& fmt = reformInfo.getFormat(EncodeFileKey(videoFileIndex, 0));
        auto amtsPath = setting.getTmpAMTSourcePath(videoFileIndex);
        av::SaveAMTSource(amtsPath,
            setting.getIntVideoFilePath(videoFileIndex),
            setting.getWaveFilePath(),
            fmt.videoFormat, fmt.audioFormat[0],
            reformInfo.getFilterSourceFrames(videoFileIndex),
            reformInfo.getFilterSourceAudioFrames(videoFileIndex),
            setting.getDecoderSetting());
    }

    // ロゴ・CM解析
    rm.wait(HOST_CMD_CMAnalyze);
    sw.start();
    std::vector<std::pair<size_t, bool>> logoFound;
    std::vector<std::unique_ptr<MakeChapter>> chapterMakers(numVideoFiles);
    for (int videoFileIndex = 0; videoFileIndex < numVideoFiles; ++videoFileIndex) {
        cmanalyze.push_back(std::make_unique<CMAnalyze>(ctx, setting));

        const int numFrames = (int)reformInfo.getFilterSourceFrames(videoFileIndex).size();
        const bool delogoEnabled = setting.isNoDelogo() ? false : true;
        // チャプター解析は300フレーム（約10秒）以上ある場合だけ
        //（短すぎるとエラーになることがあるので
        const bool analyzeChapterAndCM = (setting.isChapterEnabled() && numFrames >= 300);
        CMAnalyze *cma = cmanalyze.back().get();
        if (analyzeChapterAndCM || delogoEnabled) {
            cma->analyze(serviceId, videoFileIndex, numFrames, analyzeChapterAndCM);
        }

        if (analyzeChapterAndCM && setting.isPmtCutEnabled()) {
            // PMT変更によるCM追加認識
            cma->applyPmtCut(numFrames, setting.getPmtCutSideRate(),
                reformInfo.getPidChangedList(videoFileIndex));
        }

        if (videoFileIndex == mainFileIndex) {
            if (setting.getTrimAVSPath().size()) {
                // Trim情報入力
                cma->inputTrimAVS(numFrames, setting.getTrimAVSPath());
            }
        }

        logoFound.emplace_back(numFrames, cma->getLogoPath().size() > 0);
        reformInfo.applyCMZones(videoFileIndex, cma->getZones(), cma->getDivs());

        if (analyzeChapterAndCM) {
            chapterMakers[videoFileIndex] = std::unique_ptr<MakeChapter>(
                new MakeChapter(ctx, setting, reformInfo, videoFileIndex, cma->getTrims()));
        }
    }
    if (setting.isChapterEnabled()) {
        // ロゴがあったかチェック //
        // 映像ファイルをフレーム数でソート
        std::sort(logoFound.begin(), logoFound.end());
        if (setting.getLogoPath().size() > 0 && // ロゴ指定あり
            setting.isIgnoreNoLogo() == false &&          // ロゴなし無視でない
            logoFound.back().first >= 300 &&
            logoFound.back().second == false)     // 最も長い映像でロゴが見つからなかった
        {
            THROW(NoLogoException, "マッチするロゴが見つかりませんでした");
        }
        ctx.infoF("ロゴ・CM解析完了: %.2f秒", sw.getAndReset());
    }

    if (isNoEncode) {
        // CM解析のみならここで終了
        return;
    }

    auto audioDiffInfo = reformInfo.genAudio(setting.getCMTypes());
    audioDiffInfo.printAudioPtsDiff(ctx);

    const auto& allKeys = reformInfo.getOutFileKeys();
    std::vector<EncodeFileKey> keys;
    // 1秒以下なら出力しない
    std::copy_if(allKeys.begin(), allKeys.end(), std::back_inserter(keys),
        [&](EncodeFileKey key) { return reformInfo.getEncodeFile(key).duration >= MPEG_CLOCK_HZ; });

    std::vector<EncodeFileOutput> outFileInfo(keys.size());

    ctx.info("[チャプター生成]");
    for (int i = 0; i < (int)keys.size(); ++i) {
        auto key = keys[i];
        if (chapterMakers[key.video]) {
            chapterMakers[key.video]->exec(key);
        }
    }

    ctx.info("[字幕ファイル生成]");
    for (int i = 0; i < (int)keys.size(); ++i) {
        auto key = keys[i];
        CaptionASSFormatter formatterASS(ctx);
        CaptionSRTFormatter formatterSRT(ctx);
        NicoJKFormatter formatterNicoJK(ctx);
        const auto& capList = reformInfo.getEncodeFile(key).captionList;
        for (int lang = 0; lang < capList.size(); ++lang) {
            auto ass = formatterASS.generate(capList[lang]);
            auto srt = formatterSRT.generate(capList[lang]);
            WriteUTF8File(setting.getTmpASSFilePath(key, lang), ass);
            if (srt.size() > 0) {
                // SRTはCP_STR_SMALLしかなかった場合など出力がない場合があり、
                // 空ファイルはmux時にエラーになるので、1行もない場合は出力しない
                WriteUTF8File(setting.getTmpSRTFilePath(key, lang), srt);
            }
        }
        if (nicoOK) {
            const auto& headerLines = nicoJK.getHeaderLines();
            const auto& dialogues = reformInfo.getEncodeFile(key).nicojkList;
            for (NicoJKType jktype : setting.getNicoJKTypes()) {
                File file(setting.getTmpNicoJKASSPath(key, jktype), _T("w"));
                auto text = formatterNicoJK.generate(headerLines[(int)jktype], dialogues[(int)jktype]);
                file.write(MemoryChunk((uint8_t*)text.data(), text.size()));
            }
        }
    }
    ctx.infoF("字幕ファイル生成完了: %.2f秒", sw.getAndReset());

    if (setting.isEncodeAudio()) {
        ctx.info("[音声エンコード]");
        for (int i = 0; i < (int)keys.size(); ++i) {
            auto key = keys[i];
            auto outpath = setting.getIntAudioFilePath(key, 0, setting.getAudioEncoder());
            auto args = makeAudioEncoderArgs(
                setting.getAudioEncoder(),
                setting.getAudioEncoderPath(),
                setting.getAudioEncoderOptions(),
                setting.getAudioBitrateInKbps(),
                outpath);
            auto format = reformInfo.getFormat(key);
            auto audioFrames = reformInfo.getWaveInput(reformInfo.getEncodeFile(key).audioFrames[0]);
            EncodeAudio(ctx, args, setting.getWaveFilePath(), format.audioFormat[0], audioFrames);
        }
    }

    auto argGen = std::unique_ptr<EncoderArgumentGenerator>(new EncoderArgumentGenerator(setting, reformInfo));

    sw.start();
    for (int i = 0; i < (int)keys.size(); ++i) {
        auto key = keys[i];
        auto& fileOut = outFileInfo[i];
        const CMAnalyze* cma = cmanalyze[key.video].get();

        AMTFilterSource filterSource(ctx, setting, reformInfo,
            cma->getZones(), cma->getLogoPath(), key, rm);

        try {
            PClip filterClip = filterSource.getClip();
            IScriptEnvironment2* env = filterSource.getEnv();
            auto encoderZones = filterSource.getZones();
            auto& outfmt = filterSource.getFormat();
            auto& outvi = filterClip->GetVideoInfo();
            auto& timeCodes = filterSource.getTimeCodes();

            ctx.infoF("[エンコード開始] %d/%d %s", i + 1, (int)keys.size(), CMTypeToString(key.cm));
            auto bitrate = argGen->printBitrate(ctx, key);

            fileOut.vfmt = outfmt;
            fileOut.srcBitrate = bitrate.first;
            fileOut.targetBitrate = bitrate.second;
            fileOut.vfrTimingFps = filterSource.getVfrTimingFps();

            if (timeCodes.size() > 0) {
                // フィルタによるVFRが有効
                if (eoInfo.afsTimecode) {
                    THROW(ArgumentException, "エンコーダとフィルタの両方でVFRタイムコードが出力されています。");
                }
                if (eoInfo.selectEvery > 1) {
                    THROW(ArgumentException, "VFRで出力する場合は、エンコーダで間引くことはできません");
                } else if (!setting.isFormatVFRSupported()) {
                    THROW(FormatException, "M2TS/TS出力はVFRをサポートしていません");
                }
                ctx.infoF("VFRタイミング: %d fps", fileOut.vfrTimingFps);
                fileOut.timecode = setting.getAvsTimecodePath(key);
            }

            std::vector<int> pass;
            if (setting.isTwoPass()) {
                pass.push_back(1);
                pass.push_back(2);
            } else {
                pass.push_back(-1);
            }

            auto bitrateZones = MakeBitrateZones(timeCodes, encoderZones, setting, eoInfo, outvi);
            auto vfrBitrateScale = AdjustVFRBitrate(timeCodes, outvi.fps_numerator, outvi.fps_denominator);
            // VFRフレームタイミングが120fpsか
            std::vector<tstring> encoderArgs;
            for (int i = 0; i < (int)pass.size(); ++i) {
                encoderArgs.push_back(
                    argGen->GenEncoderOptions(
                        outvi.num_frames,
                        outfmt, bitrateZones, vfrBitrateScale,
                        fileOut.timecode, fileOut.vfrTimingFps, key, pass[i], serviceId, eoInfo));
            }
            // x264, x265, SVT-AV1のときはdisablePowerThrottoling=trueとする
            // QSV/NV/VCEEncではプロセス内で自動的に最適なように設定されるため不要
            const bool disablePowerThrottoling = (setting.getEncoder() == ENCODER_X264 || setting.getEncoder() == ENCODER_X265 || setting.getEncoder() == ENCODER_SVTAV1);
            AMTFilterVideoEncoder encoder(ctx, std::max(4, setting.getNumEncodeBufferFrames()));
            encoder.encode(filterClip, outfmt,
                timeCodes, encoderArgs, disablePowerThrottoling, env);
        } catch (const AvisynthError& avserror) {
            THROWF(AviSynthException, "%s", avserror.msg);
        }
    }
    ctx.infoF("エンコード完了: %.2f秒", sw.getAndReset());

    argGen = nullptr;

    rm.wait(HOST_CMD_Mux);
    sw.start();
    int64_t totalOutSize = 0;
    auto muxer = std::unique_ptr<AMTMuxder>(new AMTMuxder(ctx, setting, reformInfo));
    for (int i = 0; i < (int)keys.size(); ++i) {
        auto key = keys[i];

        ctx.infoF("[Mux開始] %d/%d %s", i + 1, (int)keys.size(), CMTypeToString(key.cm));
        muxer->mux(key, eoInfo, nicoOK, outFileInfo[i]);

        totalOutSize += outFileInfo[i].fileSize;
    }
    ctx.infoF("Mux完了: %.2f秒", sw.getAndReset());

    muxer = nullptr;
    // 出力結果を表示
    reformInfo.printOutputMapping([&](EncodeFileKey key) {
        const auto& file = reformInfo.getEncodeFile(key);
        return setting.getOutFilePath(file.outKey, file.keyMax, getActualOutputFormat(key, reformInfo, setting), eoInfo.format);
        });

    // 出力結果JSON出力
    if (setting.getOutInfoJsonPath().size() > 0) {
        StringBuilder sb;
        sb.append("{ ")
            .append("\"srcpath\": \"%s\", ", toJsonString(setting.getSrcFilePath()))
            .append("\"outfiles\": [");
        for (int i = 0; i < (int)keys.size(); ++i) {
            if (i > 0) sb.append(", ");
            const auto& file = reformInfo.getEncodeFile(keys[i]);
            const auto& info = outFileInfo[i];
            sb.append("{ \"path\": \"%s\", \"srcbitrate\": %d, \"outbitrate\": %d, \"outfilesize\": %lld, ",
                toJsonString(setting.getOutFilePath(file.outKey, file.keyMax, getActualOutputFormat(keys[i], reformInfo, setting), eoInfo.format)), (int)info.srcBitrate,
                std::isnan(info.targetBitrate) ? -1 : (int)info.targetBitrate, info.fileSize);
            sb.append("\"subs\": [");
            for (int s = 0; s < (int)info.outSubs.size(); ++s) {
                if (s > 0) sb.append(", ");
                sb.append("\"%s\"", toJsonString(info.outSubs[s]));
            }
            sb.append("] }");
        }
        sb.append("]")
            .append(", \"logofiles\": [");
        for (int i = 0; i < reformInfo.getNumVideoFile(); ++i) {
            if (i > 0) sb.append(", ");
            sb.append("\"%s\"", toJsonString(cmanalyze[i]->getLogoPath()));
        }
        sb.append("]")
            .append(", \"srcfilesize\": %lld, \"intvideofilesize\": %lld, \"outfilesize\": %lld",
                srcFileSize, totalIntVideoSize, totalOutSize);
        auto duration = reformInfo.getInOutDuration();
        sb.append(", \"srcduration\": %.3f, \"outduration\": %.3f",
            (double)duration.first / MPEG_CLOCK_HZ, (double)duration.second / MPEG_CLOCK_HZ);
        sb.append(", \"audiodiff\": ");
        audioDiffInfo.printToJson(sb);
        sb.append(", \"error\": {");
        for (int i = 0; i < AMT_ERR_MAX; ++i) {
            if (i > 0) sb.append(", ");
            sb.append("\"%s\": %d", AMT_ERROR_NAMES[i], ctx.getErrorCount((AMT_ERROR_COUNTER)i));
        }
        sb.append(" }");
        sb.append(", \"cmanalyze\": %s", (setting.isChapterEnabled() ? "true" : "false"))
            .append(", \"nicojk\": %s", (nicoOK ? "true" : "false"))
            .append(", \"trimavs\": %s", (setting.getTrimAVSPath().size() ? "true" : "false"))
            .append(" }");

        std::string str = sb.str();
        MemoryChunk mc(reinterpret_cast<uint8_t*>(const_cast<char*>(str.data())), str.size());
        File file(setting.getOutInfoJsonPath(), _T("w"));
        file.write(mc);
    }
}

/* static */ void transcodeSimpleMain(AMTContext& ctx, const ConfigWrapper& setting) {
    if (ends_with(setting.getSrcFilePath(), _T(".ts"))) {
        ctx.warn("一般ファイルモードでのTSファイルの処理は非推奨です");
    }

    auto encoder = std::unique_ptr<AMTSimpleVideoEncoder>(new AMTSimpleVideoEncoder(ctx, setting));
    encoder->encode();
    int audioCount = encoder->getAudioCount();
    int64_t srcFileSize = encoder->getSrcFileSize();
    VideoFormat videoFormat = encoder->getVideoFormat();
    encoder = nullptr;

    auto muxer = std::unique_ptr<AMTSimpleMuxder>(new AMTSimpleMuxder(ctx, setting));
    muxer->mux(videoFormat, audioCount);
    int64_t totalOutSize = muxer->getTotalOutSize();
    muxer = nullptr;

    // 出力結果を表示
    ctx.info("完了");
    if (setting.getOutInfoJsonPath().size() > 0) {
        StringBuilder sb;
        sb.append("{ \"srcpath\": \"%s\"", toJsonString(setting.getSrcFilePath()))
            .append(", \"outpath\": \"%s\"", toJsonString(setting.getOutFilePath(EncodeFileKey(), EncodeFileKey(), setting.getFormat(), videoFormat.format)))
            .append(", \"srcfilesize\": %lld", srcFileSize)
            .append(", \"outfilesize\": %lld", totalOutSize)
            .append(" }");

        std::string str = sb.str();
        MemoryChunk mc(reinterpret_cast<uint8_t*>(const_cast<char*>(str.data())), str.size());
        File file(setting.getOutInfoJsonPath(), _T("w"));
        file.write(mc);
    }
}
DrcsSearchSplitter::DrcsSearchSplitter(AMTContext& ctx, const ConfigWrapper& setting)
    : TsSplitter(ctx, true, false, true)
    , setting_(setting) {}

void DrcsSearchSplitter::readAll() {
    enum { BUFSIZE = 4 * 1024 * 1024 };
    auto buffer_ptr = std::unique_ptr<uint8_t[]>(new uint8_t[BUFSIZE]);
    MemoryChunk buffer(buffer_ptr.get(), BUFSIZE);
    File srcfile(setting_.getSrcFilePath(), _T("rb"));
    size_t readBytes;
    do {
        readBytes = srcfile.read(buffer);
        inputTsData(MemoryChunk(buffer.data, readBytes));
    } while (readBytes == buffer.length);
}

// TsSplitter仮想関数 //

/* virtual */ void DrcsSearchSplitter::onVideoPesPacket(
    int64_t clock,
    const std::vector<VideoFrameInfo>& frames,
    PESPacket packet) {
    // 今の所最初のフレームしか必要ないけど
    for (const VideoFrameInfo& frame : frames) {
        videoFrameList_.push_back(frame);
    }
}

/* virtual */ void DrcsSearchSplitter::onVideoFormatChanged(VideoFormat fmt) {}

/* virtual */ void DrcsSearchSplitter::onAudioPesPacket(
    int audioIdx,
    int64_t clock,
    const std::vector<AudioFrameData>& frames,
    PESPacket packet) {}

/* virtual */ void DrcsSearchSplitter::onAudioFormatChanged(int audioIdx, AudioFormat fmt) {}

/* virtual */ void DrcsSearchSplitter::onCaptionPesPacket(
    int64_t clock,
    std::vector<CaptionItem>& captions,
    PESPacket packet) {}

/* virtual */ DRCSOutInfo DrcsSearchSplitter::getDRCSOutPath(int64_t PTS, const std::string& md5) {
    DRCSOutInfo info;
    info.elapsed = (videoFrameList_.size() > 0) ? (double)(PTS - videoFrameList_[0].PTS) : -1.0;
    info.filename = setting_.getDRCSOutPath(md5);
    return info;
}

/* virtual */ void DrcsSearchSplitter::onTime(int64_t clock, JSTTime time) {}
SubtitleDetectorSplitter::SubtitleDetectorSplitter(AMTContext& ctx, const ConfigWrapper& setting)
    : TsSplitter(ctx, true, false, true)
    , setting_(setting)
    , hasSubtltle_(false) {}

void SubtitleDetectorSplitter::readAll(int maxframes) {
    enum { BUFSIZE = 4 * 1024 * 1024 };
    auto buffer_ptr = std::unique_ptr<uint8_t[]>(new uint8_t[BUFSIZE]);
    MemoryChunk buffer(buffer_ptr.get(), BUFSIZE);
    File srcfile(setting_.getSrcFilePath(), _T("rb"));
    auto fileSize = srcfile.size();
    // ファイル先頭から10%のところから読む
    srcfile.seek(fileSize / 10, SEEK_SET);
    int64_t totalRead = 0;
    // 最後の10%は読まない
    int64_t end = fileSize / 10 * 9;
    size_t readBytes;
    do {
        readBytes = srcfile.read(buffer);
        inputTsData(MemoryChunk(buffer.data, readBytes));
        totalRead += readBytes;
    } while (totalRead < end && !hasSubtltle_ && videoFrameList_.size() < maxframes);
}

bool SubtitleDetectorSplitter::getHasSubtitle() const {
    return hasSubtltle_;
}

// TsSplitter仮想関数 //

/* virtual */ void SubtitleDetectorSplitter::onVideoPesPacket(
    int64_t clock,
    const std::vector<VideoFrameInfo>& frames,
    PESPacket packet) {
    // 今の所最初のフレームしか必要ないけど
    for (const VideoFrameInfo& frame : frames) {
        videoFrameList_.push_back(frame);
    }
}

/* virtual */ void SubtitleDetectorSplitter::onVideoFormatChanged(VideoFormat fmt) {}

/* virtual */ void SubtitleDetectorSplitter::onAudioPesPacket(
    int audioIdx,
    int64_t clock,
    const std::vector<AudioFrameData>& frames,
    PESPacket packet) {}

/* virtual */ void SubtitleDetectorSplitter::onAudioFormatChanged(int audioIdx, AudioFormat fmt) {}

/* virtual */ void SubtitleDetectorSplitter::onCaptionPesPacket(
    int64_t clock,
    std::vector<CaptionItem>& captions,
    PESPacket packet) {}

/* virtual */ DRCSOutInfo SubtitleDetectorSplitter::getDRCSOutPath(int64_t PTS, const std::string& md5) {
    return DRCSOutInfo();
}

/* virtual */ void SubtitleDetectorSplitter::onTime(int64_t clock, JSTTime time) {}

/* virtual */ void SubtitleDetectorSplitter::onCaptionPacket(int64_t clock, TsPacket packet) {
    hasSubtltle_ = true;
}
AudioDetectorSplitter::AudioDetectorSplitter(AMTContext& ctx, const ConfigWrapper& setting)
    : TsSplitter(ctx, true, true, false)
    , setting_(setting) {}

void AudioDetectorSplitter::readAll(int maxframes) {
    enum { BUFSIZE = 4 * 1024 * 1024 };
    auto buffer_ptr = std::unique_ptr<uint8_t[]>(new uint8_t[BUFSIZE]);
    MemoryChunk buffer(buffer_ptr.get(), BUFSIZE);
    File srcfile(setting_.getSrcFilePath(), _T("rb"));
    auto fileSize = srcfile.size();
    // ファイル先頭から10%のところから読む
    srcfile.seek(fileSize / 10, SEEK_SET);
    int64_t totalRead = 0;
    // 最後の10%は読まない
    int64_t end = fileSize / 10 * 9;
    size_t readBytes;
    do {
        readBytes = srcfile.read(buffer);
        inputTsData(MemoryChunk(buffer.data, readBytes));
        totalRead += readBytes;
    } while (totalRead < end && videoFrameList_.size() < maxframes);
}

// TsSplitter仮想関数 //

/* virtual */ void AudioDetectorSplitter::onVideoPesPacket(
    int64_t clock,
    const std::vector<VideoFrameInfo>& frames,
    PESPacket packet) {
    // 今の所最初のフレームしか必要ないけど
    for (const VideoFrameInfo& frame : frames) {
        videoFrameList_.push_back(frame);
    }
}

/* virtual */ void AudioDetectorSplitter::onVideoFormatChanged(VideoFormat fmt) {}

/* virtual */ void AudioDetectorSplitter::onAudioPesPacket(
    int audioIdx,
    int64_t clock,
    const std::vector<AudioFrameData>& frames,
    PESPacket packet) {}

/* virtual */ void AudioDetectorSplitter::onAudioFormatChanged(int audioIdx, AudioFormat fmt) {
    printf("インデックス: %d チャンネル: %s サンプルレート: %d\n",
        audioIdx, getAudioChannelString(fmt.channels), fmt.sampleRate);
}

/* virtual */ void AudioDetectorSplitter::onCaptionPesPacket(
    int64_t clock,
    std::vector<CaptionItem>& captions,
    PESPacket packet) {}

/* virtual */ DRCSOutInfo AudioDetectorSplitter::getDRCSOutPath(int64_t PTS, const std::string& md5) {
    return DRCSOutInfo();
}

/* virtual */ void AudioDetectorSplitter::onTime(int64_t clock, JSTTime time) {}

/* static */ void searchDrcsMain(AMTContext& ctx, const ConfigWrapper& setting) {
    Stopwatch sw;
    sw.start();
    auto splitter = std::unique_ptr<DrcsSearchSplitter>(new DrcsSearchSplitter(ctx, setting));
    if (setting.getServiceId() > 0) {
        splitter->setServiceId(setting.getServiceId());
    }
    splitter->readAll();
    ctx.infoF("完了: %.2f秒", sw.getAndReset());
}

/* static */ void detectSubtitleMain(AMTContext& ctx, const ConfigWrapper& setting) {
    auto splitter = std::unique_ptr<SubtitleDetectorSplitter>(new SubtitleDetectorSplitter(ctx, setting));
    if (setting.getServiceId() > 0) {
        splitter->setServiceId(setting.getServiceId());
    }
    splitter->readAll(setting.getMaxFrames());
    printf("字幕%s\n", splitter->getHasSubtitle() ? "あり" : "なし");
}

/* static */ void detectAudioMain(AMTContext& ctx, const ConfigWrapper& setting) {
    auto splitter = std::unique_ptr<AudioDetectorSplitter>(new AudioDetectorSplitter(ctx, setting));
    if (setting.getServiceId() > 0) {
        splitter->setServiceId(setting.getServiceId());
    }
    splitter->readAll(setting.getMaxFrames());
}
