#include "ColorThemes.h"

ColorThemes::Theme::Theme(AuxComputations::RGBAColor bg, AuxComputations::RGBAColor tx, 
    AuxComputations::RGBAColor bt, AuxComputations::RGBAColor pb)
: bgColor(bg), textColor(tx), buttonColor(bt), playButtonColor(pb){
    primaryIterator = 0;
    secondaryIterator = 0;
    flipPrimaryDirection = false;
    flipSecondaryDirection = false;
}

ColorThemes::Theme::~Theme(){

}

void ColorThemes::Theme::SetPrimaryColorFunction(ColorThemes::ThemeType type, float opt_args){
    switch(type){
        case LightRainbow:
            generateColorPrimary = [](){
                return AuxComputations::RGBAColor {0.8f, 0.8f, 0.8f, 1.0f};
            };
            break;
        case TealWarm:
            generateColorPrimary = [](){
                return AuxComputations::RGBAColor {0.0f, 0.5f, 0.5f, 1.0f};
            };
            break;
        case PurpleBlue:
            generateColorPrimary = [](){
                return AuxComputations::RGBAColor {0.67f, 0.14f, 1.0f, 1.0f};
            };
            break;
        case YellowGrayScale:
            generateColorPrimary = [](){
                return AuxComputations::RGBAColor {1.0f, 1.0f, 0.3f, 1.0f};
            };
            break;
        case NeonDesRainbow:
            generateColorPrimary = [](){
                return AuxComputations::RGBAColor {0.55f, 1.0f, 0.83f, 1.0f};
            };
            break;
        case Default:
        default:
            generateColorPrimary = [this, opt_args](){
                AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
                AuxComputations::HSBtoRGBA(opt_args*this->primaryIterator, 1.0f, 1.0f, color);
                if(this->primaryIterator >= 360) this->primaryIterator %= 360;
                return color;
            };
            break;
    }
}

void ColorThemes::Theme::SetSecondaryColorFunction(ColorThemes::ThemeType type, float opt_args){
    switch(type){
        case LightRainbow:
            generateColorSecondary = [this](){
                AuxComputations::RGBAColor color = {0, 0, 0, 0.4f};
                AuxComputations::HSBtoRGBA(this->secondaryIterator, 1.0, 1.0, color);
                return color;
            };
            break;
        case TealWarm:
            generateColorSecondary = [this](){
                AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
                if(this->secondaryIterator >= 360){
                    flipSecondaryDirection = true;
                }
                if(this->secondaryIterator <= 0){
                    flipSecondaryDirection = false;
                }
                AuxComputations::HSBtoRGBA(0.167f*this->secondaryIterator, 1.0f, 1.0f, color);
                return color;
            };
            break;
        case PurpleBlue:
            generateColorSecondary = [this](){
                AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
                if(this->secondaryIterator >= 2*180){
                    flipSecondaryDirection = true;
                }
                if(this->secondaryIterator <= 0){
                    flipSecondaryDirection = false;
                }
                AuxComputations::HSBtoRGBA(150 + 0.167f*this->secondaryIterator, 1.0f, 1.0f, color);
                return color;
            };
            break;
        case YellowGrayScale:
            generateColorSecondary = [this](){
                if(this->secondaryIterator >= 60){
                    flipSecondaryDirection = true;
                }
                if(this->secondaryIterator <= 0){
                    flipSecondaryDirection = false;
                }
                return AuxComputations::RGBAColor {0.01f*this->secondaryIterator + 0.2f, 
                    0.01f*this->secondaryIterator + 0.2f, 0.01f*this->secondaryIterator + 0.2f, 1.0f};
            };
            break;
        case NeonDesRainbow:
            generateColorSecondary = [this](){
                AuxComputations::RGBAColor color = {0, 0, 0, 0.8f};
                AuxComputations::HSBtoRGBA(this->secondaryIterator, 0.3f, 0.7f, color);
                if(this->secondaryIterator >= 360) this->secondaryIterator %= 360;
                return color;
            };
            break;
        case Default:
        default:
            generateColorSecondary = [this](){
                AuxComputations::RGBAColor color = {0, 0, 0, 0.7f};
                AuxComputations::HSBtoRGBA(this->secondaryIterator, 0.6f, 0.7f, color);
                if(this->secondaryIterator >= 360) this->secondaryIterator %= 360;
                return color;
            };
            break;
    }
}

void ColorThemes::Theme::updatePrimaryIterator(){
    primaryIterator += 1 - 2*flipPrimaryDirection;
}

void ColorThemes::Theme::updateSecondaryIterator(){
    secondaryIterator += 1 - 2*flipSecondaryDirection;
}

void ColorThemes::Theme::resetPrimaryIterator(){
    primaryIterator = 0;
    flipPrimaryDirection = false;
}

void ColorThemes::Theme::resetSecondaryIterator(){
    secondaryIterator = 0;
    flipSecondaryDirection = false;
}

void ColorThemes::InitializeThemes(std::map<ColorThemes::ThemeType, ColorThemes::Theme>& themeTable, float opt_args){
    //initialize default theme
    AuxComputations::RGBAColor bg_default = {0.2f, 0.3f, 0.3f, 1.0f};
    AuxComputations::RGBAColor tx_default = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_default = {0.88f, 0.76f, 0.64f, 1.0f};
    AuxComputations::RGBAColor pb_default = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::Default,
        ColorThemes::Theme(bg_default, tx_default, bt_default, pb_default)});
    auto& defaultTheme = themeTable.at(ColorThemes::ThemeType::Default);
    defaultTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::Default, opt_args);
    defaultTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::Default, opt_args);

    //initialize light rainbow theme
    AuxComputations::RGBAColor bg_lrnb = {0.4f, 0.6f, 0.6f, 1.0f};
    AuxComputations::RGBAColor tx_lrnb = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_lrnb = {0.88f, 0.76f, 0.64f, 1.0f};
    AuxComputations::RGBAColor pb_lrnb = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::LightRainbow,
        ColorThemes::Theme(bg_lrnb, tx_lrnb, bt_lrnb, pb_lrnb)});
    auto& lightRainbowTheme = themeTable.at(ColorThemes::ThemeType::LightRainbow);
    lightRainbowTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::LightRainbow, opt_args);
    lightRainbowTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::LightRainbow, opt_args);

    //initialize teal warm theme
    AuxComputations::RGBAColor bg_tlwrm = {0.17f, 0.0f, 0.44f, 1.0f};
    AuxComputations::RGBAColor tx_tlwrm = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_tlwrm = {0.76f, 0.878f, 0.976f, 1.0f};
    AuxComputations::RGBAColor pb_tlwrm = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::TealWarm,
        ColorThemes::Theme(bg_tlwrm, tx_tlwrm, bt_tlwrm, pb_tlwrm)});
    auto& tealWarmTheme = themeTable.at(ColorThemes::ThemeType::TealWarm);
    tealWarmTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::TealWarm, opt_args);
    tealWarmTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::TealWarm, opt_args);

    //initialize purple blue theme
    AuxComputations::RGBAColor bg_prpbl = {0.0f, 0.176f, 0.016f, 1.0f};
    AuxComputations::RGBAColor tx_prpbl = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_prpbl = {0.76f, 0.878f, 0.976f, 1.0f};
    AuxComputations::RGBAColor pb_prpbl = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::PurpleBlue,
        ColorThemes::Theme(bg_prpbl, tx_prpbl, bt_prpbl, pb_prpbl)});
    auto& purpleBlueTheme = themeTable.at(ColorThemes::ThemeType::PurpleBlue);
    purpleBlueTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::PurpleBlue, opt_args);
    purpleBlueTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::PurpleBlue, opt_args);

    //initialize yellow grayscale theme
    AuxComputations::RGBAColor bg_ylwgsc = {0.05f, 0.05f, 0.05f, 1.0f};
    AuxComputations::RGBAColor tx_ylwgsc = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_ylwgsc = {0.9f, 0.9f, 0.9f, 1.0f};
    AuxComputations::RGBAColor pb_ylwgsc = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::YellowGrayScale,
        ColorThemes::Theme(bg_ylwgsc, tx_ylwgsc, bt_ylwgsc, pb_ylwgsc)});
    auto& yellowGrayscaleTheme = themeTable.at(ColorThemes::ThemeType::YellowGrayScale);
    yellowGrayscaleTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::YellowGrayScale, opt_args);
    yellowGrayscaleTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::YellowGrayScale, opt_args);

    //initialize neon desaturated rainbow theme
    AuxComputations::RGBAColor bg_ndsrnbw = {0.04f, 0.04f, 0.25f, 1.0f};
    AuxComputations::RGBAColor tx_ndsrnbw = {1.0f, 1.0f, 1.0f, 1.0f};
    AuxComputations::RGBAColor bt_ndsrnbw = {0.42f, 0.85f, 0.54f, 1.0f};
    AuxComputations::RGBAColor pb_ndsrnbw = {0.1f, 0.1f, 0.1f, 1.0f};
    themeTable.insert({ColorThemes::ThemeType::NeonDesRainbow,
        ColorThemes::Theme(bg_ndsrnbw, tx_ndsrnbw, bt_ndsrnbw, pb_ndsrnbw)});
    auto& neonDesRainbowTheme = themeTable.at(ColorThemes::ThemeType::NeonDesRainbow);
    neonDesRainbowTheme.SetPrimaryColorFunction(ColorThemes::ThemeType::NeonDesRainbow, opt_args);
    neonDesRainbowTheme.SetSecondaryColorFunction(ColorThemes::ThemeType::NeonDesRainbow, opt_args);
}

void ColorThemes::ApplyThemeToImGuiWindow(const ColorThemes::Theme& t){
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_WindowBg] = t.getBGVec4();
    colors[ImGuiCol_Text] = t.getTextVec4();
    colors[ImGuiCol_Button] = t.getButtonVec4();
    // Continue applying as needed

    // You can also pass theme values to your OpenGL renderer for graph coloring
    // e.g., glUniform4f(shaderUniformLocation, t.graphPrimary.x, t.graphPrimary.y, ...)
}

// int stuff(){
//     if (ImGui::Begin("Theme Selector")) {
//     if (ImGui::Button("Light Theme")) ApplyTheme(themeTable[ThemeType::Light]);
//     if (ImGui::Button("Neon Theme")) ApplyTheme(themeTable[ThemeType::Neon]);
//     // Add more buttons for themes
//     ImGui::End();
// }
// }