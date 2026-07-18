#pragma once
#include <vector>
#include "scene.h"
#include "gameData.h"
#include "raylib.h"

struct ParallaxBackground{
    const int layerCount = 6;
    std::vector<Texture2D*> layers;
    std::vector<float> layerSpeeds = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 1.2f };
    float scrollVel = 200.0f;
    std::vector<float> scrollAmount;
    ParallaxBackground();
    ~ParallaxBackground() = default;
    void ParallaxUpdate(float dt);
    void Draw() const;
};

class MenuScene : public Scene{
    public:
        MenuScene();
        Scene* Update(float dt) override;
        void Draw() override;
        int GuiSoundedButton(const Rectangle& bounds, const char* text);
    private:
        ParallaxBackground background;
        Sound *buttonClickSound = nullptr;
        Music *menuMusic = nullptr;
        enum buttons{ CONTINUE, NEW_GAME, NEW_YES, NEW_NO, OPTIONS, EXIT, EXIT_YES, EXIT_NO, VOLUME_GRP, GRAPHICS_GRP, CONTROLS_GRP, BACK, SAVE, KEYBINDINGS_CLOSE } button;
        enum windowBox{ CNFRM_NEW_GAME, CNFRM_EXIT } windowBox;
        enum sliderBar{ MASTER_VOL, VOL, SFX } sliderBar;
        std::vector<bool> buttonPressed;
        std::vector<bool> windowBoxActive;
        std::vector<float> SliderBarValue;
        bool showOptions, vsyncCheckboxValue, fpsDropdownEdit, antiAliasingDropdownEdit, fadeoutActive;
        bool showKeybindings = false;
        void DrawKeybindingsOverlay();
        int opt, fpsValue, antiAliasingValue, cursorVisibilityValue;
        float timer;
};
