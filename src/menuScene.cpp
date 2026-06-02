#include "menuScene.h"
#include "gameScene.h"
#include "raylib.h"
#include "raygui.h"


MenuScene::MenuScene() : showOptions(false), opt(1), vsyncCheckboxValue(false), fpsValue(1), antiAliasingValue(0), cursorVisibilityValue(0), fpsDropdownEdit(false), antiAliasingDropdownEdit(false), fadeoutActive(false) {
    buttonPressed.resize(buttons::CONTROLS_GRP + 1, false);
    windowBoxActive.resize(windowBox::CNFRM + 1, false);
    SliderBarValue.resize(sliderBar::SFX + 1, 0.0f);
    timer = 0.0f;
}

Scene* MenuScene::Update(float dt){
    if(buttonPressed[buttons::PLAY]) fadeoutActive = true;
    if(fadeoutActive){
        timer += dt;
        if(timer > 2.0f) return new GameScene();
    }
    if(windowBoxActive[windowBox::CNFRM]){
        if(buttonPressed[buttons::YES]){
            return nullptr;
        }
        else if(buttonPressed[buttons::NO]){
            windowBoxActive[windowBox::CNFRM] = false;
        }
    }
    if(buttonPressed[buttons::OPTIONS]){
        showOptions = !showOptions;
    }
    if(buttonPressed[buttons::VOLUME_GRP]) opt = 1;
    else if(buttonPressed[buttons::GRAPHICS_GRP]) opt = 2;
    else if(buttonPressed[buttons::CONTROLS_GRP]) opt = 3;
    return this;
}

void MenuScene::Draw(){
    if(fadeoutActive) GuiLock();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    //ClearBackground(WHITE);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    char windowLabelText[128] = "SURE TO EXIT?";
    buttonPressed[buttons::PLAY] = GuiButton((Rectangle){ 248, 360, 232, 64 }, "PLAY");
    buttonPressed[buttons::OPTIONS] = GuiButton((Rectangle){ 248, 456, 232, 64 }, "OPTIONS"); 
    buttonPressed[buttons::EXIT] = GuiButton((Rectangle){ 248, 560, 232, 64 }, "EXIT"); 
    if (windowBoxActive[windowBox::CNFRM] || buttonPressed[buttons::EXIT]) {
        //DrawRectangle(824, 280, 456, 232, RED);
        windowBoxActive[windowBox::CNFRM] = !GuiWindowBox((Rectangle){ 824, 280, 456, 232 }, "CONFIRM");
        GuiLabel((Rectangle){ 970, 340, 156, 60 }, windowLabelText);
        buttonPressed[buttons::YES] = GuiButton((Rectangle){ 864, 424, 144, 48 }, "YES");
        buttonPressed[buttons::NO] = GuiButton((Rectangle){ 1096, 424, 152, 48 }, "NO"); 
    }
    else if(showOptions){
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        //GuiGroupBox((Rectangle){ 536, 200, 936, 520 }, NULL);
        GuiPanel((Rectangle){ 536, 200, 936, 520 }, NULL);
        if(opt == 1){
            GuiSliderBar((Rectangle){ 760, 304, 536, 24 }, "MASTER VOLUME", NULL, &SliderBarValue[sliderBar::MASTER_VOL], 0, 100);
            //GuiLine((Rectangle){ 536, 342, 936, 2 }, nullptr);
            GuiSliderBar((Rectangle){ 760, 360, 536, 24 }, "MUSIC", NULL, &SliderBarValue[sliderBar::VOL], 0, 100);
            //GuiLine((Rectangle){ 536, 398, 936, 2 }, nullptr);
            GuiSliderBar((Rectangle){ 760, 416, 536, 24 }, "SFX", NULL, &SliderBarValue[sliderBar::SFX], 0, 100);
            GuiLock();
            buttonPressed[buttons::VOLUME_GRP] = GuiButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
            GuiUnlock();
            buttonPressed[buttons::GRAPHICS_GRP] = GuiButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
            buttonPressed[buttons::CONTROLS_GRP] = GuiButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
        }
        if(opt == 2){
            GuiCheckBox((Rectangle){ 760, 304, 24, 24 }, "USE VSYNC", &vsyncCheckboxValue);
            GuiLabel((Rectangle){ 695, 344, 216, 32 }, "FPS LIMIT");
            if(vsyncCheckboxValue){
                GuiLock();
            }
            if(GuiDropdownBox((Rectangle) { 914, 344, 160, 32, }, "120 FPS;60 FPS;30 FPS", &fpsValue, fpsDropdownEdit)) fpsDropdownEdit = !fpsDropdownEdit;
            if(vsyncCheckboxValue){
                GuiUnlock();
            }
            GuiLabel((Rectangle){ 715, 495, 216, 32 }, "ANTI-ALIASING");
            if(GuiDropdownBox((Rectangle) { 914, 495, 160, 32, }, "NONE; MSAA; FXAA", &antiAliasingValue, antiAliasingDropdownEdit)) antiAliasingDropdownEdit = !antiAliasingDropdownEdit;
            
            buttonPressed[buttons::VOLUME_GRP] = GuiButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
            GuiLock();
            buttonPressed[buttons::GRAPHICS_GRP] = GuiButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
            GuiUnlock();
            buttonPressed[buttons::CONTROLS_GRP] = GuiButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
        }
        if(opt == 3){
                GuiLabel((Rectangle){ 648, 304, 168, 32 }, "CURSOR VISIBILITY");
                GuiToggleGroup((Rectangle){ 824, 304, 128, 32 }, "ON TOGGLE;ON HOLD", &cursorVisibilityValue);
                GuiButton((Rectangle) { 648, 360, 200, 32 }, "KEYBINDINGS");
                buttonPressed[buttons::VOLUME_GRP] = GuiButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
                buttonPressed[buttons::GRAPHICS_GRP] = GuiButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
                GuiLock();
                buttonPressed[buttons::CONTROLS_GRP] = GuiButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
                GuiUnlock();
        }
    }
    if(fadeoutActive){
        GuiUnlock();
        float alpha = (timer / 2.0f);
        if(alpha < 0) alpha = 0;
        if(alpha > 1) alpha = 1;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
    }
}
