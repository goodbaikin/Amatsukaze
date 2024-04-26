/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include <cmath>
#include "H264VideoParser.h"

void H264HRDBitRate::read(BitReader& reader) {
    bit_rate_vlaue_minus1 = reader.readExpGolom();
    cpb_size_value_minus1 = reader.readExpGolom();
    cbr_flag = reader.read<1>();
}

void H264HRDParameters::read(BitReader& reader) {
    cpb_cnt_minus1 = reader.readExpGolom();
    bit_rate_scale = reader.read<4>();
    cpb_size_scale = reader.read<4>();

    bit_rate_value_minus1.clear();
    for (int i = 0; i <= (int)cpb_cnt_minus1; ++i) {
        H264HRDBitRate rate;
        rate.read(reader);
        bit_rate_value_minus1.push_back(rate);
    }

    initial_cpb_removal_delay_length_minus1 = reader.read<5>();
    cpb_removal_delay_length_minus1 = reader.read<5>();
    dpb_output_delay_length_minus1 = reader.read<5>();
    time_offset_length = reader.read<5>();
}

bool H264SequenceParameterSet::parse(uint8_t *data, int length) {
    // 初期値
    chroma_format_idc = 1;
    bit_depth_luma_minus8 = 0;
    bit_depth_chroma_minus8 = 0;
    separate_colour_plane_flag = 0;

    BitReader reader(MemoryChunk(data, length));
    try {
        profile_idc = reader.read<8>();
        for (int i = 0; i < 6; ++i) {
            constraint_set_flag[i] = reader.read<1>();
        }
        reader.skip(2); // reserved_zero_2bits
        level_idc = reader.read<8>();
        seq_parameter_set_id = reader.readExpGolom();

        if (isHighProfile()) {
            chroma_format_idc = reader.readExpGolom();
            if (chroma_format_idc == 3) {
                separate_colour_plane_flag = reader.read<1>();
            }
            bit_depth_luma_minus8 = reader.readExpGolom();
            bit_depth_chroma_minus8 = reader.readExpGolom();
            qpprime_y_zero_transform_bypass_flag = reader.read<1>();
            seq_scaling_matrix_present_flag = reader.read<1>();
            if (seq_scaling_matrix_present_flag) {
                int i_end = (chroma_format_idc != 3) ? 8 : 12;
                for (int i = 0; i < i_end; ++i) {
                    uint8_t flag = reader.read<1>();
                    if (flag) {
                        if (i < 6) {
                            scailng_list(reader, 16);
                        } else {
                            scailng_list(reader, 64);
                        }
                    }
                }
            }
        }

        log2_max_frame_num_minus4 = reader.readExpGolom();
        pic_order_cnt_type = reader.readExpGolom();
        if (pic_order_cnt_type == 0) {
            reader.readExpGolom(); // log2_max_pic_order_cnt_lsb_minus4
        } else if (pic_order_cnt_type == 1) {
            reader.read<1>(); // delta_pic_order_always_zero_flag
            reader.readExpGolomSigned(); // offset_for_non_ref_pic
            reader.readExpGolomSigned(); // offset_for_top_to_bottom_field
            int num_ref_frames_in_pic_order_cnt_cycle = reader.readExpGolom();
            for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
                reader.readExpGolomSigned(); // offset_for_ref_frame[i]
            }
        }

        max_num_ref_frames = reader.readExpGolom();
        gaps_in_frame_num_value_allowed_flag = reader.read<1>();
        pic_width_in_mbs_minus1 = reader.readExpGolom();
        pic_height_in_map_units_minus1 = reader.readExpGolom();
        frame_mbs_only_flag = reader.read<1>();
        if (!frame_mbs_only_flag) {
            mb_adaptive_frame_field_flag = reader.read<1>();
        }
        direct_8x8_inference_flag = reader.read<1>();
        frame_cropping_flag = reader.read<1>();
        if (frame_cropping_flag) {
            frame_crop.left = reader.readExpGolom();
            frame_crop.right = reader.readExpGolom();
            frame_crop.top = reader.readExpGolom();
            frame_crop.bottom = reader.readExpGolom();
        }
        vui_parameters_present_flag = reader.read<1>();
        if (vui_parameters_present_flag) {
            vui_parameters(reader);
        }

        numReadBytes = reader.numReadBytes();
    } catch (const EOFException&) {
        return false;
    } catch (const FormatException&) {
        return false;
    }
    return true;
}
bool H264SequenceParameterSet::check() { return true; }

void H264SequenceParameterSet::getPicutureSize(int& width, int& height) {
    width = (pic_width_in_mbs_minus1 + 1) * 16;
    height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
    if (frame_cropping_flag) {
        int subWidthC, subHeightC;
        getSubC(subWidthC, subHeightC);
        int unitX, unitY;
        if (getChromaArrayType() == 0) {
            unitX = 1;
            unitY = 2 - frame_mbs_only_flag;
        } else {
            unitX = subWidthC;
            unitY = subHeightC * (2 - frame_mbs_only_flag);
        }
        width -= (frame_crop.left + frame_crop.right) * unitX;
        height -= (frame_crop.top + frame_crop.bottom) * unitY;
    }
}

void H264SequenceParameterSet::getSAR(int& width, int& height) {
    if (!vui_parameters_present_flag || !aspect_ratio_info_present_flag) {
        // unspecified (FFMPEGに合わせる）
        width = 0;
        height = 1;
        return;
    }
    if (aspect_ratio_idc == Extended_SAR) {
        width = sar_width;
        height = sar_height;
        return;
    }
    getSARFromIdc(aspect_ratio_idc, width, height);
}

void H264SequenceParameterSet::getFramteRate(int& frameRateNum, int& frameRateDenom, bool& cfr) {
    if (vui_parameters_present_flag) {
        frameRateNum = time_scale / 2;
        frameRateDenom = num_units_in_tick;
        cfr = (fixed_frame_rate_flag != 0);
    }
}

void H264SequenceParameterSet::getColorDesc(uint8_t& colorPrimaries, uint8_t& transferCharacteristics, uint8_t& colorSpace) {
    if (!vui_parameters_present_flag || !colour_description_present_flag) {
        // 2はUNSPECIFIED
        colorPrimaries = 2;
        transferCharacteristics = 2;
        colorSpace = 2;
        return;
    }
    colorPrimaries = colour_primaries;
    transferCharacteristics = transfer_characteristics;
    colorSpace = matrix_coefficients;
}

double H264SequenceParameterSet::getClockTick() {
    if (!vui_parameters_present_flag) {
        // VUI parametersがなかったら再生できないよ
        THROW(FormatException, "VUI parametersがない");
    }
    return (double)num_units_in_tick / time_scale;
}

bool H264SequenceParameterSet::isHighProfile() {
    return (profile_idc == 100 ||
        profile_idc == 110 ||
        profile_idc == 122 ||
        profile_idc == 244 ||
        profile_idc == 44 ||
        profile_idc == 83 ||
        profile_idc == 86);
}

void H264SequenceParameterSet::getSubC(int& width, int& height) {
    switch (chroma_format_idc) {
    case 2:
        width = 2; height = 1; break;
    case 3:
        width = 1; height = 1; break;
    default:
        width = 2; height = 2; break;
    }
}

int H264SequenceParameterSet::getChromaArrayType() {
    if (separate_colour_plane_flag == 0) return chroma_format_idc;
    return 0;
}

void H264SequenceParameterSet::getSARFromIdc(int idc, int& w, int& h) {
    switch (idc) {
    case 1: w = h = 1; break;
    case 2: w = 12; h = 11; break;
    case 3: w = 10; h = 11; break;
    case 4: w = 16; h = 11; break;
    case 5: w = 40; h = 33; break;
    case 6: w = 24; h = 11; break;
    case 7: w = 20; h = 11; break;
    case 8: w = 32; h = 11; break;
    case 9: w = 80; h = 33; break;
    case 10: w = 18; h = 11; break;
    case 11: w = 15; h = 11; break;
    case 12: w = 64; h = 33; break;
    case 13: w = 160; h = 99; break;
    case 14: w = 4; h = 3; break;
    case 15: w = 3; h = 2; break;
    case 16: w = 2; h = 1; break;
    default: w = h = 1; break;
    }
}

void H264SequenceParameterSet::scailng_list(BitReader& reader, int sizeOfScalingList) {
    int lastScale = 8;
    int nextScale = 8;
    for (int j = 0; j < sizeOfScalingList; ++j) {
        if (nextScale != 0) {
            int delta_scale = reader.readExpGolomSigned();
            nextScale = (lastScale + delta_scale + 256) % 256;
        }
        lastScale = (nextScale == 0) ? lastScale : nextScale;
    }
}

void H264SequenceParameterSet::vui_parameters(BitReader& reader) {
    aspect_ratio_info_present_flag = reader.read<1>();
    if (aspect_ratio_info_present_flag) {
        aspect_ratio_idc = reader.read<8>();
        if (aspect_ratio_idc == Extended_SAR) {
            sar_width = reader.read<16>();
            sar_height = reader.read<16>();
        }
    }
    overscan_info_present_flag = reader.read<1>();
    if (overscan_info_present_flag) {
        overscan_appropriate_flag = reader.read<1>();
    }
    video_signal_type_present_flag = reader.read<1>();
    if (video_signal_type_present_flag) {
        video_format = reader.read<3>();
        video_full_range_flag = reader.read<1>();
        colour_description_present_flag = reader.read<1>();
        if (colour_description_present_flag) {
            colour_primaries = reader.read<8>();
            transfer_characteristics = reader.read<8>();
            matrix_coefficients = reader.read<8>();
        }
    }
    chroma_loc_info_present_flag = reader.read<1>();
    if (chroma_loc_info_present_flag) {
        chroma_sample_loc_type_top_field = reader.readExpGolom();
        chroma_sample_loc_type_bottom_field = reader.readExpGolom();
    }
    timing_info_present_flag = reader.read<1>();
    if (timing_info_present_flag) {
        num_units_in_tick = reader.read<32>();
        time_scale = reader.read<32>();
        fixed_frame_rate_flag = reader.read<1>();
    }
    nal_hrd_parameters_present_flag = reader.read<1>();
    if (nal_hrd_parameters_present_flag) {
        nal_hrd_parameters.read(reader);
    }
    vcl_hrd_parameters_present_flag = reader.read<1>();
    if (vcl_hrd_parameters_present_flag) {
        vcl_hrd_parameters.read(reader);
    }
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        low_delay_hrd_flag = reader.read<1>();
    }
    pic_struct_present_flag = reader.read<1>();
    bitstream_restriction_flag = reader.read<1>();
    if (bitstream_restriction_flag) {
        motion_vectors_over_pic_boundaries_flag = reader.read<1>();
        max_bytes_per_pic_denom = reader.readExpGolom();
        max_bits_per_mb_denom = reader.readExpGolom();
        log2_max_mv_length_horizontal = reader.readExpGolom();
        log2_max_mv_length_vertical = reader.readExpGolom();
        num_reorder_frames = reader.readExpGolom();
        max_dec_frame_buffering = reader.readExpGolom();
    }
}

bool H264PicParameterSet::parse(uint8_t *data, int length) {
    BitReader reader(MemoryChunk(data, length));
    try {

        uint32_t pic_parameter_set_id = reader.readExpGolom();
        uint32_t seq_parameter_set_id = reader.readExpGolom();

        numReadBytes = reader.numReadBytes();
    } catch (const EOFException&) {
        return false;
    } catch (const FormatException&) {
        return false;
    }
    return true;
}
bool H264PicParameterSet::check() { return true; }

void H264SuplementaryEnhancementInformation::updateSPS(H264SequenceParameterSet* sps) {
    // Type II Stream にしか対応しないので VCL HRD 関連は省略

    nal_hrd_parameters_present_flag = sps->nal_hrd_parameters_present_flag;
    vcl_hrd_parameters_present_flag = sps->vcl_hrd_parameters_present_flag;

    pic_struct_present_flag = sps->pic_struct_present_flag;

    cpb_cnt_minus1 = 0;
    initial_cpb_removal_delay_length_minus1 = 23;
    cpb_removal_delay_length_minus1 = 23;
    dpb_output_delay_length_minus1 = 23;
    if (nal_hrd_parameters_present_flag) {
        cpb_cnt_minus1 = sps->nal_hrd_parameters.cpb_cnt_minus1;
        initial_cpb_removal_delay_length_minus1 = sps->nal_hrd_parameters.initial_cpb_removal_delay_length_minus1;
        cpb_removal_delay_length_minus1 = sps->nal_hrd_parameters.cpb_removal_delay_length_minus1;
        dpb_output_delay_length_minus1 = sps->nal_hrd_parameters.dpb_output_delay_length_minus1;
    }
}

bool H264SuplementaryEnhancementInformation::parse(uint8_t *data, int length) {
    has_buffering_period = false;
    has_pic_timing = false;
    has_pan_scan_rect = false;
    has_recovery_point = false;

    BitReader reader(MemoryChunk(data, length));
    try {
        while (reader.numReadBytes() < length) {
            int payloadType = readPayloadTypeSize(reader);
            int payloadSize = readPayloadTypeSize(reader);
            switch (payloadType) {
            case 0: // buffering_period
                has_buffering_period = true;
                buffering_period(reader, payloadSize);
                break;
            case 1: // pic_timing
                has_pic_timing = true;
                pic_timing(reader, payloadSize);
                break;
            case 2: // pan_scan_rect
                has_pan_scan_rect = true;
                pan_scan_rect(reader, payloadSize);
                break;
            case 6: // recovery_point
                has_recovery_point = true;
                recovery_point(reader, payloadSize);
                break;
            default: // 他はいらない
                reader.skip(payloadSize * 8);
                break;
            }
        }
        numReadBytes = reader.numReadBytes();
    } catch (const EOFException&) {
        return false;
    } catch (const FormatException&) {
        return false;
    }
    return true;
}

int H264SuplementaryEnhancementInformation::readPayloadTypeSize(BitReader& reader) {
    int ret = 0;
    uint8_t next;
    while ((next = reader.read<8>()) == 0xFF) {
        ret += 255;
    }
    return ret + next;
}

void H264SuplementaryEnhancementInformation::buffering_period(BitReader& reader_orig, int payloadSize) {
    BitReader reader(reader_orig);

    uint32_t seq_parameter_set_id = reader.readExpGolom();
    if (nal_hrd_parameters_present_flag) {
        for (int i = 0; i <= cpb_cnt_minus1; ++i) {
            H264InitialCpbRemoval removal;
            removal.initial_cpb_removal_delay = reader.readn(
                initial_cpb_removal_delay_length_minus1 + 1);
            removal.initial_cpb_removal_delay_offset = reader.readn(
                initial_cpb_removal_delay_length_minus1 + 1);
            nal_hrd.push_back(removal);
        }
    }

    reader_orig.skip(payloadSize * 8);
}

void H264SuplementaryEnhancementInformation::pic_timing(BitReader& reader_orig, int payloadSize) {
    BitReader reader(reader_orig);

    // CpbDpbDelaysPresentFlag
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        cpb_removal_delay = reader.readn(cpb_removal_delay_length_minus1 + 1);
        dpb_output_delay = reader.readn(dpb_output_delay_length_minus1 + 1);
    }
    if (pic_struct_present_flag) {
        pic_struct = reader.read<4>();
        // タイムスタンプは省略
    }

    reader_orig.skip(payloadSize * 8);
}

void H264SuplementaryEnhancementInformation::pan_scan_rect(BitReader& reader_orig, int payloadSize) {
    BitReader reader(reader_orig);

    uint32_t pan_scan_rect_id = reader.readExpGolom();
    pan_scan_rect_cancel_flag = reader.read<1>();
    if (!pan_scan_rect_cancel_flag) {
        pan_scan_cnt_minus1 = reader.readExpGolom();
        for (int i = 0; i <= (int)pan_scan_cnt_minus1; ++i) {
            RECT rect;
            rect.left = reader.readExpGolom();
            rect.right = reader.readExpGolom();
            rect.top = reader.readExpGolom();
            rect.bottom = reader.readExpGolom();
            pan_scan_rect_offset.push_back(rect);
        }
        pan_scan_rect_repetition_period = reader.readExpGolom();
    }

    reader_orig.skip(payloadSize * 8);
}

void H264SuplementaryEnhancementInformation::recovery_point(BitReader& reader_orig, int payloadSize) {
    BitReader reader(reader_orig);

    recovery_frame_cnt = reader.readExpGolom();
    exact_match_flag = reader.read<1>();
    broken_link_flag = reader.read<1>();
    changing_slice_group_idc = reader.read<2>();

    reader_orig.skip(payloadSize * 8);
}

H264VideoParser::H264VideoParser(AMTContext& ctx)
    : AMTObject(ctx)
    , beffering_period_DTS()
    , sps()
    , pps()
    , sei()
    , format() {}

/* virtual */ void H264VideoParser::reset() {
    beffering_period_DTS = -1;
}

/* virtual */ bool H264VideoParser::inputFrame(MemoryChunk frame, std::vector<VideoFrameInfo>& info, int64_t PTS, int64_t DTS) {
    info.clear();

    if (frame.length < 4) {
        // データ不正
        return false;
    }

    storeBuffer(frame);


    int receivedField = 0;
    bool isGopStart = false;
    PICTURE_TYPE picType = PIC_FRAME;
    FRAME_TYPE type = FRAME_NO_INFO;
    int64_t DTS_from_SEI = -1;
    int64_t PTS_from_SEI = -1;
    int64_t next_bp_DTS = beffering_period_DTS;
    int codedDataSize = 0;

    int numNalUnits = (int)nalUnits.size();
    for (int i = 0; i < numNalUnits; ++i) {
        codedDataSize += nalUnits[i].length;
    }

    for (int i = 0; i < numNalUnits; ++i) {
        NalUnit nalUnit = nalUnits[i];
        int payloadLength = nalUnit.length;
        uint8_t* ptr = buffer.ptr() + nalUnit.offset;

        uint8_t nal_ref_idc = bsm(ptr[0], 5, 2);
        uint8_t nal_unit_type = bsm(ptr[0], 0, 5);

        ++ptr; --payloadLength;

        switch (nal_unit_type) {
        case 1: // IDR以外のピクチャのスライス
        case 2: // データ・パーティショニングAで符号化されたスライス
        case 3: // データ・パーティショニングBで符号化されたスライス
        case 4: // データ・パーティショニングCで符号化されたスライス
        case 5: // IDRピクチャのスライス
            break;
        case 6: // SEI
            if (format.isEmpty()) {
                // SPSが来ないと処理できない
                break;
            }
            if (sei.parse(ptr, payloadLength)) {
                if (sei.has_buffering_period) {
                    if (DTS != -1) {
                        // 次のAUで反映される
                        next_bp_DTS = DTS;
                    }
                }
                if (sei.has_pic_timing) {
                    if (receivedField == 0) { // 最初のフィールドのPTSだけ取得できればいい
                        if (beffering_period_DTS != -1) {
                            // 直前のbeffering periodのフレームのDTS + デコードdelay + 出力delay
                            double DTS_delay = sei.cpb_removal_delay * sps.getClockTick();
                            double PTS_delay = sei.dpb_output_delay * sps.getClockTick();
                            DTS_from_SEI = beffering_period_DTS + (int)std::round(DTS_delay * 90000);
                            PTS_from_SEI = beffering_period_DTS + (int)std::round((DTS_delay + PTS_delay) * 90000);

                            // 33bit化
                            DTS_from_SEI &= (int64_t(1) << 33) - 1;
                            PTS_from_SEI &= (int64_t(1) << 33) - 1;

                            if (PTS != -1) {
                                // PESパケットのPTSと一致しているかチェック
                                if (std::abs(PTS - PTS_from_SEI) > 1) {
                                    ctx.incrementCounter(AMT_ERR_H264_PTS_MISMATCH);
                                    ctx.warn("[H264パーサ] PTSが一致しません");
                                }
                            }
                        }
                    }

                    switch (sei.pic_struct) {
                    case 0: // frame
                        picType = PIC_FRAME;
                        receivedField += 2;
                        break;
                    case 7: // frame doubling
                        picType = PIC_FRAME_DOUBLING;
                        receivedField += 2;
                        break;
                    case 8: // frame tripling
                        picType = PIC_FRAME_TRIPLING;
                        receivedField += 2;
                        break;
                    case 1: // top field
                        if (receivedField == 0) {
                            picType = PIC_TFF;
                        }
                        receivedField += 1;
                        break;
                    case 2: // bottom field
                        if (receivedField == 0) {
                            picType = PIC_BFF;
                        }
                        receivedField += 1;
                        break;
                    case 3: // top field, bottom field, in that order
                        picType = PIC_TFF;
                        receivedField += 2;
                        break;
                    case 4: // bottom field, top field, in that order
                        picType = PIC_BFF;
                        receivedField += 2;
                        break;
                    case 5: // top field, bottom field, top field repeated, in that order
                        picType = PIC_TFF_RFF;
                        receivedField += 2;
                        break;
                    case 6: // bottom field, top field, bottom field repeated, in that order
                        picType = PIC_BFF_RFF;
                        receivedField += 2;
                        break;
                    }
                }
                if (sei.has_pan_scan_rect) {
                    if (sei.pan_scan_rect_offset.size() > 0) {
                        auto& rect = sei.pan_scan_rect_offset[0];
                        format.displayWidth = (16 * format.width - rect.left + rect.right) >> 4;
                        format.displayHeight = (16 * format.height - rect.top + rect.bottom) >> 4;
                    }
                }

                if (receivedField > 2) {
                    ctx.incrementCounter(AMT_ERR_H264_UNEXPECTED_FIELD);
                    ctx.warn("フィールド配置が変則的すぎて対応できません");
                    break;
                }

                if (receivedField == 2) {
                    // 1フレーム受信した
                    VideoFrameInfo finfo;
                    finfo.format = format;
                    finfo.isGopStart = isGopStart;
                    finfo.pic = picType;
                    finfo.type = type;
                    finfo.DTS = (DTS != -1) ? DTS : DTS_from_SEI;
                    finfo.PTS = (PTS != -1) ? PTS : PTS_from_SEI;
                    finfo.codedDataSize = codedDataSize;
                    info.push_back(finfo);

                    // 初期化しておく
                    receivedField = 0;
                    isGopStart = false;
                    picType = PIC_FRAME;
                    type = FRAME_NO_INFO;
                    PTS_from_SEI = -1;
                    codedDataSize = 0;

                    // 入力されたPTS,DTSは最初のフレームにしか適用されないので-1にしておく
                    DTS = PTS = -1;
                }
            }
            break;
        case 7: // SPS
            if (sps.parse(ptr, payloadLength)) {
                sei.updateSPS(&sps);

                isGopStart = true;
                format = VideoFormat(); // 初期化
                format.format = VS_H264;
                sps.getPicutureSize(format.width, format.height);
                // 後でpan_scanで修正されるかもしれないがとりあえず同じ値にしておく
                format.displayWidth = format.width;
                format.displayHeight = format.height;
                sps.getSAR(format.sarWidth, format.sarHeight);
                sps.getFramteRate(format.frameRateNum, format.frameRateDenom, format.fixedFrameRate);
                sps.getColorDesc(format.colorPrimaries,
                    format.transferCharacteristics, format.colorSpace);
                format.progressive = (sps.frame_mbs_only_flag != 0);
            }
            break;
        case 8: // PPS
            if (pps.parse(ptr, payloadLength)) {
                //
            }
            break;
        case 9: // AUデリミタ
            frameType(bsm(*ptr, 5, 3), type);
            beffering_period_DTS = next_bp_DTS;
            break;
        case 10: // End of Sequence
        case 11: // End of Stream
        case 12: // Filler Data
        case 13: // SPS拡張
        case 19: // 補助スライス
            break;
        default: // 未定義
            break;
        }
    }

    if (format.isEmpty()) {
        // SPSが来てないのでとりあえずOKにする
        return true;
    }

    return info.size() > 0;
}

void H264VideoParser::frameType(uint8_t primary_pic_type, FRAME_TYPE& type) {
    switch (primary_pic_type) {
    case 0: // I frame
    case 3: // SI frame
    case 5: // I,SI frame
        type = FRAME_I;
        break;
    case 1: // P frame
    case 4: // SP frame
    case 6: // I,SI,P,SP frame ???
        type = FRAME_P;
        break;
    case 2: // B frame
    case 7: // all types ???
        type = FRAME_B;
        break;
    }
}

void H264VideoParser::pushNalUnit(int unitStart, int lastNonZero) {
    if (lastNonZero > 0) {
        NalUnit nal;
        nal.offset = unitStart;
        // rbsp_stop_one_bitを取り除く
        uint8_t& lastByte = buffer.ptr()[lastNonZero - 1];
        if (lastByte == 0x80) {
            // ペイロードはこのバイトにはないので1バイト削る
            --lastNonZero;
        } else {
            lastByte &= lastByte - 1;
        }
        nal.length = lastNonZero - unitStart;
        nalUnits.push_back(nal);
    }
}

void H264VideoParser::storeBuffer(MemoryChunk frame) {
    buffer.clear();
    nalUnits.clear();
    buffer.add(MemoryChunk(frame.data, 2));
    int32_t n3bytes = (frame.data[0] << 8) | frame.data[1];
    int unitStart = 0;
    int lastNonZero = 0;
    int unitStartOrig = 0;
    int lastNonZeroOrig = 0;
    for (int i = 2; i < (int)frame.length; ++i) {
        uint8_t inByte = frame.data[i];
        n3bytes = ((n3bytes & 0xFFFF) << 8) | inByte;
        if (n3bytes == 0x03) {
            // skip one byte
            //PRINTF("Skip one byte (H264 NAL)\n");
        } else {
            buffer.add(inByte);
            int k = (int)buffer.size();

            if (n3bytes == 0x01) {
                // start code prefix
                pushNalUnit(unitStart, lastNonZero);
                unitStart = k;
                unitStartOrig = i;
            }
            if (inByte) {
                lastNonZero = k;
                lastNonZeroOrig = i;
            }
        }
    }
    pushNalUnit(unitStart, lastNonZero);
}
