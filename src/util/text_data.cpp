#include "text_data.hpp"
#include <glibmm.h>

namespace horizon {

const LutEnumStr<TextData::Font> TextData::font_lut = {
        {"simplex", TextData::Font::SIMPLEX},
        {"complex", TextData::Font::COMPLEX},
        {"complex_italic", TextData::Font::COMPLEX_ITALIC},
        {"complex_small", TextData::Font::COMPLEX_SMALL},
        {"complex_small_italic", TextData::Font::COMPLEX_SMALL_ITALIC},
        {"duplex", TextData::Font::DUPLEX},
        {"triplex", TextData::Font::TRIPLEX},
        {"triplex_italic", TextData::Font::TRIPLEX_ITALIC},
        {"small", TextData::Font::SMALL},
        {"small_italic", TextData::Font::SMALL_ITALIC},
        {"script_simplex", TextData::Font::SCRIPT_SIMPLEX},
        {"script_complex", TextData::Font::SCRIPT_COMPLEX},
};

static const std::vector<unsigned int> font_hershey_plain = {
        199, 214, 217, 233, 219, 197, 234, 216, 221, 222, 228, 225, 211, 224, 210, 220, 200, 201, 202,
        203, 204, 205, 206, 207, 208, 209, 212, 213, 191, 226, 192, 215, 190, 1,   2,   3,   4,   5,
        6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
        25,  26,  193, 84,  194, 85,  86,  87,  101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 195, 223, 196, 88};

static const std::vector<unsigned int> font_hershey_plain_italic = {
        199, 214, 217, 233, 219, 197, 234, 216, 221, 222, 228, 225, 211, 224, 210, 220, 200, 201, 202,
        203, 204, 205, 206, 207, 208, 209, 212, 213, 191, 226, 192, 215, 190, 51,  52,  53,  54,  55,
        56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
        75,  76,  193, 84,  194, 85,  86,  87,  151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
        162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 195, 223, 196, 88};

static const std::vector<unsigned int> font_hershey_complex_small = {
        1199, 1214, 1217, 1275, 1274, 1271, 1272, 1216, 1221, 1222, 1219, 1232, 1211, 1231, 1210, 1220,
        1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1212, 2213, 1241, 1238, 1242, 1215,
        1273, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015,
        1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025, 1026, 1223, 1084, 1224, 1247, 586,
        1249, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113, 1114, 1115,
        1116, 1117, 1118, 1119, 1120, 1121, 1122, 1123, 1124, 1125, 1126, 1225, 1229, 1226, 1246};

static const std::vector<unsigned int> font_hershey_complex_small_italic = {
        1199, 1214, 1217, 1275, 1274, 1271, 1272, 1216, 1221, 1222, 1219, 1232, 1211, 1231, 1210, 1220,
        1200, 1201, 1202, 1203, 1204, 1205, 1206, 1207, 1208, 1209, 1212, 1213, 1241, 1238, 1242, 1215,
        1273, 1051, 1052, 1053, 1054, 1055, 1056, 1057, 1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065,
        1066, 1067, 1068, 1069, 1070, 1071, 1072, 1073, 1074, 1075, 1076, 1223, 1084, 1224, 1247, 586,
        1249, 1151, 1152, 1153, 1154, 1155, 1156, 1157, 1158, 1159, 1160, 1161, 1162, 1163, 1164, 1165,
        1166, 1167, 1168, 1169, 1170, 1171, 1172, 1173, 1174, 1175, 1176, 1225, 1229, 1226, 1246};

static const std::vector<unsigned int> font_hershey_simplex = {
        2198, 714, 717, 733, 719, 697,  734, 716,  721, 722, 728, 725, 711, 724, 710, 720, 700, 701, 702,
        703,  704, 705, 706, 707, 708,  709, 712,  713, 691, 726, 692, 715, 690, 501, 502, 503, 504, 505,
        506,  507, 508, 509, 510, 511,  512, 513,  514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524,
        525,  526, 693, 584, 694, 2247, 586, 2249, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611,
        612,  613, 614, 615, 616, 617,  618, 619,  620, 621, 622, 623, 624, 625, 626, 695, 723, 696, 2246};

static const std::vector<unsigned int> font_hershey_duplex = {
        2198, 2714, 2728, 2732, 2719, 2733, 2718, 2727, 2721, 2722, 2723, 2725, 2711, 2724, 2710, 2720,
        2700, 2701, 2702, 2703, 2704, 2705, 2706, 2707, 2708, 2709, 2712, 2713, 2730, 2726, 2731, 2715,
        2734, 2501, 2502, 2503, 2504, 2505, 2506, 2507, 2508, 2509, 2510, 2511, 2512, 2513, 2514, 2515,
        2516, 2517, 2518, 2519, 2520, 2521, 2522, 2523, 2524, 2525, 2526, 2223, 2084, 2224, 2247, 587,
        2249, 2601, 2602, 2603, 2604, 2605, 2606, 2607, 2608, 2609, 2610, 2611, 2612, 2613, 2614, 2615,
        2616, 2617, 2618, 2619, 2620, 2621, 2622, 2623, 2624, 2625, 2626, 2225, 2229, 2226, 2246};

static const std::vector<unsigned int> font_hershey_complex = {
        2198, 2214, 2217, 2275, 2274, 2271, 2272, 2216, 2221, 2222, 2219, 2232, 2211, 2231, 2210, 2220, 2200, 2201,
        2202, 2203, 2204, 2205, 2206, 2207, 2208, 2209, 2212, 2213, 2241, 2238, 2242, 2215, 2273, 2001, 2002, 2003,
        2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021,
        2022, 2023, 2024, 2025, 2026, 2223, 2084, 2224, 2247, 587,  2249, 2101, 2102, 2103, 2104, 2105, 2106, 2107,
        2108, 2109, 2110, 2111, 2112, 2113, 2114, 2115, 2116, 2117, 2118, 2119, 2120, 2121, 2122, 2123, 2124, 2125,
        2126, 2225, 2229, 2226, 2246, 2801, 2802, 2803, 2804, 2805, 2806, 2807, 2808, 2809, 2810, 2811, 2812, 2813,
        2814, 2815, 2816, 2817, 2818, 2819, 2820, 2821, 2822, 2823, 2824, 2825, 2826, 2827, 2828, 2829, 2830, 2831,
        2832, 2901, 2902, 2903, 2904, 2905, 2906, 2907, 2908, 2909, 2910, 2911, 2912, 2913, 2914, 2915, 2916, 2917,
        2918, 2919, 2920, 2921, 2922, 2923, 2924, 2925, 2926, 2927, 2928, 2929, 2930, 2931, 2932};

static const std::vector<unsigned int> font_hershey_complex_italic = {
        2198, 2764, 2778, 2782, 2769, 2783, 2768, 2777, 2771, 2772, 2219, 2232, 2211, 2231, 2210, 2220,
        2750, 2751, 2752, 2753, 2754, 2755, 2756, 2757, 2758, 2759, 2212, 2213, 2241, 2238, 2242, 2765,
        2273, 2051, 2052, 2053, 2054, 2055, 2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 2064, 2065,
        2066, 2067, 2068, 2069, 2070, 2071, 2072, 2073, 2074, 2075, 2076, 2223, 2084, 2224, 2247, 587,
        2249, 2151, 2152, 2153, 2154, 2155, 2156, 2157, 2158, 2159, 2160, 2161, 2162, 2163, 2164, 2165,
        2166, 2167, 2168, 2169, 2170, 2171, 2172, 2173, 2174, 2175, 2176, 2225, 2229, 2226, 2246};

static const std::vector<unsigned int> font_hershey_triplex = {
        2198, 3214, 3228, 3232, 3219, 3233, 3218, 3227, 3221, 3222, 3223, 3225, 3211, 3224, 3210, 3220,
        3200, 3201, 3202, 3203, 3204, 3205, 3206, 3207, 3208, 3209, 3212, 3213, 3230, 3226, 3231, 3215,
        3234, 3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009, 3010, 3011, 3012, 3013, 2014, 3015,
        3016, 3017, 3018, 3019, 3020, 3021, 3022, 3023, 3024, 3025, 3026, 2223, 2084, 2224, 2247, 587,
        2249, 3101, 3102, 3103, 3104, 3105, 3106, 3107, 3108, 3109, 3110, 3111, 3112, 3113, 3114, 3115,
        3116, 3117, 3118, 3119, 3120, 3121, 3122, 3123, 3124, 3125, 3126, 2225, 2229, 2226, 2246};

static const std::vector<unsigned int> font_hershey_triplex_italic = {
        2198, 3264, 3278, 3282, 3269, 3233, 3268, 3277, 3271, 3272, 3223, 3225, 3261, 3224, 3260, 3270,
        3250, 3251, 3252, 3253, 3254, 3255, 3256, 3257, 3258, 3259, 3262, 3263, 3230, 3226, 3231, 3265,
        3234, 3051, 3052, 3053, 3054, 3055, 3056, 3057, 3058, 3059, 3060, 3061, 3062, 3063, 2064, 3065,
        3066, 3067, 3068, 3069, 3070, 3071, 3072, 3073, 3074, 3075, 3076, 2223, 2084, 2224, 2247, 587,
        2249, 3151, 3152, 3153, 3154, 3155, 3156, 3157, 3158, 3159, 3160, 3161, 3162, 3163, 3164, 3165,
        3166, 3167, 3168, 3169, 3170, 3171, 3172, 3173, 3174, 3175, 3176, 2225, 2229, 2226, 2246};

static const std::vector<unsigned int> font_hershey_script_simplex = {
        2198, 714, 717, 733, 719, 697,  734, 716,  721, 722, 728, 725, 711, 724, 710, 720, 700, 701, 702,
        703,  704, 705, 706, 707, 708,  709, 712,  713, 691, 726, 692, 715, 690, 551, 552, 553, 554, 555,
        556,  557, 558, 559, 560, 561,  562, 563,  564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574,
        575,  576, 693, 584, 694, 2247, 586, 2249, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661,
        662,  663, 664, 665, 666, 667,  668, 669,  670, 671, 672, 673, 674, 675, 676, 695, 723, 696, 2246};

static const std::vector<unsigned int> font_hershey_script_complex = {
        2198, 2764, 2778, 2782, 2769, 2783, 2768, 2777, 2771, 2772, 2219, 2232, 2211, 2231, 2210, 2220,
        2750, 2751, 2752, 2753, 2754, 2755, 2756, 2757, 2758, 2759, 2212, 2213, 2241, 2238, 2242, 2215,
        2273, 2551, 2552, 2553, 2554, 2555, 2556, 2557, 2558, 2559, 2560, 2561, 2562, 2563, 2564, 2565,
        2566, 2567, 2568, 2569, 2570, 2571, 2572, 2573, 2574, 2575, 2576, 2223, 2084, 2224, 2247, 586,
        2249, 2651, 2652, 2653, 2654, 2655, 2656, 2657, 2658, 2659, 2660, 2661, 2662, 2663, 2664, 2665,
        2666, 2667, 2668, 2669, 2670, 2671, 2672, 2673, 2674, 2675, 2676, 2225, 2229, 2226, 2246};


static const std::vector<unsigned int> &get_font_data(TextData::Font font)
{
    switch (font) {
    case TextData::Font::COMPLEX:
        return font_hershey_complex;

    case TextData::Font::COMPLEX_ITALIC:
        return font_hershey_complex_italic;

    case TextData::Font::COMPLEX_SMALL:
        return font_hershey_complex_small;

    case TextData::Font::COMPLEX_SMALL_ITALIC:
        return font_hershey_complex_small_italic;

    case TextData::Font::DUPLEX:
        return font_hershey_duplex;

    case TextData::Font::SCRIPT_COMPLEX:
        return font_hershey_script_complex;

    case TextData::Font::SCRIPT_SIMPLEX:
        return font_hershey_script_simplex;

    case TextData::Font::SIMPLEX:
        return font_hershey_simplex;

    case TextData::Font::SMALL:
        return font_hershey_plain;

    case TextData::Font::SMALL_ITALIC:
        return font_hershey_plain_italic;

    case TextData::Font::TRIPLEX:
        return font_hershey_triplex;

    case TextData::Font::TRIPLEX_ITALIC:
        return font_hershey_triplex_italic;

    default:
        return font_hershey_simplex;
    }
}

static unsigned int codepoint_to_hershey(gunichar c, TextData::Font font)
{
    const auto &font_data = get_font_data(font);
    unsigned int x = c - ' ';
    if (x >= 0 && x < font_data.size()) {
        return font_data.at(x);
    }
    else {
        switch (font) {
        case TextData::Font::SIMPLEX:
            switch (c) {
            case 0xc4: //Ä
                return 3300;
            case 0xd6: //Ö
                return 3301;
            case 0xdc: //Ü
                return 3302;
            case 0xe4: //ä
                return 3303;
            case 0xf6: //ö
                return 3304;
            case 0xfc: //ü
                return 3305;
            }
            break;

        case TextData::Font::COMPLEX_ITALIC:
            switch (c) {
            case 0xc4: //Ä
                return 3310;
            case 0xd6: //Ö
                return 3311;
            case 0xdc: //Ü
                return 3312;
            case 0xe4: //ä
                return 3313;
            case 0xf6: //ö
                return 3314;
            case 0xfc: //ü
                return 3315;
            }
            break;

        case TextData::Font::DUPLEX:
            switch (c) {
            case 0xc4: //Ä
                return 3320;
            case 0xd6: //Ö
                return 3321;
            case 0xdc: //Ü
                return 3322;
            case 0xe4: //ä
                return 3323;
            case 0xf6: //ö
                return 3324;
            case 0xfc: //ü
                return 3325;
            }
            break;

        case TextData::Font::COMPLEX:
            switch (c) {
            case 0xc4: //Ä
                return 3330;
            case 0xd6: //Ö
                return 3331;
            case 0xdc: //Ü
                return 3332;
            case 0xe4: //ä
                return 3333;
            case 0xf6: //ö
                return 3334;
            case 0xfc: //ü
                return 3335;
            }
            break;

        case TextData::Font::TRIPLEX:
            switch (c) {
            case 0xc4: //Ä
                return 3340;
            case 0xd6: //Ö
                return 3341;
            case 0xdc: //Ü
                return 3342;
            case 0xe4: //ä
                return 3343;
            case 0xf6: //ö
                return 3344;
            case 0xfc: //ü
                return 3345;
            }
            break;

        case TextData::Font::TRIPLEX_ITALIC:
            switch (c) {
            case 0xc4: //Ä
                return 3350;
            case 0xd6: //Ö
                return 3351;
            case 0xdc: //Ü
                return 3352;
            case 0xe4: //ä
                return 3353;
            case 0xf6: //ö
                return 3354;
            case 0xfc: //ü
                return 3355;
            }
            break;

        default:;
        }

        switch (c) {
        case 0x3bc: // U+03BC GREEK SMALL LETTER MU
        case 0xb5:  // U+00B5 MICRO SIGN
            return 638;
        case 0xb7: // U+00B7 MIDDLE DOT
            return 729;
        case 0x2126: // U+2126 OHM SIGN
        case 0x3a9:  // U+03A9 GREEK CAPITAL LETTER OMEGA
            return 550;
        case 0x3d1: // U+03D1 GREEK THETA SYMBOL
            return 634;
        case 0xd7: // U+00D7 MULTIPLICATION SIGN
            return 727;
        case 0xa0: // U+00A0 NO-BREAK SPACE
            return codepoint_to_hershey(' ', font);
        case 0x1F384: // U+1F384 CHRISTMAS TREE
            return 871;
        case 0xb0: // U+00B0 DEGREE SIGN
            return 718;
        case 0xb1: // U+00B1 PLUS MINUS SIGN
            return 2233;
        }

        return 870; // palm tree
    }
}

extern const char *hershey_glyphs[];

TextData::TextData(Buffer &buf, const std::string &str, Font font) : lines(buf)
{
    lines.clear();
    Glib::ustring ustr(str);
    int x0 = 0;
    lines.reserve(ustr.size());
    int overbar_start = -1;
    const int bar_y = 24;
    for (const gunichar c : ustr) {
        if (c == '~') {
            if (overbar_start == -1) {
                overbar_start = x0;
                continue;
            }
            else {
                if (overbar_start != x0) {
                    lines.emplace_back(std::piecewise_construct, std::forward_as_tuple(overbar_start, bar_y),
                                       std::forward_as_tuple(x0, bar_y));
                    overbar_start = -1;
                    continue;
                }
                overbar_start = -1;
            }
        }
        const char *s = hershey_glyphs[codepoint_to_hershey(c, font)];
        int left = s[0] - 'R';
        int right = s[1] - 'R';
        int xshift = -left;
        size_t n = 0;
        int x, y, x2, y2;
        s += 2;
        while (1) {
            if (*s == ' ' || !*s) {
                if (!*s++)
                    break;
                n = 0;
            }
            else {
                x = s[0] - 'R';
                y = -(s[1] - 'R') + 9;
                ymax = std::max(ymax, y);
                ymin = std::min(ymin, y);
                xmin = std::min(xmin, x0 + x + xshift);
                xmax = std::max(xmax, x0 + x + xshift);
                s += 2;
                if (n > 0) {
                    lines.emplace_back(std::piecewise_construct, std::forward_as_tuple(x0 + x + xshift, y),
                                       std::forward_as_tuple(x0 + x2 + xshift, y2));
                }
                x2 = x;
                y2 = y;

                n++;
            }
        }
        x0 += right - left;
    }
    xright = x0;
    if (overbar_start != -1)
        lines.emplace_back(std::piecewise_construct, std::forward_as_tuple(overbar_start, bar_y),
                           std::forward_as_tuple(x0, bar_y));
}

std::string TextData::trim(const std::string &s)
{
    char last = 0;
    std::string out;
    for (const auto c : s) {
        if (c == '~') {
            if (last == '~')
                out.append(1, '~');
            else
                ; // eat it
        }
        else {
            out.append(1, c);
        }
        last = c;
    }
    return out;
}

} // namespace horizon
