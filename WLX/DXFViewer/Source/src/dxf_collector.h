#pragma once

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "drw_interface.h"
#include "libdxfrw.h"

// ============================================================================
// Decode DXF Unicode escape sequences (\U+XXXX) to UTF-8
// ============================================================================

inline std::string decodeDxfUnicode(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        // Check for \U+XXXX pattern
        if (i + 6 <= input.size() && input[i] == '\\' && input[i + 1] == 'U' && input[i + 2] == '+') {
            // Parse 4 hex digits
            char hex[5] = {0};
            bool valid = true;
            for (int j = 0; j < 4; ++j) {
                char c = input[i + 3 + j];
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
                    hex[j] = c;
                else { valid = false; break; }
            }
            if (valid) {
                unsigned int codepoint = (unsigned int)strtoul(hex, nullptr, 16);
                // Encode as UTF-8
                if (codepoint <= 0x7F) {
                    result += (char)codepoint;
                } else if (codepoint <= 0x7FF) {
                    result += (char)(0xC0 | (codepoint >> 6));
                    result += (char)(0x80 | (codepoint & 0x3F));
                } else {
                    result += (char)(0xE0 | (codepoint >> 12));
                    result += (char)(0x80 | ((codepoint >> 6) & 0x3F));
                    result += (char)(0x80 | (codepoint & 0x3F));
                }
                i += 7; // skip \U+XXXX
                continue;
            }
        }
        result += input[i];
        ++i;
    }
    return result;
}

// Count UTF-8 codepoints (not bytes)
inline size_t utf8CharCount(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char ch = (unsigned char)s[i];
        if (ch < 0x80) ++i;
        else if ((ch & 0xE0) == 0xC0) i += 2;
        else if ((ch & 0xF0) == 0xE0) i += 3;
        else i += 4;
        ++count;
    }
    return count;
}

// ============================================================================
// Strip MTEXT formatting codes, keeping only visible text
// Handles: {\fFont|...;text}, \P (paragraph), \S (stacking),
// \Hx;, \Wx;, \Tx;, \Qx;, \Ax;, \Cx;, \px;, \lx;, \Ox;, \ox;, \Kx;, \kx;, etc.
// ============================================================================

inline std::string stripMtextFormatting(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        char c = input[i];
        if (c == '{' || c == '}') {
            // Skip braces (grouping markers)
            ++i;
        } else if (c == '\\') {
            ++i;
            if (i >= input.size()) break;
            char cmd = input[i];
            if (cmd == 'P' || cmd == 'p') {
                // \P = paragraph break → newline
                // \pxi...; = paragraph indent codes → skip to ';'
                if (cmd == 'p') {
                    ++i;
                    while (i < input.size() && input[i] != ';') ++i;
                    if (i < input.size()) ++i; // skip ';'
                } else {
                    result += '\n';
                    ++i;
                }
            } else if (cmd == 'S') {
                // \S stacking: \Snum1/num2; or \Snum1^num2; or \Snum1#num2;
                ++i;
                while (i < input.size() && input[i] != ';') {
                    char sc = input[i];
                    if (sc == '/' || sc == '^' || sc == '#')
                        result += '/';
                    else
                        result += sc;
                    ++i;
                }
                if (i < input.size()) ++i; // skip ';'
            } else if (cmd == 'f' || cmd == 'F') {
                // \f or \F font change: skip to ';'
                ++i;
                while (i < input.size() && input[i] != ';') ++i;
                if (i < input.size()) ++i;
            } else if (cmd == 'H' || cmd == 'W' || cmd == 'T' || cmd == 'Q' ||
                       cmd == 'A' || cmd == 'C' || cmd == 'L' || cmd == 'O' ||
                       cmd == 'o' || cmd == 'K' || cmd == 'k' || cmd == 'l') {
                // \Hx; \Wx; \Tx; \Qx; \Ax; \Cx; \Lx; - property changes → skip to ';'
                ++i;
                while (i < input.size() && input[i] != ';') ++i;
                if (i < input.size()) ++i;
            } else if (cmd == '~') {
                // \~ = non-breaking space
                result += ' ';
                ++i;
            } else if (cmd == '\\') {
                result += '\\';
                ++i;
            } else if (cmd == '{') {
                result += '{';
                ++i;
            } else if (cmd == '}') {
                result += '}';
                ++i;
            } else {
                // Unknown escape: keep character
                result += cmd;
                ++i;
            }
        } else {
            result += c;
            ++i;
        }
    }
    return result;
}

// Convert DRW_LW_Conv::lineWidth enum to mm
inline double lWeightToMm(int lw) {
    // lWeight DXF int values are in 1/100mm; enum maps 0-23 to specific values
    // Special values: -1 = ByLayer, -2 = ByBlock, -3 = Default → treat as 0.25mm default
    static const double lut[] = {
        0.00, 0.05, 0.09, 0.13, 0.15, 0.18, 0.20, 0.25,  // 0-7
        0.30, 0.35, 0.40, 0.50, 0.53, 0.60, 0.70, 0.80,  // 8-15
        0.90, 1.00, 1.06, 1.20, 1.40, 1.58, 2.00, 2.11   // 16-23
    };
    if (lw >= 0 && lw <= 23) return lut[lw];
    return 0.25; // default
}

// ============================================================================
// Data structures for collected DXF entities
// ============================================================================

struct DxfColor {
    unsigned char r, g, b;
};

// AutoCAD Color Index (ACI) to RGB mapping — standard 256-color palette
static const DxfColor g_aciPalette[256] = {
    {0,0,0},       // 0 - BYBLOCK
    {255,0,0},     // 1 - red
    {255,255,0},   // 2 - yellow
    {0,255,0},     // 3 - green
    {0,255,255},   // 4 - cyan
    {0,0,255},     // 5 - blue
    {255,0,255},   // 6 - magenta
    {255,255,255}, // 7 - white/black
    {128,128,128}, // 8
    {192,192,192}, // 9
    {255,0,0},     // 10
    {255,127,127}, // 11
    {165,0,0},     // 12
    {165,82,82},   // 13
    {127,0,0},     // 14
    {127,63,63},   // 15
    {76,0,0},      // 16
    {76,38,38},    // 17
    {38,0,0},      // 18
    {38,19,19},    // 19
    {255,63,0},    // 20
    {255,159,127}, // 21
    {165,41,0},    // 22
    {165,103,82},  // 23
    {127,31,0},    // 24
    {127,79,63},   // 25
    {76,19,0},     // 26
    {76,47,38},    // 27
    {38,9,0},      // 28
    {38,23,19},    // 29
    {255,127,0},   // 30
    {255,191,127}, // 31
    {165,82,0},    // 32
    {165,124,82},  // 33
    {127,63,0},    // 34
    {127,95,63},   // 35
    {76,38,0},     // 36
    {76,57,38},    // 37
    {38,19,0},     // 38
    {38,28,19},    // 39
    {255,191,0},   // 40
    {255,223,127}, // 41
    {165,124,0},   // 42
    {165,145,82},  // 43
    {127,95,0},    // 44
    {127,111,63},  // 45
    {76,57,0},     // 46
    {76,66,38},    // 47
    {38,28,0},     // 48
    {38,33,19},    // 49
    {255,255,0},   // 50
    {255,255,127}, // 51
    {165,165,0},   // 52
    {165,165,82},  // 53
    {127,127,0},   // 54
    {127,127,63},  // 55
    {76,76,0},     // 56
    {76,76,38},    // 57
    {38,38,0},     // 58
    {38,38,19},    // 59
    {191,255,0},   // 60
    {223,255,127}, // 61
    {124,165,0},   // 62
    {145,165,82},  // 63
    {95,127,0},    // 64
    {111,127,63},  // 65
    {57,76,0},     // 66
    {66,76,38},    // 67
    {28,38,0},     // 68
    {33,38,19},    // 69
    {127,255,0},   // 70
    {191,255,127}, // 71
    {82,165,0},    // 72
    {124,165,82},  // 73
    {63,127,0},    // 74
    {95,127,63},   // 75
    {38,76,0},     // 76
    {57,76,38},    // 77
    {19,38,0},     // 78
    {28,38,19},    // 79
    {63,255,0},    // 80
    {159,255,127}, // 81
    {41,165,0},    // 82
    {103,165,82},  // 83
    {31,127,0},    // 84
    {79,127,63},   // 85
    {19,76,0},     // 86
    {47,76,38},    // 87
    {9,38,0},      // 88
    {23,38,19},    // 89
    {0,255,0},     // 90
    {127,255,127}, // 91
    {0,165,0},     // 92
    {82,165,82},   // 93
    {0,127,0},     // 94
    {63,127,63},   // 95
    {0,76,0},      // 96
    {38,76,38},    // 97
    {0,38,0},      // 98
    {19,38,19},    // 99
    {0,255,63},    // 100
    {127,255,159}, // 101
    {0,165,41},    // 102
    {82,165,103},  // 103
    {0,127,31},    // 104
    {63,127,79},   // 105
    {0,76,19},     // 106
    {38,76,47},    // 107
    {0,38,9},      // 108
    {19,38,23},    // 109
    {0,255,127},   // 110
    {127,255,191}, // 111
    {0,165,82},    // 112
    {82,165,124},  // 113
    {0,127,63},    // 114
    {63,127,95},   // 115
    {0,76,38},     // 116
    {38,76,57},    // 117
    {0,38,19},     // 118
    {19,38,28},    // 119
    {0,255,191},   // 120
    {127,255,223}, // 121
    {0,165,124},   // 122
    {82,165,145},  // 123
    {0,127,95},    // 124
    {63,127,111},  // 125
    {0,76,57},     // 126
    {38,76,66},    // 127
    {0,38,28},     // 128
    {19,38,33},    // 129
    {0,255,255},   // 130
    {127,255,255}, // 131
    {0,165,165},   // 132
    {82,165,165},  // 133
    {0,127,127},   // 134
    {63,127,127},  // 135
    {0,76,76},     // 136
    {38,76,76},    // 137
    {0,38,38},     // 138
    {19,38,38},    // 139
    {0,191,255},   // 140
    {127,223,255}, // 141
    {0,124,165},   // 142
    {82,145,165},  // 143
    {0,95,127},    // 144
    {63,111,127},  // 145
    {0,57,76},     // 146
    {38,66,76},    // 147
    {0,28,38},     // 148
    {19,33,38},    // 149
    {0,127,255},   // 150
    {127,191,255}, // 151
    {0,82,165},    // 152
    {82,124,165},  // 153
    {0,63,127},    // 154
    {63,95,127},   // 155
    {0,38,76},     // 156
    {38,57,76},    // 157
    {0,19,38},     // 158
    {19,28,38},    // 159
    {0,63,255},    // 160
    {127,159,255}, // 161
    {0,41,165},    // 162
    {82,103,165},  // 163
    {0,31,127},    // 164
    {63,79,127},   // 165
    {0,19,76},     // 166
    {38,47,76},    // 167
    {0,9,38},      // 168
    {19,23,38},    // 169
    {0,0,255},     // 170
    {127,127,255}, // 171
    {0,0,165},     // 172
    {82,82,165},   // 173
    {0,0,127},     // 174
    {63,63,127},   // 175
    {0,0,76},      // 176
    {38,38,76},    // 177
    {0,0,38},      // 178
    {19,19,38},    // 179
    {63,0,255},    // 180
    {159,127,255}, // 181
    {41,0,165},    // 182
    {103,82,165},  // 183
    {31,0,127},    // 184
    {79,63,127},   // 185
    {19,0,76},     // 186
    {47,38,76},    // 187
    {9,0,38},      // 188
    {23,19,38},    // 189
    {127,0,255},   // 190
    {191,127,255}, // 191
    {82,0,165},    // 192
    {124,82,165},  // 193
    {63,0,127},    // 194
    {95,63,127},   // 195
    {38,0,76},     // 196
    {57,38,76},    // 197
    {19,0,38},     // 198
    {28,19,38},    // 199
    {191,0,255},   // 200
    {223,127,255}, // 201
    {124,0,165},   // 202
    {145,82,165},  // 203
    {95,0,127},    // 204
    {111,63,127},  // 205
    {57,0,76},     // 206
    {66,38,76},    // 207
    {28,0,38},     // 208
    {33,19,38},    // 209
    {255,0,255},   // 210
    {255,127,255}, // 211
    {165,0,165},   // 212
    {165,82,165},  // 213
    {127,0,127},   // 214
    {127,63,127},  // 215
    {76,0,76},     // 216
    {76,38,76},    // 217
    {38,0,38},     // 218
    {38,19,38},    // 219
    {255,0,191},   // 220
    {255,127,223}, // 221
    {165,0,124},   // 222
    {165,82,145},  // 223
    {127,0,95},    // 224
    {127,63,111},  // 225
    {76,0,57},     // 226
    {76,38,66},    // 227
    {38,0,28},     // 228
    {38,19,33},    // 229
    {255,0,127},   // 230
    {255,127,191}, // 231
    {165,0,82},    // 232
    {165,82,124},  // 233
    {127,0,63},    // 234
    {127,63,95},   // 235
    {76,0,38},     // 236
    {76,38,57},    // 237
    {38,0,19},     // 238
    {38,19,28},    // 239
    {255,0,63},    // 240
    {255,127,159}, // 241
    {165,0,41},    // 242
    {165,82,103},  // 243
    {127,0,31},    // 244
    {127,63,79},   // 245
    {76,0,19},     // 246
    {76,38,47},    // 247
    {38,0,9},      // 248
    {38,19,23},    // 249
    {51,51,51},    // 250
    {91,91,91},    // 251
    {132,132,132}, // 252
    {173,173,173}, // 253
    {214,214,214}, // 254
    {255,255,255}, // 255
};

// ============================================================================
// Geometric primitive types for rendering
// ============================================================================

enum class PrimitiveType {
    Line,
    Circle,
    Arc,
    Ellipse,
    LWPolyline,
    Spline,
    Point,
    Text,
    Solid,       // filled triangle/quad (SOLID, TRACE, 3DFACE)
    Hatch,
    Insert,
};

struct Vec2 {
    double x = 0, y = 0;
};

struct Primitive {
    PrimitiveType type;
    DxfColor color = {255, 255, 255};
    double lineWidth = 0;
    std::string layer;
    std::string lineType;     // line type name
    double ltypeScale = 1.0;  // entity line type scale
    std::string textStyle;    // text style name

    // Line
    Vec2 p1, p2;

    // Circle / Arc
    Vec2 center;
    double radius = 0;
    double startAngle = 0; // degrees
    double endAngle = 360; // degrees

    // Ellipse
    Vec2 majorAxis;
    double ratio = 1.0;

    // LWPolyline / Spline
    std::vector<Vec2> points;
    std::vector<double> bulges; // for LWPolyline
    bool closed = false;

    // Text
    std::string text;
    double textHeight = 2.5;
    double textAngle = 0; // degrees
    int textAlignH = 0; // 0=left, 1=center, 2=right, 3=aligned, 4=middle, 5=fit
    int textAlignV = 0; // 0=baseline, 1=bottom, 2=middle, 3=top

    // Solid/3DFace (3-4 corner points stored in points[])

    // Insert
    std::string blockName;
    Vec2 insertionPoint;
    double scaleX = 1, scaleY = 1;
    double insertAngle = 0;
};

// ============================================================================
// Layer info
// ============================================================================

struct LayerInfo {
    std::string name;
    int colorIndex = 7;
    int color24 = -1;     // 24-bit true color (code 420), -1 = not set
    std::string lineType; // layer default line type
    bool on = true;
    bool frozen = false;
};

// ============================================================================
// Line type info
// ============================================================================

struct LineTypeInfo {
    std::string name;
    std::vector<double> pattern; // dash/gap sequence (positive=dash, negative=gap, 0=dot)
    double totalLength = 0;
};

// ============================================================================
// Text style info
// ============================================================================

struct TextStyleInfo {
    std::string name;
    std::string font;       // font file or family name
    double widthFactor = 1.0;
    double oblique = 0;     // oblique angle in degrees
    int fontFamily = 0;     // TTF flags
};

// ============================================================================
// DxfCollector — implements DRW_Interface to collect entities
// ============================================================================

class DxfCollector : public DRW_Interface {
public:
    std::vector<Primitive> primitives;
    std::map<std::string, LayerInfo> layers;
    std::map<std::string, std::vector<Primitive>> blocks;
    std::map<std::string, LineTypeInfo> lineTypes;
    std::map<std::string, TextStyleInfo> textStyles;

    // Header extents (from $EXTMIN/$EXTMAX)
    bool hasHeaderExtents = false;
    double headerMinX = 0, headerMinY = 0;
    double headerMaxX = 0, headerMaxY = 0;

    // Bounding box
    double minX = 1e30, minY = 1e30;
    double maxX = -1e30, maxY = -1e30;

    void expandBounds(double x, double y) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    DxfColor resolveColor(const DRW_Entity& ent) {
        // Entity-level true color (code 420) takes precedence
        if (ent.color24 >= 0) {
            DxfColor c;
            c.r = (unsigned char)((ent.color24 >> 16) & 0xFF);
            c.g = (unsigned char)((ent.color24 >> 8) & 0xFF);
            c.b = (unsigned char)(ent.color24 & 0xFF);
            return c;
        }
        int ci = ent.color;
        if (ci == 256) { // BYLAYER
            auto it = layers.find(ent.layer);
            if (it != layers.end()) {
                // Layer true color overrides index color
                if (it->second.color24 >= 0) {
                    DxfColor c;
                    c.r = (unsigned char)((it->second.color24 >> 16) & 0xFF);
                    c.g = (unsigned char)((it->second.color24 >> 8) & 0xFF);
                    c.b = (unsigned char)(it->second.color24 & 0xFF);
                    return c;
                }
                ci = it->second.colorIndex;
            } else {
                ci = 7;
            }
        }
        if (ci == 0) ci = 7; // BYBLOCK fallback to white
        if (ci < 0) ci = -ci; // negative = layer off, use abs
        if (ci < 1 || ci > 255) ci = 7;
        return g_aciPalette[ci];
    }

    // Currently collecting into a block?
    std::string currentBlock;

    std::vector<Primitive>& target() {
        if (!currentBlock.empty())
            return blocks[currentBlock];
        return primitives;
    }

    // Populate line type and text style from entity
    static void applyEntityProps(Primitive& p, const DRW_Entity& ent) {
        p.lineType = ent.lineType;
        p.ltypeScale = ent.ltypeScale;
    }

    // --- DRW_Interface: Tables ---
    void addHeader(const DRW_Header* data) override {
        if (!data) return;
        auto itMin = data->vars.find("$EXTMIN");
        auto itMax = data->vars.find("$EXTMAX");
        if (itMin != data->vars.end() && itMax != data->vars.end()) {
            DRW_Variant* vMin = itMin->second;
            DRW_Variant* vMax = itMax->second;
            if (vMin->type() == DRW_Variant::COORD && vMax->type() == DRW_Variant::COORD) {
                headerMinX = vMin->content.v->x;
                headerMinY = vMin->content.v->y;
                headerMaxX = vMax->content.v->x;
                headerMaxY = vMax->content.v->y;
                // Only use if extents look valid
                if (headerMaxX > headerMinX && headerMaxY > headerMinY)
                    hasHeaderExtents = true;
            }
        }
    }

    void addLType(const DRW_LType& data) override {
        LineTypeInfo lt;
        lt.name = data.name;
        lt.totalLength = data.length;
        lt.pattern = data.path;
        lineTypes[data.name] = lt;
    }

    void addLayer(const DRW_Layer& data) override {
        LayerInfo li;
        li.name = data.name;
        li.colorIndex = std::abs(data.color);
        li.color24 = data.color24;
        li.lineType = data.lineType;
        li.on = (data.color >= 0); // negative color = layer OFF
        li.frozen = (data.flags & 0x01) != 0;
        layers[data.name] = li;
    }

    void addDimStyle(const DRW_Dimstyle& /*data*/) override {}
    void addVport(const DRW_Vport& /*data*/) override {}

    void addTextStyle(const DRW_Textstyle& data) override {
        TextStyleInfo ts;
        ts.name = data.name;
        ts.font = data.font;
        ts.widthFactor = data.width;
        ts.oblique = data.oblique * 180.0 / M_PI;
        ts.fontFamily = data.fontFamily;
        textStyles[data.name] = ts;
    }
    void addAppId(const DRW_AppId& /*data*/) override {}

    // --- DRW_Interface: Blocks ---
    void addBlock(const DRW_Block& data) override {
        currentBlock = data.name;
        blocks[currentBlock].clear();
    }

    void setBlock(const int /*handle*/) override {}

    void endBlock() override {
        currentBlock.clear();
    }

    // --- DRW_Interface: Entities ---
    void addPoint(const DRW_Point& data) override {
        Primitive p;
        p.type = PrimitiveType::Point;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.center = {data.basePoint.x, data.basePoint.y};
        expandBounds(data.basePoint.x, data.basePoint.y);
        target().push_back(std::move(p));
    }

    void addLine(const DRW_Line& data) override {
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.p1 = {data.basePoint.x, data.basePoint.y};
        p.p2 = {data.secPoint.x, data.secPoint.y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addRay(const DRW_Ray& /*data*/) override {}
    void addXline(const DRW_Xline& /*data*/) override {}

    void addArc(const DRW_Arc& data) override {
        Primitive p;
        p.type = PrimitiveType::Arc;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.center = {data.basePoint.x, data.basePoint.y};
        p.radius = data.radious;
        p.startAngle = data.staangle * 180.0 / M_PI;
        p.endAngle = data.endangle * 180.0 / M_PI;
        expandBounds(p.center.x - p.radius, p.center.y - p.radius);
        expandBounds(p.center.x + p.radius, p.center.y + p.radius);
        target().push_back(std::move(p));
    }

    void addCircle(const DRW_Circle& data) override {
        Primitive p;
        p.type = PrimitiveType::Circle;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.center = {data.basePoint.x, data.basePoint.y};
        p.radius = data.radious;
        expandBounds(p.center.x - p.radius, p.center.y - p.radius);
        expandBounds(p.center.x + p.radius, p.center.y + p.radius);
        target().push_back(std::move(p));
    }

    void addEllipse(const DRW_Ellipse& data) override {
        Primitive p;
        p.type = PrimitiveType::Ellipse;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.center = {data.basePoint.x, data.basePoint.y};
        p.majorAxis = {data.secPoint.x, data.secPoint.y};
        p.ratio = data.ratio;
        p.startAngle = data.staparam * 180.0 / M_PI;
        p.endAngle = data.endparam * 180.0 / M_PI;
        double majorLen = std::sqrt(p.majorAxis.x * p.majorAxis.x + p.majorAxis.y * p.majorAxis.y);
        expandBounds(p.center.x - majorLen, p.center.y - majorLen);
        expandBounds(p.center.x + majorLen, p.center.y + majorLen);
        target().push_back(std::move(p));
    }

    void addLWPolyline(const DRW_LWPolyline& data) override {
        Primitive p;
        p.type = PrimitiveType::LWPolyline;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.closed = (data.flags & 1) != 0;
        for (size_t i = 0; i < data.vertlist.size(); ++i) {
            const auto* v = data.vertlist[i];
            p.points.push_back({v->x, v->y});
            p.bulges.push_back(v->bulge);
            expandBounds(v->x, v->y);
        }
        target().push_back(std::move(p));
    }

    void addPolyline(const DRW_Polyline& data) override {
        Primitive p;
        p.type = PrimitiveType::LWPolyline;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.lineWidth = lWeightToMm(data.lWeight);
        applyEntityProps(p, data);
        p.closed = (data.flags & 1) != 0;
        for (size_t i = 0; i < data.vertlist.size(); ++i) {
            const auto* v = data.vertlist[i];
            p.points.push_back({v->basePoint.x, v->basePoint.y});
            p.bulges.push_back(v->bulge);
            expandBounds(v->basePoint.x, v->basePoint.y);
        }
        target().push_back(std::move(p));
    }

    void addSpline(const DRW_Spline* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Spline;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.lineWidth = lWeightToMm(data->lWeight);
        applyEntityProps(p, *data);
        p.closed = (data->flags & 1) != 0;
        // Use fit points if available, otherwise control points
        if (!data->fitlist.empty()) {
            for (const auto* pt : data->fitlist) {
                p.points.push_back({pt->x, pt->y});
                expandBounds(pt->x, pt->y);
            }
        } else {
            for (const auto* pt : data->controllist) {
                p.points.push_back({pt->x, pt->y});
                expandBounds(pt->x, pt->y);
            }
        }
        target().push_back(std::move(p));
    }

    void addKnot(const DRW_Entity& /*data*/) override {}

    void addInsert(const DRW_Insert& data) override {
        Primitive p;
        p.type = PrimitiveType::Insert;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.blockName = data.name;
        p.insertionPoint = {data.basePoint.x, data.basePoint.y};
        p.scaleX = data.xscale;
        p.scaleY = data.yscale;
        p.insertAngle = data.angle * 180.0 / M_PI;
        expandBounds(data.basePoint.x, data.basePoint.y);
        target().push_back(std::move(p));
    }

    template<typename T>
    void addSolidLike(const T& data) {
        Primitive p;
        p.type = PrimitiveType::Solid;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.points.push_back({data.basePoint.x, data.basePoint.y});
        p.points.push_back({data.secPoint.x, data.secPoint.y});
        p.points.push_back({data.thirdPoint.x, data.thirdPoint.y});
        p.points.push_back({data.fourPoint.x, data.fourPoint.y});
        for (const auto& pt : p.points) expandBounds(pt.x, pt.y);
        target().push_back(std::move(p));
    }

    void addTrace(const DRW_Trace& data) override { addSolidLike(data); }
    void add3dFace(const DRW_3Dface& data) override { addSolidLike(data); }
    void addSolid(const DRW_Solid& data) override { addSolidLike(data); }

    void addMText(const DRW_MText& data) override {
        Primitive p;
        p.type = PrimitiveType::Text;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.center = {data.basePoint.x, data.basePoint.y};
        p.text = stripMtextFormatting(decodeDxfUnicode(data.text));
        p.textHeight = data.height;
        p.textAngle = data.angle; // already in degrees (inherited from DRW_Text)
        p.textStyle = data.style;
        double estWidth = p.textHeight * 0.6 * utf8CharCount(p.text);
        expandBounds(data.basePoint.x, data.basePoint.y);
        expandBounds(data.basePoint.x + estWidth, data.basePoint.y + p.textHeight);
        target().push_back(std::move(p));
    }

    void addText(const DRW_Text& data) override {
        Primitive p;
        p.type = PrimitiveType::Text;
        p.color = resolveColor(data);
        p.layer = data.layer;
        p.textAlignH = data.alignH;
        p.textAlignV = data.alignV;
        // Insertion point: LEFT/BASELINE uses basePoint, others use secPoint
        if (data.alignH == 0 && data.alignV == 0) {
            p.center = {data.basePoint.x, data.basePoint.y};
        } else if (data.alignH == 3 || data.alignH == 5) {
            // ALIGNED/FIT: midpoint of basePoint and secPoint
            p.center = {(data.basePoint.x + data.secPoint.x) / 2.0,
                        (data.basePoint.y + data.secPoint.y) / 2.0};
        } else {
            p.center = {data.secPoint.x, data.secPoint.y};
        }
        p.text = decodeDxfUnicode(data.text);
        p.textHeight = data.height;
        p.textAngle = data.angle; // already in degrees
        p.textStyle = data.style;
        // Approximate text extent for bounds (assume ~0.6 width factor per char)
        double estWidth = p.textHeight * 0.6 * utf8CharCount(p.text);
        expandBounds(p.center.x, p.center.y);
        expandBounds(p.center.x + estWidth, p.center.y + p.textHeight);
        target().push_back(std::move(p));
    }

    void addDimAlign(const DRW_DimAligned* data) override {
        if (!data) return;
        // Render dimension as a line between definition points
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getDef1Point().x, data->getDef1Point().y};
        p.p2 = {data->getDef2Point().x, data->getDef2Point().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimLinear(const DRW_DimLinear* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getDef1Point().x, data->getDef1Point().y};
        p.p2 = {data->getDef2Point().x, data->getDef2Point().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimRadial(const DRW_DimRadial* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getCenterPoint().x, data->getCenterPoint().y};
        p.p2 = {data->getDefPoint().x, data->getDefPoint().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimDiametric(const DRW_DimDiametric* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getDiameter1Point().x, data->getDiameter1Point().y};
        p.p2 = {data->getDiameter2Point().x, data->getDiameter2Point().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimAngular(const DRW_DimAngular* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getFirstLine1().x, data->getFirstLine1().y};
        p.p2 = {data->getFirstLine2().x, data->getFirstLine2().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimAngular3P(const DRW_DimAngular3p* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getFirstLine().x, data->getFirstLine().y};
        p.p2 = {data->getSecondLine().x, data->getSecondLine().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addDimOrdinate(const DRW_DimOrdinate* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::Line;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.p1 = {data->getOriginPoint().x, data->getOriginPoint().y};
        p.p2 = {data->getFirstLine().x, data->getFirstLine().y};
        expandBounds(p.p1.x, p.p1.y);
        expandBounds(p.p2.x, p.p2.y);
        target().push_back(std::move(p));
    }

    void addLeader(const DRW_Leader* data) override {
        if (!data) return;
        Primitive p;
        p.type = PrimitiveType::LWPolyline;
        p.color = resolveColor(*data);
        p.layer = data->layer;
        p.closed = false;
        for (const auto* v : data->vertexlist) {
            p.points.push_back({v->x, v->y});
            p.bulges.push_back(0);
            expandBounds(v->x, v->y);
        }
        target().push_back(std::move(p));
    }

    void addHatch(const DRW_Hatch* data) override {
        if (!data) return;

        if (data->solid) {
            // Solid fill: collect boundary points and create filled polygon
            for (const auto* loop : data->looplist) {
                if (!loop) continue;
                Primitive p;
                p.type = PrimitiveType::Hatch;
                p.color = resolveColor(*data);
                p.layer = data->layer;
                p.closed = true;

                for (const auto* entity : loop->objlist) {
                    if (!entity) continue;
                    if (entity->eType == DRW::LWPOLYLINE) {
                        const auto* pline = static_cast<const DRW_LWPolyline*>(entity);
                        for (const auto* vert : pline->vertlist) {
                            p.points.push_back({vert->x, vert->y});
                            expandBounds(vert->x, vert->y);
                        }
                    } else if (entity->eType == DRW::LINE) {
                        const auto* line = static_cast<const DRW_Line*>(entity);
                        // Add start point of each line segment
                        p.points.push_back({line->basePoint.x, line->basePoint.y});
                        expandBounds(line->basePoint.x, line->basePoint.y);
                    } else if (entity->eType == DRW::ARC) {
                        const auto* arc = static_cast<const DRW_Arc*>(entity);
                        // Approximate arc with line segments
                        double cx = arc->basePoint.x, cy = arc->basePoint.y;
                        double r = arc->radious;
                        double sa = arc->staangle, ea = arc->endangle;
                        if (ea < sa) ea += 2 * M_PI;
                        double sweep = ea - sa;
                        int segs = (std::max)(4, (int)(std::abs(sweep) * r * 2));
                        for (int i = 0; i <= segs; ++i) {
                            double a = sa + sweep * i / segs;
                            double px = cx + r * std::cos(a);
                            double py = cy + r * std::sin(a);
                            p.points.push_back({px, py});
                            expandBounds(px, py);
                        }
                    }
                }
                if (p.points.size() >= 3)
                    target().push_back(std::move(p));
            }
        } else {
            // Pattern hatch: draw boundary edges only
            for (const auto* loop : data->looplist) {
                if (!loop) continue;
                for (const auto* entity : loop->objlist) {
                    if (!entity) continue;
                    if (entity->eType == DRW::LINE) {
                        const auto* line = static_cast<const DRW_Line*>(entity);
                        Primitive p;
                        p.type = PrimitiveType::Line;
                        p.color = resolveColor(*data);
                        p.layer = data->layer;
                        p.p1 = {line->basePoint.x, line->basePoint.y};
                        p.p2 = {line->secPoint.x, line->secPoint.y};
                        expandBounds(p.p1.x, p.p1.y);
                        expandBounds(p.p2.x, p.p2.y);
                        target().push_back(std::move(p));
                    } else if (entity->eType == DRW::ARC) {
                        const auto* arc = static_cast<const DRW_Arc*>(entity);
                        Primitive p;
                        p.type = PrimitiveType::Arc;
                        p.color = resolveColor(*data);
                        p.layer = data->layer;
                        p.center = {arc->basePoint.x, arc->basePoint.y};
                        p.radius = arc->radious;
                        p.startAngle = arc->staangle * 180.0 / M_PI;
                        p.endAngle = arc->endangle * 180.0 / M_PI;
                        expandBounds(p.center.x - p.radius, p.center.y - p.radius);
                        expandBounds(p.center.x + p.radius, p.center.y + p.radius);
                        target().push_back(std::move(p));
                    }
                }
            }
        }
    }

    void addViewport(const DRW_Viewport& /*data*/) override {}
    void addImage(const DRW_Image* /*data*/) override {}
    void linkImage(const DRW_ImageDef* /*data*/) override {}
    void addComment(const char* /*comment*/) override {}

    // --- DRW_Interface: Write methods (unused) ---
    void writeHeader(DRW_Header& /*data*/) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
};
