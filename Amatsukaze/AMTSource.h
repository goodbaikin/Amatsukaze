/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#pragma once

#include "common.h"

#include "avisynth.h"
#pragma comment(lib, "avisynth.lib")

#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include <set>
#include <deque>
#include <unordered_map>
#include "StreamReform.h"
#include "ReaderWriterFFmpeg.h"

typedef int64_t __int64;

namespace av {

struct FakeAudioSample {

    enum {
        MAGIC = 0xFACE0D10,
        VERSION = 1
    };

    int32_t magic;
    int32_t version;
    int64_t index;
};

struct AMTSourceData {
    std::vector<FilterSourceFrame> frames;
    std::vector<FilterAudioFrame> audioFrames;
};

class AMTSource : public IClip, AMTObject {
    const std::vector<FilterSourceFrame>& frames;
    const std::vector<FilterAudioFrame>& audioFrames;
    DecoderSetting decoderSetting;
    std::string filterdesc;
    int decodeThreads;
    int audioSamplesPerFrame;
    bool interlaced;

    bool outputQP; // QP�e�[�u�����o�͂��邩

    InputContext inputCtx;
    CodecContext codecCtx;

#if ENABLE_FFMPEG_FILTER
    FilterGraph filterGraph;
    AVFilterContext* bufferSrcCtx;
    AVFilterContext* bufferSinkCtx;
#endif

    AVStream *videoStream;

    std::unique_ptr<AMTSourceData> storage;

    struct CacheFrame {
        PVideoFrame data;
        int key;
    };

    std::map<int, CacheFrame*> frameCache;
    std::deque<CacheFrame*> recentAccessed;

    // �f�R�[�h�ł��Ȃ������t���[���̒u���惊�X�g
    std::map<int, int> failedMap;

    VideoInfo vi;

    std::mutex mutex;

    File waveFile;

    int seekDistance;

    // OnFrameDecoded�Œ��O�Ƀf�R�[�h���ꂽ�t���[��
    // �܂��f�R�[�h���ĂȂ��ꍇ��-1
    int lastDecodeFrame;

    // codecCtx�����O�Ƀf�R�[�h�����t���[���ԍ�
    // �܂��f�R�[�h���ĂȂ��ꍇ��nullptr
    std::unique_ptr<Frame> prevFrame;

    // ���O��non B QP�e�[�u��
    PVideoFrame nonBQPTable;

    AVCodec* getHWAccelCodec(AVCodecID vcodecId);

    void MakeCodecContext(IScriptEnvironment* env);

#if ENABLE_FFMPEG_FILTER
    void MakeFilterGraph(IScriptEnvironment* env);
#endif

    void MakeVideoInfo(const VideoFormat& vfmt, const AudioFormat& afmt);

    void UpdateVideoInfo(IScriptEnvironment* env);

    void ResetDecoder(IScriptEnvironment* env);

    template <typename T>
    void Copy1(T* dst, const T* top, const T* bottom, int w, int h, int dpitch, int tpitch, int bpitch) {
        for (int y = 0; y < h; y += 2) {
            T* dst0 = dst + dpitch * (y + 0);
            T* dst1 = dst + dpitch * (y + 1);
            const T* src0 = top + tpitch * (y + 0);
            const T* src1 = bottom + bpitch * (y + 1);
            memcpy(dst0, src0, sizeof(T) * w);
            memcpy(dst1, src1, sizeof(T) * w);
        }
    }

    template <typename T>
    void Copy2(T* dstU, T* dstV, const T* top, const T* bottom, int w, int h, int dpitch, int tpitch, int bpitch) {
        for (int y = 0; y < h; y += 2) {
            T* dstU0 = dstU + dpitch * (y + 0);
            T* dstU1 = dstU + dpitch * (y + 1);
            T* dstV0 = dstV + dpitch * (y + 0);
            T* dstV1 = dstV + dpitch * (y + 1);
            const T* src0 = top + tpitch * (y + 0);
            const T* src1 = bottom + bpitch * (y + 1);
            for (int x = 0; x < w; ++x) {
                dstU0[x] = src0[x * 2 + 0];
                dstV0[x] = src0[x * 2 + 1];
                dstU1[x] = src1[x * 2 + 0];
                dstV1[x] = src1[x * 2 + 1];
            }
        }
    }

    template <typename T>
    void MergeField(PVideoFrame& dst, AVFrame* top, AVFrame* bottom) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)(top->format));

        T* srctY = (T*)top->data[0];
        T* srctU = (T*)top->data[1];
        T* srctV = (top->format != AV_PIX_FMT_NV12) ? (T*)top->data[2] : ((T*)top->data[1] + 1);
        T* srcbY = (T*)bottom->data[0];
        T* srcbU = (T*)bottom->data[1];
        T* srcbV = (top->format != AV_PIX_FMT_NV12) ? (T*)bottom->data[2] : ((T*)bottom->data[1] + 1);
        T* dstY = (T*)dst->GetWritePtr(PLANAR_Y);
        T* dstU = (T*)dst->GetWritePtr(PLANAR_U);
        T* dstV = (T*)dst->GetWritePtr(PLANAR_V);

        int srctPitchY = top->linesize[0];
        int srctPitchUV = top->linesize[1];
        int srcbPitchY = bottom->linesize[0];
        int srcbPitchUV = bottom->linesize[1];
        int dstPitchY = dst->GetPitch(PLANAR_Y);
        int dstPitchUV = dst->GetPitch(PLANAR_U);

        Copy1<T>(dstY, srctY, srcbY, vi.width, vi.height, dstPitchY, srctPitchY, srcbPitchY);

        int widthUV = vi.width >> desc->log2_chroma_w;
        int heightUV = vi.height >> desc->log2_chroma_h;
        if (top->format != AV_PIX_FMT_NV12) {
            Copy1<T>(dstU, srctU, srcbU, widthUV, heightUV, dstPitchUV, srctPitchUV, srcbPitchUV);
            Copy1<T>(dstV, srctV, srcbV, widthUV, heightUV, dstPitchUV, srctPitchUV, srcbPitchUV);
        } else {
            Copy2<T>(dstU, dstV, srctU, srcbU, widthUV, heightUV, dstPitchUV, srctPitchUV, srcbPitchUV);
        }
    }

    PVideoFrame MakeFrame(AVFrame * top, AVFrame * bottom, IScriptEnvironment * env);

    void PutFrame(int n, const PVideoFrame & frame);

    int toAVSFormat(AVPixelFormat format, IScriptEnvironment * env);

#if ENABLE_FFMPEG_FILTER
    void InputFrameFilter(Frame* frame, bool enableOut, IScriptEnvironment* env);

    void OnFrameDecoded(Frame& frame, IScriptEnvironment* env);
#endif

    void OnFrameOutput(Frame& frame, IScriptEnvironment* env);

    void UpdateAccessed(CacheFrame* frame);

    PVideoFrame ForceGetFrame(int n, IScriptEnvironment* env);

    void DecodeLoop(int goal, IScriptEnvironment* env);

    void registerFailedFrames(int begin, int end, int replace, IScriptEnvironment* env);

public:
    AMTSource(AMTContext& ctx,
        const tstring& srcpath,
        const tstring& audiopath,
        const VideoFormat& vfmt, const AudioFormat& afmt,
        const std::vector<FilterSourceFrame>& frames,
        const std::vector<FilterAudioFrame>& audioFrames,
        const DecoderSetting& decoderSetting,
        const int threads,
        const char* filterdesc,
        bool outputQP,
        IScriptEnvironment* env);

    ~AMTSource();

    void TransferStreamInfo(std::unique_ptr<AMTSourceData>&& streamInfo);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

    const VideoInfo& __stdcall GetVideoInfo();

    bool __stdcall GetParity(int n);

    int __stdcall SetCacheHints(int cachehints, int frame_range);
};

extern AMTContext* g_ctx_for_plugin_filter;

void SaveAMTSource(
    const tstring& savepath,
    const tstring& srcpath,
    const tstring& audiopath,
    const VideoFormat& vfmt, const AudioFormat& afmt,
    const std::vector<FilterSourceFrame>& frames,
    const std::vector<FilterAudioFrame>& audioFrames,
    const DecoderSetting& decoderSetting);

PClip LoadAMTSource(const tstring& loadpath, const char* filterdesc, bool outputQP, IScriptEnvironment* env);

AVSValue CreateAMTSource(AVSValue args, void* user_data, IScriptEnvironment* env);

/*
class AVSLosslessSource : public IClip {
    LosslessVideoFile file;
    CCodecPointer codec;
    VideoInfo vi;
    std::unique_ptr<uint8_t[]> codedFrame;
    std::unique_ptr<uint8_t[]> rawFrame;
public:
    AVSLosslessSource(AMTContext& ctx, const tstring& filepath, const VideoFormat& format, IScriptEnvironment* env);

    ~AVSLosslessSource();

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

    void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
    const VideoInfo& __stdcall GetVideoInfo();
    bool __stdcall GetParity(int n);

    int __stdcall SetCacheHints(int cachehints, int frame_range);
};
*/

} // namespace av {
