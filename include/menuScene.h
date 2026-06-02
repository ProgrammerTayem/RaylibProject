#pragma once
#include <vector>
#include "scene.h"

class MenuScene : public Scene{
    public:
        MenuScene();
        Scene* Update(float dt) override;
        void Draw() override;
    private:
        enum buttons{ PLAY, OPTIONS, EXIT, YES, NO, VOLUME_GRP, GRAPHICS_GRP, CONTROLS_GRP } button;
        enum windowBox{ CNFRM } windowBox;
        enum sliderBar{ MASTER_VOL, VOL, SFX } sliderBar;
        std::vector<bool> buttonPressed;
        std::vector<bool> windowBoxActive;
        std::vector<float> SliderBarValue;
        bool showOptions, vsyncCheckboxValue, fpsDropdownEdit, antiAliasingDropdownEdit, fadeoutActive;
        int opt, fpsValue, antiAliasingValue, cursorVisibilityValue;
        float timer;
};