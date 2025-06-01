#pragma once

#include <functional>
#include<map>
#include "vendor/imgui/imgui.h"
#include "AuxComputations.h"

namespace ColorThemes{
    enum ThemeType{
        Default,
        LightRainbow,
        TealWarm,
        PurpleBlue,
        YellowGrayScale,
        NeonDesRainbow
    };

    class Theme {
        AuxComputations::RGBAColor bgColor;
        AuxComputations::RGBAColor textColor;
        AuxComputations::RGBAColor buttonColor;
        AuxComputations::RGBAColor playButtonColor;
        int primaryIterator;
        int secondaryIterator;
        bool flipPrimaryDirection;
        bool flipSecondaryDirection;
        public:
            std::function<AuxComputations::RGBAColor()> generateColorPrimary;
            std::function<AuxComputations::RGBAColor()> generateColorSecondary;

        public:
            Theme(AuxComputations::RGBAColor bg, AuxComputations::RGBAColor tx, 
                AuxComputations::RGBAColor bt, AuxComputations::RGBAColor pb);
            ~Theme();
            void SetPrimaryColorFunction(enum ThemeType type, float opt_args);
            void SetSecondaryColorFunction(enum ThemeType type, float opt_args);
            void updatePrimaryIterator();
            void updateSecondaryIterator();
            void resetPrimaryIterator();
            void resetSecondaryIterator();
            inline AuxComputations::RGBAColor getBG() const { return bgColor; };
            inline AuxComputations::RGBAColor getText() const { return textColor; };
            inline AuxComputations::RGBAColor getButton() const { return buttonColor; };
            inline AuxComputations::RGBAColor getPlayButton() const { return playButtonColor; };
            inline ImVec4 getBGVec4() const { return ImVec4(bgColor.r, bgColor.g, bgColor.b, bgColor.a); };
            inline ImVec4 getTextVec4() const { return ImVec4(textColor.r, textColor.g, textColor.b, textColor.a); };
            inline ImVec4 getButtonVec4() const { return ImVec4(buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a); };
            inline ImVec4 getPlayButtonVec4() const { return ImVec4(playButtonColor.r, playButtonColor.g, playButtonColor.b, playButtonColor.a); };
    };

    void InitializeThemes(std::map<ColorThemes::ThemeType, ColorThemes::Theme>& themeTable, float opt_args);

    void ApplyThemeToImGuiWindow(const ColorThemes::Theme& t);
}
