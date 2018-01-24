#pragma once

namespace horizon {
class BoardLayers {
public:
    static const int L_OUTLINE = 100;
    static const int TOP_COURTYARD = 60;
    static const int TOP_ASSEMBLY = 50;
    static const int TOP_PACKAGE = 40;
    static const int TOP_PASTE = 30;
    static const int TOP_SILKSCREEN = 20;
    static const int TOP_MASK = 10;
    static const int TOP_COPPER = 0;
    static const int IN1_COPPER = -1;
    static const int IN2_COPPER = -2;
    static const int IN3_COPPER = -3;
    static const int IN4_COPPER = -4;
    static const int BOTTOM_COPPER = -100;
    static const int BOTTOM_MASK = -110;
    static const int BOTTOM_SILKSCREEN = -120;
    static const int BOTTOM_PASTE = -130;
    static const int BOTTOM_PACKAGE = -140;
    static const int BOTTOM_ASSEMBLY = -150;
    static const int BOTTOM_COURTYARD = -160;

    static bool is_copper(int l)
    {
        return l <= TOP_COPPER && l >= BOTTOM_COPPER;
    }
};
} // namespace horizon
