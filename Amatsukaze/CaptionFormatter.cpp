/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "CaptionFormatter.h"

CaptionASSFormatter::CaptionASSFormatter(AMTContext& ctx)
    : AMTObject(ctx) {
    DefFontSize = 36;

    initialState.x = 0;
    initialState.y = 0;
    initialState.fsx = 1;
    initialState.fsy = 1;
    initialState.spacing = 4;
    initialState.textColor = { 0xFF, 0xFF, 0xFF, 0xFF };
    initialState.backColor = { 0, 0, 0, 0x80 };
    initialState.style = 0;
}

std::string CaptionASSFormatter::generate(const std::vector<OutCaptionLine>& lines) {
    sb.clear();
    PlayResX = lines[0].line->planeW;
    PlayResY = lines[0].line->planeH;
    header();
    for (int i = 0; i < (int)lines.size(); ++i) {
        item(lines[i]);
    }
    return sb.str();
}

void CaptionASSFormatter::header() {
    sb.append("[Script Info]\n")
        .append("ScriptType: v4.00+\n")
        .append("Collisions: Normal\n")
        .append("ScaledBorderAndShadow: Yes\n")
        .append("PlayResX: %d\n", (int)PlayResX)
        .append("PlayResY: %d\n", (int)PlayResY)
        .append("\n")
        .append("[V4+ Styles]\n")
        .append("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, "
            "Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, "
            "BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n")
        .append("Style: Default,Yu Gothic,%d,&H00FFFFFF,&H000000FF,&H00000000,&H7F000000,"
            "1,0,0,0,100,100,4,"
            "0,1,2,2,1,0,0,0,1\n", (int)DefFontSize + 10) // なぜかYu Gothicは+10しないとそのサイズにならなかった
        .append("\n")
        .append("[Events]\n")
        .append("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n");
}

void CaptionASSFormatter::item(const OutCaptionLine& line) {
    if (line.line->formats.size() == 0) {
        return;
    }

    curState = initialState;

    sb.append("Dialogue: 0,");
    time(line.start);
    sb.append(",");
    time(line.end);
    sb.append(",Default,,0000,0000,0000,,");

    // 途中で解像度が変わったとき用
    float scalex = (float)PlayResX / line.line->planeW;
    float scaley = (float)PlayResY / line.line->planeH;

    auto& fmts = line.line->formats;
    int nfrags = (int)fmts.size();
    auto& text = line.line->text;

    for (int i = 0; i < nfrags; ++i) {
        int begin = fmts[i].pos;
        int end = (i + 1 < nfrags) ? fmts[i + 1].pos : (int)text.size();
        auto& fmt = fmts[i];
        auto fragtext = std::u16string(text.begin() + begin, text.begin() + end);

        if (i == 0) {
            char16_t* fragtextDup = _wcsdup(fragtext.c_str());
            int len = StrlenWoLoSurrogate(fragtextDup);
            free(fragtextDup);
            float x = line.line->posX + (fmt.width / len - fmt.charW) * DefFontSize / fmt.charW / 2;
            float y = line.line->posY - (fmt.height - fmt.charH) / 2;
            // posは先頭でしか効果がない模様
            setPos((int)(x * scalex), (int)(y * scaley));
        }

        fragment(scalex, scaley, fragtext, line.line->formats[i]);
    }

    sb.append("\n");
}

void CaptionASSFormatter::fragment(float scalex, float scaley, const std::u16string& text, const CaptionFormat& fmt) {
    char16_t* duped = _wcsdup(text.c_str());
    int len = StrlenWoLoSurrogate(duped);
    free(duped);
    float fsx = fmt.charW / DefFontSize;
    float fsy = fmt.charH / DefFontSize;
    float spacing = (fmt.width / len - fmt.charW) / fsx;

    setColor(fmt.textColor, fmt.backColor);
    setFontSize(fsx * scalex, fsy * scaley);
    setSpacing((int)std::round(spacing * scalex));
    setStyle(fmt.style);

    if (attr.getMC().length > 0) {
        // オーバーライドコード出力
        sb.append("{%s}", attr.str());
        attr.clear();
    }
    sb.append("%s", text);
}

void CaptionASSFormatter::time(double t) {
    double totalSec = t / MPEG_CLOCK_HZ;
    double totalMin = totalSec / 60;
    int h = (int)(totalMin / 60);
    int m = (int)totalMin % 60;
    double sec = totalSec - (int)totalMin * 60;
    sb.append("%d:%02d:%05.2f", h, m, sec);
}

void CaptionASSFormatter::setPos(int x, int y) {
    if (curState.x != x || curState.y != y) {
        attr.append("\\pos(%d,%d)", x, y);
        curState.x = x;
        curState.y = y;
    }
}

void CaptionASSFormatter::setColor(CLUT_DAT_DLL textColor, CLUT_DAT_DLL backColor) {
    if (!(curState.textColor == textColor)) {
        attr.append("\\c&H%02X%02X%02X%02X",
            255 - textColor.ucAlpha, textColor.ucB, textColor.ucG, textColor.ucR);
        curState.textColor = textColor;
    }
    if (!(curState.backColor == backColor)) {
        attr.append("\\4c&H%02X%02X%02X%02X",
            255 - backColor.ucAlpha, backColor.ucB, backColor.ucG, backColor.ucR);
        curState.backColor = backColor;
    }
}

void CaptionASSFormatter::setFontSize(float fsx, float fsy) {
    if (curState.fsx != fsx) {
        attr.append("\\fscx%d", (int)(fsx * 100));
        curState.fsx = fsx;
    }
    if (curState.fsy != fsy) {
        attr.append("\\fscy%d", (int)(fsy * 100));
        curState.fsy = fsy;
    }
}

void CaptionASSFormatter::setSpacing(int spacing) {
    if (curState.spacing != spacing) {
        attr.append("\\fsp%d", spacing);
        curState.spacing = spacing;
    }
}

void CaptionASSFormatter::setStyle(int style) {
    bool cUnderline = (curState.style & CaptionFormat::UNDERLINE) != 0;
    bool nUnderline = (style & CaptionFormat::UNDERLINE) != 0;
    //bool cShadow = (curState.style & CaptionFormat::SHADOW) != 0;
    //bool nShadow = (style & CaptionFormat::SHADOW) != 0;
    bool cBold = (curState.style & CaptionFormat::BOLD) != 0;
    bool nBold = (style & CaptionFormat::BOLD) != 0;
    bool cItalic = (curState.style & CaptionFormat::ITALIC) != 0;
    bool nItalic = (style & CaptionFormat::ITALIC) != 0;
    if (cUnderline != nUnderline) {
        attr.append("\\u%d", (int)nUnderline);
    }
    //if (cShadow != nShadow) {
    //	attr.append(L"\\u%d", (int)nShadow);
    //}
    if (cBold != nBold) {
        attr.append("\\b%d", (int)nBold);
    }
    if (cItalic != nItalic) {
        attr.append("\\i%d", (int)nItalic);
    }
    curState.style = style;
}
CaptionSRTFormatter::CaptionSRTFormatter(AMTContext& ctx)
    : AMTObject(ctx) {}

std::string CaptionSRTFormatter::generate(const std::vector<OutCaptionLine>& lines) {
    sb.clear();
    subIndex = 1;
    prevEnd = -1;
    prevPosY = -1;

    for (int i = 0; i < (int)lines.size(); ++i) {
        item(lines[i]);
    }
    pushLine();
    return sb.str();
}

void CaptionSRTFormatter::pushLine() {
    auto str = linebuf.str();
    if (str.size() > 0) {
        sb.append("%s\n", str);
        linebuf.clear();
    }
}

void CaptionSRTFormatter::time(double t) {
    double totalSec = t / MPEG_CLOCK_HZ;
    double totalMin = totalSec / 60;
    int h = (int)(totalMin / 60);
    int m = (int)totalMin % 60;
    double sec = totalSec - (int)totalMin * 60;
    int s = (int)sec;
    int ss = (int)std::round(((sec - s) * 1000));
    sb.append("%02d:%02d:%02d,%03d", h, m, s, ss);
}

void CaptionSRTFormatter::item(const OutCaptionLine& line) {
    if (line.line->formats.size() == 0) {
        return;
    }

    auto& fmts = line.line->formats;
    int nfrags = (int)fmts.size();
    auto& text = line.line->text;

    for (int i = 0; i < nfrags; ++i) {
        if (fmts[i].sizeMode == CP_STR_SMALL) {
            // 小サイズは出力しない
            continue;
        }
        if (line.end != prevEnd) {
            pushLine();
            // 時間を出力
            sb.append("\n%d\n", subIndex++);
            time(line.start);
            sb.append(" --> ");
            time(line.end);
            sb.append("\n");
            prevEnd = line.end;
            prevPosY = -1;
        }
        if (line.line->posY != prevPosY) {
            // 改行
            pushLine();
            prevPosY = line.line->posY;
        }
        int begin = fmts[i].pos;
        int end = (i + 1 < nfrags) ? fmts[i + 1].pos : (int)text.size();
        auto fragtext = std::u16string(text.begin() + begin, text.begin() + end);
        linebuf.append("%s", fragtext);
    }
}
