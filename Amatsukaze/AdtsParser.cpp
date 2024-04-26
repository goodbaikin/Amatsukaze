/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#include "AdtsParser.h"
#include "faad.h"

bool AdtsHeader::parse(uint8_t *data, int length) {
    // �����`�F�b�N
    if (length < 7) return false;

    BitReader reader(MemoryChunk(data, length));
    try {
        uint16_t syncword = reader.read<12>();
        // sync word �s��
        if (syncword != 0xFFF) return false;

        uint8_t ID = reader.read<1>();
        if (ID != 1) return false; // �Œ�
        uint8_t layer = reader.read<2>();
        if (layer != 0) return false; // �Œ�

        protection_absent = reader.read<1>();
        profile = reader.read<2>();
        sampling_frequency_index = reader.read<4>();
        uint8_t private_bit = reader.read<1>();
        channel_configuration = reader.read<3>();
        uint8_t original_copy = reader.read<1>();
        uint8_t home = reader.read<1>();

        uint8_t copyright_identification_bit = reader.read<1>();
        uint8_t copyright_identification_start = reader.read<1>();
        frame_length = reader.read<13>();
        uint16_t adts_buffer_fullness = reader.read<11>();
        number_of_raw_data_blocks_in_frame = reader.read<2>();

        numBytesRead = reader.numReadBytes();

        if (frame_length < numBytesRead) return false; // �w�b�_���Z���̂͂�������
    } catch (const EOFException&) {
        return false;
    } catch (const FormatException&) {
        return false;
    }
    return true;
}

bool AdtsHeader::check() {

    return true;
}

void AdtsHeader::getSamplingRate(int& rate) {
    switch (sampling_frequency_index) {
    case 0: rate = 96000; return;
    case 1: rate = 88200; return;
    case 2: rate = 64000; return;
    case 3: rate = 48000; return;
    case 4: rate = 44100; return;
    case 5: rate = 32000; return;
    case 6: rate = 24000; return;
    case 7: rate = 22050; return;
    case 8: rate = 16000; return;
    case 9: rate = 12000; return;
    case 0xa: rate = 11025; return;
    case 0xb: rate = 8000; return;
    default: return;
    }
}
AdtsParser::AdtsParser(AMTContext&ctx)
    : AMTObject(ctx)
    , hAacDec(NULL)
    , bytesConsumed_(0)
    , lastPTS_(-1)
    , syncOK(false) {
    createChannelsMap();
}
AdtsParser::~AdtsParser() {
    closeDecoder();
}

/* virtual */ void AdtsParser::reset() {
    decodedBuffer.release();
}

/* virtual */ bool AdtsParser::inputFrame(MemoryChunk frame__, std::vector<AudioFrameData>& info, int64_t PTS) {
    info.clear();
    decodedBuffer.clear();

    // codedBuffer�͎���inputFrame���Ă΂��܂Ńf�[�^��ێ�����K�v������̂�
    // inputFrame�̐擪�őO��inputFrame�Ăяo���œǂ񂾃f�[�^������
    codedBuffer.trimHead(bytesConsumed_);

    if (codedBuffer.size() >= (1 << 13)) {
        // �s���ȃf�[�^�������Ə�������Ȃ��f�[�^���i���Ƒ����Ă����̂�
        // �����߂�����̂Ă�
        // �w�b�_��frame_length�t�B�[���h��13bit�Ȃ̂ł���ȏ�f�[�^����������
        // ���S�ɕs���f�[�^
        codedBuffer.clear();
    }

    int prevDataSize = (int)codedBuffer.size();
    codedBuffer.add(frame__);
    MemoryChunk frame = codedBuffer.get();

    if (frame.length < 7) {
        // �f�[�^�s��
        return false;
    }

    if (lastPTS_ == -1 && PTS >= 0) {
        // �ŏ���PTS
        lastPTS_ = PTS;
        PTS = -1;
    }

    int ibytes = 0;
    bytesConsumed_ = 0;
    for (; ibytes < (int)frame.length - 1; ++ibytes) {
        uint16_t syncword = (read16(&frame.data[ibytes]) >> 4);
        if (syncword != 0xFFF) {
            syncOK = false;
        } else {
            uint8_t* ptr = frame.data + ibytes;
            int len = (int)frame.length - ibytes;

            // �w�b�_�[OK���t���[���������̃f�[�^������
            if (header.parse(ptr, len)
                && header.frame_length <= len) {
                // �X�g���[������͂���͖̂ʓ|�Ȃ̂Ńf�R�[�h�����Ⴄ
                if (hAacDec == NULL) {
                    resetDecoder(MemoryChunk(ptr, len));
                }
                NeAACDecFrameInfo frameInfo;
                void* samples = NeAACDecDecode(hAacDec, &frameInfo, ptr, len);
                if (frameInfo.error != 0) {
                    // �t�H�[�}�b�g���ς��ƃG���[��f���̂ŏ��������Ă����P��H�킹��
                    // �ςȎg����������NeroAAC�N�̓X�g���[���̓r����
                    // �t�H�[�}�b�g���ς�邱�Ƃ�z�肵�Ă��Ȃ��񂾂���d���Ȃ�
                    //�ifixed header���ς��Ȃ��Ă��`�����l���\�����ς�邱�Ƃ����邩��ǂ�ł݂Ȃ��ƕ�����Ȃ��j
                    resetDecoder(MemoryChunk(ptr, len));
                    samples = NeAACDecDecode(hAacDec, &frameInfo, ptr, len);
                }
                if (frameInfo.error == 0) {
                    // �_�E���~�b�N�X���Ă���̂�2ch�ɂȂ�͂�
                    int numChannels = frameInfo.num_front_channels +
                        frameInfo.num_back_channels + frameInfo.num_side_channels + frameInfo.num_lfe_channels;

                    if (numChannels != 2) {
                        // �t�H�[�}�b�g���ς��ƃo�O����2ch�ɂł��Ȃ����Ƃ�����̂ŁA���������Ă����P��H�킹��
                        // �ςȎg����������NeroAAC�N�̓X�g���[���̓r����(ry
                        resetDecoder(MemoryChunk(ptr, len));
                        samples = NeAACDecDecode(hAacDec, &frameInfo, ptr, len);

                        numChannels = frameInfo.num_front_channels +
                            frameInfo.num_back_channels + frameInfo.num_side_channels + frameInfo.num_lfe_channels;
                    }

                    if (frameInfo.error != 0 || numChannels != 2) {
                        ctx.incrementCounter(AMT_ERR_DECODE_AUDIO);
                        ctx.warn("�����t���[���𐳂����f�R�[�h�ł��܂���ł���");
                    } else {
                        decodedBuffer.add(MemoryChunk((uint8_t*)samples, frameInfo.samples * 2));

                        AudioFrameData frameData;
                        frameData.numSamples = frameInfo.original_samples / numChannels;
                        frameData.numDecodedSamples = frameInfo.samples / numChannels;
                        frameData.format.channels = getAudioChannels(header, &frameInfo);
                        frameData.format.sampleRate = frameInfo.samplerate;

                        // �X�g���[��������Ȃ� frameInfo.bytesconsumed == header.frame_length �ƂȂ�͂�����
                        // �X�g���[�����s�����Ɠ����ɂȂ�Ȃ����Ƃ�����
                        // ���̏ꍇ�A������ header.frame_length ��D�悷��
                        //�i���̕������̃t���[�����������f�R�[�h�����m�����オ��̂�
                        //  L-SMASH��header.frame_length�����ăt���[�����X�L�b�v���Ă���̂�
                        //  ���ꂪ���ۂ̃t���[�����ƈ�v���Ă��Ȃ��Ɨ�����̂Łj
                        //frameData.codedDataSize = frameInfo.bytesconsumed;
                        frameData.codedDataSize = header.frame_length;

                        // codedBuffer���f�[�^�ւ̃|�C���^�����Ă���̂�
                        // codedBuffer�ɂ͐G��Ȃ��悤�ɒ��ӁI
                        frameData.codedData = ptr;
                        frameData.decodedDataSize = frameInfo.samples * 2;
                        // AutoBuffer�̓������Ċm�ۂ�����̂Ńf�R�[�h�f�[�^�ւ̃|�C���^�͌�œ����

                        // PTS���v�Z
                        int64_t duration = 90000 * frameData.numSamples / frameData.format.sampleRate;
                        if (ibytes < prevDataSize) {
                            // �t���[���̊J�n�����݂̃p�P�b�g�擪���O�������ꍇ
                            // �i�܂�APES�p�P�b�g�̋��E�ƃt���[���̋��E����v���Ȃ������ꍇ�j
                            // ���݂̃p�P�b�g��PTS�͓K�p�ł��Ȃ��̂őO�̃p�P�b�g����̒l������
                            frameData.PTS = lastPTS_;
                            lastPTS_ += duration;
                            // ���݂̃p�P�b�g�����Ȃ���΃t���[�����o�͂ł��Ȃ������̂ŁA�o�͂����t���[���͌��݂̃p�P�b�g�̈ꕔ���܂ނ͂�
                            ASSERT(ibytes + header.frame_length > prevDataSize);
                            // �܂�APTS�́i��������΁j����̃t���[����PTS�ł���
                            if (PTS >= 0) {
                                lastPTS_ = PTS;
                                PTS = -1;
                            }
                        } else {
                            // PES�p�P�b�g�̋��E�ƃt���[���̋��E����v�����ꍇ
                            // ��������PES�p�P�b�g��2�Ԗڈȍ~�̃t���[��
                            if (PTS >= 0) {
                                lastPTS_ = PTS;
                                PTS = -1;
                            }
                            frameData.PTS = lastPTS_;
                            lastPTS_ += duration;
                        }

                        info.push_back(frameData);

                        // �f�[�^��i�߂�
                        //ASSERT(frameInfo.bytesconsumed == header.frame_length);
                        ibytes += header.frame_length - 1;
                        bytesConsumed_ = ibytes + 1;

                        syncOK = true;
                    }
                }
            } else {
                // �w�b�_�s�� or �\���ȃf�[�^���Ȃ�����
                if (syncOK) {
                    // ���O�̃t���[����OK�Ȃ�P�Ɏ��̃p�P�b�g����M����΂�������
                    break;
                }
            }

        }
    }

    // �f�R�[�h�f�[�^�̃|�C���^������
    uint8_t* decodedData = decodedBuffer.ptr();
    for (int i = 0; i < (int)info.size(); ++i) {
        info[i].decodedData = (uint16_t*)decodedData;
        decodedData += info[i].decodedDataSize;
    }
    ASSERT(decodedData - decodedBuffer.ptr() == decodedBuffer.size());

    return info.size() > 0;
}

void AdtsParser::closeDecoder() {
    if (hAacDec != NULL) {
        NeAACDecClose(hAacDec);
        hAacDec = NULL;
    }
}

bool AdtsParser::resetDecoder(MemoryChunk data) {
    closeDecoder();

    hAacDec = NeAACDecOpen();
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAacDec);
    conf->outputFormat = FAAD_FMT_16BIT;
    conf->downMatrix = 1; // WAV�o�͉͂�͗p�Ȃ̂�2ch����Ώ\��
    NeAACDecSetConfiguration(hAacDec, conf);

    unsigned long samplerate;
    unsigned char channels;
    if (NeAACDecInit(hAacDec, data.data, (int)data.length, &samplerate, &channels)) {
        ctx.warn("NeAACDecInit�Ɏ��s");
        return false;
    }
    return true;
}

AUDIO_CHANNELS AdtsParser::getAudioChannels(const AdtsHeader& header, const NeAACDecFrameInfo *frameInfo) {

    if (header.channel_configuration > 0) {
        switch (header.channel_configuration) {
        case 1: return AUDIO_MONO;
        case 2: return AUDIO_STEREO;
        case 3: return AUDIO_30;
        case 4: return AUDIO_31;
        case 5: return AUDIO_32;
        case 6: return AUDIO_32_LFE;
        case 7: return AUDIO_52_LFE; // 4K
        }
    }

    int64_t canonical = channelCanonical(frameInfo->fr_ch_ele, frameInfo->element_id);
    auto it = channelsMap.find(canonical);
    if (it == channelsMap.end()) {
        return AUDIO_NONE;
    }
    return it->second;
}

int64_t AdtsParser::channelCanonical(int numElem, const uint8_t* elems) {
    int64_t canonical = -1;

    // canonical�ɂ������i22.2ch�ł�16�Ȃ̂ŏ\���Ȃ͂��j
    if (numElem > 20) {
        numElem = 20;
    }
    for (int i = 0; i < numElem; ++i) {
        canonical = (canonical << 3) | elems[i];
    }
    return canonical;
}

void AdtsParser::createChannelsMap() {

    struct {
        AUDIO_CHANNELS channels;
        int numElem;
        const uint8_t elems[20];
    } table[] = {
        {
            AUDIO_21,
            2,{ (uint8_t)ID_CPE, (uint8_t)ID_SCE }
        },
        {
            AUDIO_22,
            2,{ (uint8_t)ID_CPE, (uint8_t)ID_CPE }
        },
        {
            AUDIO_2LANG,
            2,{ (uint8_t)ID_SCE, (uint8_t)ID_SCE }
        },
        // �ȉ�4K
        {
            AUDIO_33_LFE,
            5,{ (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_SCE, (uint8_t)ID_LFE }
        },
        {
            AUDIO_2_22_LFE,
            4,{ (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_LFE, (uint8_t)ID_CPE }
        },
        {
            AUDIO_322_LFE,
            5,{ (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_LFE }
        },
        {
            AUDIO_2_32_LFE,
            5,{ (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_LFE, (uint8_t)ID_CPE }
        },
        {
            AUDIO_2_323_2LFE,
            8,{
                (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_CPE,
                (uint8_t)ID_SCE, (uint8_t)ID_LFE, (uint8_t)ID_LFE, (uint8_t)ID_CPE
            }
        },
        {
            AUDIO_333_523_3_2LFE,
            16,{
                (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_CPE,
                (uint8_t)ID_SCE, (uint8_t)ID_LFE, (uint8_t)ID_LFE,
                (uint8_t)ID_SCE, (uint8_t)ID_CPE, (uint8_t)ID_CPE, (uint8_t)ID_SCE, (uint8_t)ID_CPE,
                (uint8_t)ID_SCE, (uint8_t)ID_SCE, (uint8_t)ID_CPE
            }
        }
    };

    channelsMap.clear();
    for (int i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
        int64_t canonical = channelCanonical(table[i].numElem, table[i].elems);
        ASSERT(channelsMap.find(canonical) == channelsMap.end());
        channelsMap[canonical] = table[i].channels;
    }
}
DualMonoSplitter::DualMonoSplitter(AMTContext& ctx)
    : AMTObject(ctx)
    , hAacDec(NULL) {}

DualMonoSplitter::~DualMonoSplitter() {
    closeDecoder();
}

void DualMonoSplitter::inputPacket(MemoryChunk frame) {
    AdtsHeader header;
    if (!header.parse(frame.data, (int)frame.length)) {
        THROW(FormatException, "[DualMonoSplitter] �w�b�_��parse�ł��Ȃ�����");
    }
    // �X�g���[������͂���͖̂ʓ|�Ȃ̂Ńf�R�[�h�����Ⴄ
    if (hAacDec == NULL) {
        resetDecoder(MemoryChunk(frame.data, frame.length));
    }
    NeAACDecFrameInfo frameInfo;
    void* samples = NeAACDecDecode(hAacDec, &frameInfo, frame.data, (int)frame.length);
    if (frameInfo.error != 0) {
        // �����ł͑��v���Ƃ͎v�����ǈꉞ�G���[�΍�͂���Ă���
        resetDecoder(MemoryChunk(frame.data, frame.length));
        samples = NeAACDecDecode(hAacDec, &frameInfo, frame.data, (int)frame.length);
    }
    if (frameInfo.error == 0) {
        if (frameInfo.fr_ch_ele != 2) {
            THROWF(FormatException, "�f���A�����mAAC�̃G�������g���s�� %d != 2", frameInfo.fr_ch_ele);
        }

        for (int i = 0; i < 2; ++i) {
            BitWriter writer(buf);

            int start_bits = frameInfo.element_start[i];
            int end_bits = frameInfo.element_end[i];
            int frame_length = (end_bits - start_bits + 3 + 7) / 8 + 7;

            // �w�b�_
            writer.write<12>(0xFFF); // sync word
            writer.write<1>(1); // ID
            writer.write<2>(0); // layer
            writer.write<1>(1); // protection_absend
            writer.write<2>(header.profile); // profile
            writer.write<4>(header.sampling_frequency_index); // 
            writer.write<1>(0); // private bits
            writer.write<3>(1); // channel_configuration
            writer.write<1>(0); // original_copy
            writer.write<1>(0); // home
            writer.write<1>(0); // copyright_identification_bit
            writer.write<1>(0); // copyright_identification_start
            writer.write<13>(frame_length); // frame_length
            writer.write<11>((1 << 11) - 1); // adts_buffer_fullness(all ones means variable bit rate)
            writer.write<2>(0); // number_of_raw_data_blocks_in_frame

            // SPE�P��
            BitReader reader(frame);
            reader.skip(start_bits);
            int bitpos = start_bits;
            for (; bitpos + 32 <= end_bits; bitpos += 32) {
                writer.write<32>(reader.read<32>());
            }
            int remain = end_bits - bitpos;
            if (remain > 0) {
                writer.writen(reader.readn(remain), remain);
            }
            writer.write<3>(ID_END);
            writer.byteAlign<0>();
            writer.flush();

            if (buf.size() != frame_length) {
                THROW(RuntimeException, "�T�C�Y������Ȃ�");
            }

            OnOutFrame(i, buf.get());
            buf.clear();
        }
    }
}

void DualMonoSplitter::closeDecoder() {
    if (hAacDec != NULL) {
        NeAACDecClose(hAacDec);
        hAacDec = NULL;
    }
}

bool DualMonoSplitter::resetDecoder(MemoryChunk data) {
    closeDecoder();

    hAacDec = NeAACDecOpen();
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAacDec);
    conf->outputFormat = FAAD_FMT_16BIT;
    NeAACDecSetConfiguration(hAacDec, conf);

    unsigned long samplerate;
    unsigned char channels;
    if (NeAACDecInit(hAacDec, data.data, (int)data.length, &samplerate, &channels)) {
        ctx.warn("NeAACDecInit�Ɏ��s");
        return false;
    }
    return true;
}
