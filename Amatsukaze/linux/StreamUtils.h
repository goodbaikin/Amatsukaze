/**
* Memory and Stream utility
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#pragma once
#ifndef STREAM_UTIL_H
#define STREAM_UTIL_H

#include <algorithm>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <fstream>

#include "CoreUtils.hpp"
#include "FileUtils.h"
#include "OSUtil.h"
#include "StringUtils.h"
#define CP_UTF8 65001

enum {
    TS_SYNC_BYTE = 0x47,

    TS_PACKET_LENGTH = 188,
    TS_PACKET_LENGTH2 = 192,

    MAX_PID = 0x1FFF,

    MPEG_CLOCK_HZ = 90000, // MPEG2,H264,H265��PTS��90kHz�P�ʂƂȂ��Ă���
};

inline static int nblocks(int n, int block) {
    return (n + block - 1) / block;
}

/** @brief shift�����E�V�t�g����mask��bit�����Ԃ�(bit shift mask) */
template <typename T>
T bsm(T v, int shift, int mask) {
    return (v >> shift) & ((T(1) << mask) - 1);
}

/** @brief mask��bit����shift�������V�t�g���ď�������(bit mask shift) */
template <typename T, typename U>
void bms(T& v, U data, int shift, int mask) {
    v |= (data & ((T(1) << mask) - 1)) << shift;
}

template<int bytes, typename T>
T readN(const uint8_t* ptr) {
    T r = 0;
    for (int i = 0; i < bytes; ++i) {
        r = r | (T(ptr[i]) << ((bytes - i - 1) * 8));
    }
    return r;
}
static uint16_t read16(const uint8_t* ptr) { return readN<2, uint16_t>(ptr); }
static uint32_t read24(const uint8_t* ptr) { return readN<3, uint32_t>(ptr); }
static uint32_t read32(const uint8_t* ptr) { return readN<4, uint32_t>(ptr); }
static uint64_t read40(const uint8_t* ptr) { return readN<5, uint64_t>(ptr); }
static uint64_t read48(const uint8_t* ptr) { return readN<6, uint64_t>(ptr); }

template<int bytes, typename T>
void writeN(uint8_t* ptr, T w) {
    for (int i = 0; i < bytes; ++i) {
        ptr[i] = uint8_t(w >> ((bytes - i - 1) * 8));
    }
}
static void write16(uint8_t* ptr, uint16_t w) { writeN<2, uint16_t>(ptr, w); }
static void write24(uint8_t* ptr, uint32_t w) { writeN<3, uint32_t>(ptr, w); }
static void write32(uint8_t* ptr, uint32_t w) { writeN<4, uint32_t>(ptr, w); }
static void write40(uint8_t* ptr, uint64_t w) { writeN<5, uint64_t>(ptr, w); }
static void write48(uint8_t* ptr, uint64_t w) { writeN<6, uint64_t>(ptr, w); }


class BitReader {
public:
    BitReader(MemoryChunk data)
        : data(data)
        , offset(0)
        , filled(0) {
        fill();
    }

    bool canRead(int bits) {
        return (filled + ((int)data.length - offset) * 8) >= bits;
    }

    // bits�r�b�g�ǂ�Ői�߂�
    // bits <= 32
    template <int bits>
    uint32_t read() {
        return readn(bits);
    }

    uint32_t readn(int bits) {
        if (bits <= filled) {
            return read_(bits);
        }
        fill();
        if (bits > filled) {
            throw EOFException("BitReader.read�ŃI�[�o�[����");
        }
        return read_(bits);
    }

    // bits�r�b�g�ǂނ���
    // bits <= 32
    template <int bits>
    uint32_t next() {
        return nextn(bits);
    }

    uint32_t nextn(int bits) {
        if (bits <= filled) {
            return next_(bits);
        }
        fill();
        if (bits > filled) {
            throw EOFException("BitReader.next�ŃI�[�o�[����");
        }
        return next_(bits);
    }

    int32_t readExpGolomSigned() {
        static int table[] = { 1, -1 };
        uint32_t v = readExpGolom() + 1;
        return (v >> 1) * table[v & 1];
    }

    uint32_t readExpGolom() {
        uint64_t masked = bsm(current, 0, filled);
        if (masked == 0) {
            fill();
            masked = bsm(current, 0, filled);
            if (masked == 0) {
                throw EOFException("BitReader.readExpGolom�ŃI�[�o�[����");
            }
        }
        int bodyLen = filled - __builtin_clzl(masked);
        filled -= bodyLen - 1;
        if (bodyLen > filled) {
            fill();
            if (bodyLen > filled) {
                throw EOFException("BitReader.readExpGolom�ŃI�[�o�[����");
            }
        }
        int shift = filled - bodyLen;
        filled -= bodyLen;
        return (uint32_t)bsm(current, shift, bodyLen) - 1;
    }

    void skip(int bits) {
        if (filled > bits) {
            filled -= bits;
        } else {
            // ��fill����Ă��镪������
            bits -= filled;
            filled = 0;
            // ����Ńo�C�g�A���C�������̂Ŏc��o�C�g���X�L�b�v
            int skipBytes = bits / 8;
            offset += skipBytes;
            if (offset > (int)data.length) {
                throw EOFException("BitReader.skip�ŃI�[�o�[����");
            }
            bits -= skipBytes * 8;
            // ����1��fill���Ďc�����r�b�g��������
            fill();
            if (bits > filled) {
                throw EOFException("BitReader.skip�ŃI�[�o�[����");
            }
            filled -= bits;
        }
    }

    // ���̃o�C�g���E�܂ł̃r�b�g���̂Ă�
    void byteAlign() {
        fill();
        filled &= ~7;
    }

    // �ǂ񂾃o�C�g���i���r���[�ȕ����܂œǂ񂾏ꍇ���P�o�C�g�Ƃ��Čv�Z�j
    int numReadBytes() {
        return offset - (filled / 8);
    }

private:
    MemoryChunk data;
    int offset;
    uint64_t current;
    int filled;

    void fill() {
        while (filled + 8 <= 64 && offset < (int)data.length) readByte();
    }

    void readByte() {
        current = (current << 8) | data.data[offset++];
        filled += 8;
    }

    uint32_t read_(int bits) {
        int shift = filled - bits;
        filled -= bits;
        return (uint32_t)bsm(current, shift, bits);
    }

    uint32_t next_(int bits) {
        int shift = filled - bits;
        return (uint32_t)bsm(current, shift, bits);
    }
};

class BitWriter {
public:
    BitWriter(AutoBuffer& dst)
        : dst(dst)
        , current(0)
        , filled(0) {}

    void writen(uint32_t data, int bits) {
        if (filled + bits > 64) {
            store();
        }
        current |= (((uint64_t)data & ((uint64_t(1) << bits) - 1)) << (64 - filled - bits));
        filled += bits;
    }

    template <int bits>
    void write(uint32_t data) {
        writen(data, bits);
    }

    template <bool bits>
    void byteAlign() {
        int pad = ((filled + 7) & ~7) - filled;
        filled += pad;
        if (bits) {
            current |= ((uint64_t(1) << pad) - 1) << (64 - filled);
        }
    }

    void flush() {
        if (filled & 7) {
            THROW(FormatException, "�o�C�g�A���C�����Ă��܂���");
        }
        store();
    }

private:
    AutoBuffer& dst;
    uint64_t current;
    int filled;

    void storeByte() {
        dst.add(uint8_t(current >> 56));
        current <<= 8;
        filled -= 8;
    }

    void store() {
        while (filled >= 8) storeByte();
    }
};

class CRC32 {
public:
    CRC32() {
        createTable(table, 0x04C11DB7UL);
    }

    uint32_t calc(const uint8_t* data, int length, uint32_t crc) const {
        for (int i = 0; i < length; ++i) {
            crc = (crc << 8) ^ table[(crc >> 24) ^ data[i]];
        }
        return crc;
    }

    const uint32_t* getTable() const { return table; }

private:
    uint32_t table[256];

    static void createTable(uint32_t* table, uint32_t exp) {
        for (int i = 0; i < 256; ++i) {
            uint32_t crc = i << 24;
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80000000UL) {
                    crc = (crc << 1) ^ exp;
                } else {
                    crc = crc << 1;
                }
            }
            table[i] = crc;
        }
    }
};

enum AMT_LOG_LEVEL {
    AMT_LOG_DEBUG,
    AMT_LOG_INFO,
    AMT_LOG_WARN,
    AMT_LOG_ERROR
};

enum AMT_ERROR_COUNTER {
    // �s����PTS�̃t���[��
    AMT_ERR_UNKNOWN_PTS = 0,
    // PES�p�P�b�g�f�R�[�h�G���[
    AMT_ERR_DECODE_PACKET_FAILED,
    // H264�ɂ�����PTS�~�X�}�b�`
    AMT_ERR_H264_PTS_MISMATCH,
    // H264�ɂ�����t�B�[���h�z�u�G���[
    AMT_ERR_H264_UNEXPECTED_FIELD,
    // PTS���߂��Ă���
    AMT_ERR_NON_CONTINUOUS_PTS,
    // DRCS�}�b�s���O���Ȃ�
    AMT_ERR_NO_DRCS_MAP,
    // �����ŃR�[�h�G���[
    AMT_ERR_DECODE_AUDIO,
    // �G���[�̌�
    AMT_ERR_MAX,
};

static const char* AMT_ERROR_NAMES[] = {
   "unknown-pts",
   "decode-packet-failed",
   "h264-pts-mismatch",
   "h264-unexpected-field",
   "non-continuous-pts",
     "no-drcs-map",
     "decode-audio-failed",
};

class AMTContext {
public:
    AMTContext()
        : timePrefix(true)
        , errCounter() {}

    const CRC32* getCRC() const {
        return &crc;
    }

    // �I�I���͕�������܂ޕ������fmt�ɓn���̂͋֎~�I�I
    // �i%���܂܂�Ă���ƌ듮�삷��̂Łj

    void debug(const char *str) const {
        print(str, AMT_LOG_DEBUG);
    }
    template <typename ... Args>
    void debugF(const char *fmt, const Args& ... args) const {
        print(StringFormat(fmt, args ...).c_str(), AMT_LOG_DEBUG);
    }
    void info(const char *str) const {
        print(str, AMT_LOG_INFO);
    }
    template <typename ... Args>
    void infoF(const char *fmt, const Args& ... args) const {
        print(StringFormat(fmt, args ...).c_str(), AMT_LOG_INFO);
    }
    void warn(const char *str) const {
        print(str, AMT_LOG_WARN);
    }
    template <typename ... Args>
    void warnF(const char *fmt, const Args& ... args) const {
        print(StringFormat(fmt, args ...).c_str(), AMT_LOG_WARN);
    }
    void error(const char *str) const {
        print(str, AMT_LOG_ERROR);
    }
    template <typename ... Args>
    void errorF(const char *fmt, const Args& ... args) const {
        print(StringFormat(fmt, args ...).c_str(), AMT_LOG_ERROR);
    }
    void progress(const char *str) const {
        printProgress(str);
    }
    template <typename ... Args>
    void progressF(const char *fmt, const Args& ... args) const {
        printProgress(StringFormat(fmt, args ...).c_str());
    }

    void registerTmpFile(const tstring& path) {
        tmpFiles.insert(path);
    }

    void clearTmpFiles() {
        for (auto& path : tmpFiles) {
            if (path.find(_T('*')) != tstring::npos) {
                std::string dir = pathGetDirectory(path);
                for (auto name : GetDirectoryFiles(dir, path.substr(dir.size() + 1))) {
                    auto path2 = dir + "/" + name;
                    removeT(path2.c_str());
                }
            } else {
                removeT(path.c_str());
            }
        }
        tmpFiles.clear();
    }

    void incrementCounter(AMT_ERROR_COUNTER err) {
        errCounter[err]++;
    }

    int getErrorCount(AMT_ERROR_COUNTER err) const {
        return errCounter[err];
    }

    void setError(const Exception& exception) {
        errMessage = exception.message();
    }

    const std::string& getError() const {
        return errMessage;
    }

    void setTimePrefix(bool enable) {
        timePrefix = enable;
    }

    const std::map<std::string, std::u16string>& getDRCSMapping() const {
        return drcsMap;
    }

    void loadDRCSMapping(const tstring& mapPath) {
        if (File::exists(mapPath) == false) {
            THROWF(ArgumentException, "DRCS�}�b�s���O�t�@�C����������܂���: %s",
                mapPath.c_str());
        } else {
            // BOM����UTF-8�œǂݍ���
            std::ifstream input(mapPath);
            // BOM�X�L�b�v
            {
                const char a = input.get();
                const char b = input.get();
                const char c = input.get();
                if (a != (char)0xEF || b != (char)0xBB || c != (char)0xBF) { //UTF-8 BOM
                    input.seekg(0);
                }
            }
            for (std::string line; getline(input, line);) {
                if (line.size() >= 34) {
                    std::string key = line.substr(0, 32);
                    std::transform(key.begin(), key.end(), key.begin(), toupper);
                    bool ok = (line[32] == '=');
                    for (auto c : key) if (!isxdigit(c)) ok = false;
                    if (ok) {
                        drcsMap[key] = to_u16string(line.substr(33), CP_UTF8);
                    }
                }
            }
        }
    }

    // �R���\�[���o�͂��f�t�H���g�R�[�h�y�[�W�ɐݒ�
    void setDefaultCP() {}

private:
    bool timePrefix;
    CRC32 crc;
    int acp;

    std::set<tstring> tmpFiles;
    std::array<int, AMT_ERR_MAX> errCounter;
    std::string errMessage;

    std::map<std::string, std::u16string> drcsMap;

    void printWithTimePrefix(const char* str) const {
        time_t rawtime;
        char buffer[80];

        time(&rawtime);
        tm * timeinfo = localtime(&rawtime);

        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        PRINTF("%s %s\n", buffer, str);
    }

    void print(const char* str, AMT_LOG_LEVEL level) const {
        if (timePrefix) {
            printWithTimePrefix(str);
        } else {
            static const char* log_levels[] = { "debug", "info", "warn", "error" };
            PRINTF("AMT [%s] %s\n", log_levels[level], str);
        }
    }

    void printProgress(const char* str) const {
        if (timePrefix) {
            printWithTimePrefix(str);
        } else {
            PRINTF("AMT %s\r", str);
        }
    }
};

class AMTObject {
public:
    AMTObject(AMTContext& ctx) : ctx(ctx) {}
    virtual ~AMTObject() {}
    AMTContext& ctx;
};

enum DECODER_TYPE {
    DECODER_DEFAULT = 0,
    DECODER_QSV,
    DECODER_CUVID,
};

struct DecoderSetting {
    DECODER_TYPE mpeg2;
    DECODER_TYPE h264;
    DECODER_TYPE hevc;

    DecoderSetting()
        : mpeg2(DECODER_DEFAULT)
        , h264(DECODER_DEFAULT)
        , hevc(DECODER_DEFAULT) {}
};

enum CMType {
    CMTYPE_BOTH = 0,
    CMTYPE_NONCM = 1,
    CMTYPE_CM = 2,
    CMTYPE_MAX
};

// �o�̓t�@�C������
struct EncodeFileKey {
    int video;   // ���ԃt�@�C���ԍ��i�f���t�H�[�}�b�g�؂�ւ��ɂ�镪���j
    int format;  // �t�H�[�}�b�g�ԍ��i�������̑��̃t�H�[�}�b�g�ύX�ɂ�镪���j
    int div;     // �����ԍ��iCM�\���F���ɂ�镪���j
    CMType cm;   // CM�^�C�v�i�{�ҁACM�Ȃǁj

    explicit EncodeFileKey()
        : video(0), format(0), div(0), cm(CMTYPE_BOTH) {}
    EncodeFileKey(int video, int format)
        : video(video), format(format), div(0), cm(CMTYPE_BOTH) {}
    EncodeFileKey(int video, int format, int div, CMType cm)
        : video(video), format(format), div(div), cm(cm) {}

    int key() const {
        return (video << 24) | (format << 14) | (div << 4) | cm;
    }
};

const char* CMTypeToString(CMType cmtype);

enum VIDEO_STREAM_FORMAT {
    VS_UNKNOWN,
    VS_MPEG2,
    VS_H264,
    VS_H265,
    VS_AV1
};

enum PICTURE_TYPE {
    PIC_FRAME = 0, // progressive frame
    PIC_FRAME_DOUBLING, // frame doubling
    PIC_FRAME_TRIPLING, // frame tripling
    PIC_TFF, // top field first
    PIC_BFF, // bottom field first
    PIC_TFF_RFF, // tff ���� repeat first field
    PIC_BFF_RFF, // bff ���� repeat first field
    MAX_PIC_TYPE,
};

const char* PictureTypeString(PICTURE_TYPE pic);

enum FRAME_TYPE {
    FRAME_NO_INFO = 0,
    FRAME_I,
    FRAME_P,
    FRAME_B,
    FRAME_OTHER,
    MAX_FRAME_TYPE,
};

const char* FrameTypeString(FRAME_TYPE frame);

double presenting_time(PICTURE_TYPE picType, double frameRate);

struct VideoFormat {
    VIDEO_STREAM_FORMAT format;
    int width, height; // �t���[���̉��c
    int displayWidth, displayHeight; // �t���[���̓��\���̈�̏c���i�\���̈撆�S�I�t�Z�b�g�̓[���Ɖ���j
    int sarWidth, sarHeight; // �A�X�y�N�g��
    int frameRateNum, frameRateDenom; // �t���[�����[�g
    uint8_t colorPrimaries, transferCharacteristics, colorSpace; // �J���[�X�y�[�X
    bool progressive, fixedFrameRate;

    bool isEmpty() const {
        return width == 0;
    }

    bool isSARUnspecified() const {
        return sarWidth == 0 && sarHeight == 1;
    }

    void mulDivFps(int mul, int div) {
        frameRateNum *= mul;
        frameRateDenom *= div;
        int g = gcd(frameRateNum, frameRateDenom);
        frameRateNum /= g;
        frameRateDenom /= g;
    }

    void getDAR(int& darWidth, int& darHeight) const {
        darWidth = displayWidth * sarWidth;
        darHeight = displayHeight * sarHeight;
        int denom = gcd(darWidth, darHeight);
        darWidth /= denom;
        darHeight /= denom;
    }

    // �A�X�y�N�g��͌��Ȃ�
    bool isBasicEquals(const VideoFormat& o) const {
        return (width == o.width && height == o.height
            && frameRateNum == o.frameRateNum && frameRateDenom == o.frameRateDenom
            && progressive == o.progressive);
    }

    // �A�X�y�N�g�������
    bool operator==(const VideoFormat& o) const {
        return (isBasicEquals(o)
            && displayWidth == o.displayWidth && displayHeight == o.displayHeight
            && sarWidth == o.sarWidth && sarHeight == o.sarHeight);
    }
    bool operator!=(const VideoFormat& o) const {
        return !(*this == o);
    }

private:
    static int gcd(int u, int v) {
        int r;
        while (0 != v) {
            r = u % v;
            u = v;
            v = r;
        }
        return u;
    }
};

struct VideoFrameInfo {
    int64_t PTS, DTS;
    // MPEG2�̏ꍇ sequence header ������
    // H264�̏ꍇ SPS ������
    bool isGopStart;
    bool progressive;
    PICTURE_TYPE pic;
    FRAME_TYPE type; // �g��Ȃ����ǎQ�l���
    int codedDataSize; // �f���r�b�g�X�g���[���ɂ�����o�C�g��
    VideoFormat format;
};

enum AUDIO_CHANNELS {
    AUDIO_NONE,

    AUDIO_MONO,
    AUDIO_STEREO,
    AUDIO_30, // 3/0
    AUDIO_31, // 3/1
    AUDIO_32, // 3/2
    AUDIO_32_LFE, // 5.1ch

    AUDIO_21, // 2/1
    AUDIO_22, // 2/2
    AUDIO_2LANG, // 2 ���� (1/ 0 + 1 / 0)

    // �ȉ�4K����
    AUDIO_52_LFE, // 7.1ch
    AUDIO_33_LFE, // 3/3.1
    AUDIO_2_22_LFE, // 2/0/0-2/0/2-0.1
    AUDIO_322_LFE, // 3/2/2.1
    AUDIO_2_32_LFE, // 2/0/0-3/0/2-0.1
    AUDIO_020_32_LFE, // 0/2/0-3/0/2-0.1 // AUDIO_2_32_LFE�Ƌ�ʂł��Ȃ��ˁH
    AUDIO_2_323_2LFE, // 2/0/0-3/2/3-0.2
    AUDIO_333_523_3_2LFE, // 22.2ch
};

const char* getAudioChannelString(AUDIO_CHANNELS channels);

int getNumAudioChannels(AUDIO_CHANNELS channels);

struct AudioFormat {
    AUDIO_CHANNELS channels;
    int sampleRate;

    bool operator==(const AudioFormat& o) const {
        return (channels == o.channels && sampleRate == o.sampleRate);
    }
    bool operator!=(const AudioFormat& o) const {
        return !(*this == o);
    }
};

struct AudioFrameInfo {
    int64_t PTS;
    int numSamples; // 1�`�����l��������̃T���v����
    AudioFormat format;
};

struct AudioFrameData : public AudioFrameInfo {
    int codedDataSize;
    uint8_t* codedData;
    int numDecodedSamples;
    int decodedDataSize;
    uint16_t* decodedData;
};

class IVideoParser {
public:
    // �Ƃ肠�����K�v�ȕ�����
    virtual void reset() = 0;

    // PTS, DTS: 90kHz�^�C���X�^���v ��񂪂Ȃ��ꍇ��-1
    virtual bool inputFrame(MemoryChunk frame, std::vector<VideoFrameInfo>& info, int64_t PTS, int64_t DTS) = 0;
};

enum NicoJKType {
    NICOJK_720S = 0,
    NICOJK_720T = 1,
    NICOJK_1080S = 2,
    NICOJK_1080T = 3,
    NICOJK_MAX
};

#include "avisynth.h"
#include "utvideo/utvideo.h"
#include "utvideo/Codec.h"

void DeleteScriptEnvironment(IScriptEnvironment2* env);

typedef std::unique_ptr<IScriptEnvironment2, decltype(&DeleteScriptEnvironment)> ScriptEnvironmentPointer;

ScriptEnvironmentPointer make_unique_ptr(IScriptEnvironment2* env);

// 1�C���X�^���X�͏�������or�ǂݍ��݂̂ǂ��炩��������g���Ȃ�
/*
class LosslessVideoFile : AMTObject {
    struct LosslessFileHeader {
        int magic;
        int version;
        int width;
        int height;
    };

    File file;
    LosslessFileHeader fh;
    std::vector<uint8_t> extra;
    std::vector<int> framesizes;
    std::vector<int64_t> offsets;

    int current;

public:
    LosslessVideoFile(AMTContext& ctx, const tstring& filepath, const tchar* mode);

    void writeHeader(int width, int height, int numframes, const std::vector<uint8_t>& extra);

    void readHeader();

    int getWidth() const { return fh.width; }
    int getHeight() const { return fh.height; }
    int getNumFrames() const { return (int)framesizes.size(); }
    const std::vector<uint8_t>& getExtra() const { return extra; }

    void writeFrame(const uint8_t* data, int len);

    int64_t readFrame(int n, uint8_t* data);
};
*/

void CopyYV12(uint8_t* dst, PVideoFrame& frame, int width, int height);

void CopyYV12(PVideoFrame& dst, uint8_t* frame, int width, int height);

void CopyYV12(uint8_t* dst,
    const uint8_t* srcY, const uint8_t* srcU, const uint8_t* srcV,
    int pitchY, int pitchUV, int width, int height);

void ConcatFiles(const std::vector<tstring>& srcpaths, const tstring& dstpath);

// BOM����UTF8�ŏ�������
void WriteUTF8File(const tstring& filename, const std::string& utf8text);

//void WriteUTF8File(const tstring& filename, const std::u16string& text);

#endif