#include "menuScene.h"
#include "gameScene.h"
#include "gameData.h"
#include "raylib.h"
#include "raygui.h"
#include "Loader.h"
#include <fstream>

static const char* SETTINGS_PATH = "data/system/settings.json";

static void LoadGraphicsSettings(int& vsync, int& fpsIndex){
    vsync = 0;
    fpsIndex = 1;
    std::ifstream file(ResolveDataPath(SETTINGS_PATH));
    if(!file.is_open()) return;
    try{
        json j;
        file >> j;
        if(j.contains("vsync")) vsync = j["vsync"].get<int>() ? 1 : 0;
        if(j.contains("fpsIndex")) fpsIndex = j["fpsIndex"].get<int>();
    }
    catch(...){}
}

static void SaveGraphicsSettings(int vsync, int fpsIndex){
    json j = {
        {"vsync", vsync ? 1 : 0},
        {"fpsIndex", fpsIndex}
    };
    std::ofstream file(ResolveDataPath(SETTINGS_PATH));
    if(file.is_open()) file << j.dump(4);
}

ParallaxBackground::ParallaxBackground(){
    scrollAmount.resize(layerCount);
    std::fill(scrollAmount.begin(), scrollAmount.end(), 0.0f);
    for(int i = 0; i < layerCount; i++){
        layers.push_back(gameData.res.GetTexture("city_bg_" + std::to_string(i + 1)));
    }
}

void ParallaxBackground::ParallaxUpdate(float dt){
    for(int i = 0; i < layerCount; i++){
        scrollAmount[i] -= layerSpeeds[i] * scrollVel * dt;
        if(layers[i] && scrollAmount[i] <= -layers[i]->width) scrollAmount[i] = 0;
    }
}

void ParallaxBackground::Draw() const{
    for(int i = 0; i < layerCount; i++){
        if(!layers[i]) continue;
        Rectangle src = {
            .x = scrollAmount[i],
            .y = 0,
            .width = layers[i]->width * 2.5f,
            .height = layers[i]->height
        };
        Rectangle dst = {
            .x = 0,
            .y = 0,
            .width = (float)GetScreenWidth(),
            .height = (float)GetScreenHeight()
        };
        DrawTexturePro(*layers[i], src, dst, (Vector2) {0.0f, 0.0f}, 0.0f, WHITE);
    }
}

int MenuScene::GuiSoundedButton(const Rectangle& bounds, const char* text){
    int result = GuiButton(bounds, text);
    if(result && buttonClickSound){
        PlaySound(*buttonClickSound);
    }
    return result;
}

MenuScene::MenuScene() : showOptions(false), opt(1), vsyncCheckboxValue(false), fpsValue(1), antiAliasingValue(0), cursorVisibilityValue(0), fpsDropdownEdit(false), antiAliasingDropdownEdit(false), fadeoutActive(false) {
    buttonPressed.resize(buttons::GRAPHICS_NO + 1, false);
    windowBoxActive.resize(windowBox::CNFRM_GRAPHICS + 1, false);
    SliderBarValue.resize(sliderBar::SFX + 1, 0.0f);
    SliderBarValue = {100.0f, 75.0f, 75.0f};
    timer = 0.0f;
    LoadGraphicsSettings(savedVsyncValue, savedFpsValue);
    vsyncCheckboxValue = savedVsyncValue ? true : false;
    fpsValue = savedFpsValue;
    buttonClickSound = gameData.res.GetSfx("button_click");
    menuMusic = gameData.res.GetMusic("menu_music");
    if(menuMusic) PlayMusicStream(*menuMusic);
    menuMusic->looping = true;
}

Scene* MenuScene::Update(float dt){
    background.ParallaxUpdate(dt);
    if(menuMusic) UpdateMusicStream(*menuMusic);
    float masterVol = SliderBarValue[sliderBar::MASTER_VOL] / 100.0f, 
          musicVol = SliderBarValue[sliderBar::VOL] / 100.0f, 
          sfxVol = SliderBarValue[sliderBar::SFX] / 100.0f;

    if(menuMusic) SetMusicVolume(*menuMusic, masterVol * musicVol);
    if(buttonClickSound) SetSoundVolume(*buttonClickSound, masterVol * sfxVol);
    if(showKeybindings){
        if(buttonPressed[buttons::KEYBINDINGS_CLOSE] || IsKeyPressed(KEY_ESCAPE)) showKeybindings = false;
    }
    if(buttonPressed[buttons::CONTINUE]){
        fadeoutActive = true;
    }
    else if(buttonPressed[buttons::NEW_YES]){
        gameData.DeleteSave();
        fadeoutActive = true;
    }
    if(fadeoutActive){
        timer += dt;
        if(timer > 2.0f){
            if(menuMusic) StopMusicStream(*menuMusic);
            return new GameScene();
        }
    }
    if(windowBoxActive[windowBox::CNFRM_EXIT]){
        if(buttonPressed[buttons::EXIT_YES]){
            return nullptr;
        }
        else if(buttonPressed[buttons::EXIT_NO]){
            windowBoxActive[windowBox::CNFRM_EXIT] = false;
        }
    }
    if(windowBoxActive[windowBox::CNFRM_GRAPHICS]){
        if(buttonPressed[buttons::GRAPHICS_YES]){
            SaveGraphicsSettings(vsyncCheckboxValue, fpsValue);
            return nullptr;
        }
        else if(buttonPressed[buttons::GRAPHICS_NO]){
            windowBoxActive[windowBox::CNFRM_GRAPHICS] = false;
        }
    }
    if(buttonPressed[buttons::BACK]){
        showOptions = false;
        opt = 1;
    }
    if(buttonPressed[buttons::OPTIONS]){
        showOptions = true;
        windowBoxActive[windowBox::CNFRM_EXIT] = false;
        windowBoxActive[windowBox::CNFRM_NEW_GAME] = false;
    }
    if(buttonPressed[buttons::VOLUME_GRP]) opt = 1;
    else if(buttonPressed[buttons::GRAPHICS_GRP]) opt = 2;
    else if(buttonPressed[buttons::CONTROLS_GRP]) opt = 3;
    if(buttonPressed[buttons::SAVE] && opt == 2){
        bool fpsChanged = (fpsValue != savedFpsValue);
        bool vsyncChanged = (vsyncCheckboxValue != (savedVsyncValue ? true : false));
        if(fpsChanged || vsyncChanged){
            windowBoxActive[windowBox::CNFRM_GRAPHICS] = true;
        }
    }
    return this;
}

void MenuScene::Draw(){
    //ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    background.Draw();
    if(fadeoutActive) GuiLock();
    //ClearBackground(WHITE);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 120);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(DARKBROWN));
    GuiLabel((Rectangle){ 0, 140, (float)GetScreenWidth(), 160 }, "Venture");
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    buttonPressed[buttons::CONTINUE] = GuiSoundedButton((Rectangle){ 248, 360, 232, 64 }, "CONTINUE");
    buttonPressed[buttons::NEW_GAME] = GuiSoundedButton((Rectangle){ 248, 456, 232, 64 }, "NEW GAME");
    buttonPressed[buttons::OPTIONS] = GuiSoundedButton((Rectangle){ 248, 560, 232, 64 }, "OPTIONS"); 
    buttonPressed[buttons::EXIT] = GuiSoundedButton((Rectangle){ 248, 656, 232, 64 }, "EXIT"); 
    if ((windowBoxActive[windowBox::CNFRM_EXIT] || buttonPressed[buttons::EXIT]) && !showOptions) {
        //DrawRectangle(824, 280, 456, 232, RED);
        char windowLabelText[128] = "SURE TO EXIT?";
        windowBoxActive[windowBox::CNFRM_EXIT] = !GuiWindowBox((Rectangle){ 824, 280, 456, 232 }, "CONFIRM");
        GuiLabel((Rectangle){ 970, 340, 156, 60 }, windowLabelText);
        buttonPressed[buttons::EXIT_YES] = GuiSoundedButton((Rectangle){ 864, 424, 144, 48 }, "YES");
        buttonPressed[buttons::EXIT_NO] = GuiSoundedButton((Rectangle){ 1096, 424, 152, 48 }, "NO"); 
    }
    if ((windowBoxActive[windowBox::CNFRM_NEW_GAME] || buttonPressed[buttons::NEW_GAME]) && !showOptions) {
        //DrawRectangle(824, 280, 456, 232, RED);
        char windowLabelText[128] = "THIS WILL OVERWRITE YOUR PREVIOUS SAVE\nPROCEED?";
        windowBoxActive[windowBox::CNFRM_NEW_GAME] = !GuiWindowBox((Rectangle){ 824, 280, 456, 232 }, "CONFIRM");
        GuiLabel((Rectangle){ 860, 340, 390, 60 }, windowLabelText);
        buttonPressed[buttons::NEW_YES] = GuiSoundedButton((Rectangle){ 864, 424, 144, 48 }, "YES");
        buttonPressed[buttons::NEW_NO] = GuiSoundedButton((Rectangle){ 1096, 424, 152, 48 }, "NO"); 
    }
    else if(showOptions){
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        //GuiGroupBox((Rectangle){ 536, 200, 936, 520 }, NULL);
        if(showKeybindings) GuiLock();
        GuiPanel((Rectangle){ 536, 200, 936, 520 }, NULL);
        if(opt == 1){
            GuiSliderBar((Rectangle){ 760, 304, 536, 24 }, "MASTER VOLUME", NULL, &SliderBarValue[sliderBar::MASTER_VOL], 0, 100);
            //GuiLine((Rectangle){ 536, 342, 936, 2 }, nullptr);
            GuiSliderBar((Rectangle){ 760, 360, 536, 24 }, "MUSIC", NULL, &SliderBarValue[sliderBar::VOL], 0, 100);
            //GuiLine((Rectangle){ 536, 398, 936, 2 }, nullptr);
            GuiSliderBar((Rectangle){ 760, 416, 536, 24 }, "SFX", NULL, &SliderBarValue[sliderBar::SFX], 0, 100);
            GuiLock();
            buttonPressed[buttons::VOLUME_GRP] = GuiSoundedButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
            GuiUnlock();
            buttonPressed[buttons::GRAPHICS_GRP] = GuiSoundedButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
            buttonPressed[buttons::CONTROLS_GRP] = GuiSoundedButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
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
            
            buttonPressed[buttons::VOLUME_GRP] = GuiSoundedButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
            GuiLock();
            buttonPressed[buttons::GRAPHICS_GRP] = GuiSoundedButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
            GuiUnlock();
            buttonPressed[buttons::CONTROLS_GRP] = GuiSoundedButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
        }
        if(opt == 3){
                GuiLabel((Rectangle){ 648, 304, 168, 32 }, "CURSOR VISIBILITY");
                GuiToggleGroup((Rectangle){ 824, 304, 128, 32 }, "ON TOGGLE;ON HOLD", &cursorVisibilityValue);
                if(GuiSoundedButton((Rectangle) { 648, 360, 200, 32 }, "KEYBINDINGS")) showKeybindings = true;
                buttonPressed[buttons::VOLUME_GRP] = GuiSoundedButton((Rectangle){ 536, 168, 312, 32 }, "VOLUME");
                buttonPressed[buttons::GRAPHICS_GRP] = GuiSoundedButton((Rectangle){ 848, 168, 312, 32 }, "GRAPHICS");
                GuiLock();
                buttonPressed[buttons::CONTROLS_GRP] = GuiSoundedButton((Rectangle){ 1160, 168, 312, 32 }, "CONTROLS");
                GuiUnlock();
        }
        
        buttonPressed[buttons::BACK] = GuiSoundedButton((Rectangle){ 1322, 721, 150, 55 }, "BACK");
        GuiDrawIcon(ICON_ARROW_LEFT_FILL, 1330, 730, 2, WHITE);
        if(opt == 2 || opt == 3){
            buttonPressed[buttons::SAVE] = GuiSoundedButton((Rectangle){ 1172, 721, 150, 55}, "SAVE");
            GuiDrawIcon(ICON_FILE_SAVE_CLASSIC, 1180, 730, 2, WHITE);
        }
        if(showKeybindings){
            GuiUnlock();
            DrawKeybindingsOverlay();
        }
    }
    if(windowBoxActive[windowBox::CNFRM_GRAPHICS]){
        if(showOptions){
            GuiLock();
            GuiWindowBox((Rectangle){ 824, 280, 456, 232 }, "CONFIRM");
            GuiUnlock();
        }
        else{
            windowBoxActive[windowBox::CNFRM_GRAPHICS] = !GuiWindowBox((Rectangle){ 824, 280, 456, 232 }, "CONFIRM");
        }
        GuiLabel((Rectangle){ 870, 340, 360, 60 }, "CHANGES REQUIRE RESTART\nEXIT TO APPLY?");
        buttonPressed[buttons::GRAPHICS_YES] = GuiSoundedButton((Rectangle){ 864, 424, 144, 48 }, "YES");
        buttonPressed[buttons::GRAPHICS_NO] = GuiSoundedButton((Rectangle){ 1096, 424, 152, 48 }, "NO");
    }
    if(fadeoutActive){
        GuiUnlock();
        float alpha = (timer / 2.0f);
        if(alpha < 0) alpha = 0;
        if(alpha > 1) alpha = 1;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
    }
}

void MenuScene::DrawKeybindingsOverlay(){
    struct KeyRow { const char* action; const char* key; };
    struct KeyGroup { const char* title; std::vector<KeyRow> rows; };

    static const std::vector<KeyGroup> groups = {
        { "MOVEMENT (HELD)", {
            { "Move Up",    "W" },
            { "Move Down",  "S" },
            { "Move Left",  "A" },
            { "Move Right", "D" },
        }},
        { "INTERACTION & DIALOGUE", {
            { "Talk / Interact", "F" },
            { "Advance Dialogue", "Enter" },
            { "Skip Intro",      "Enter / Left Click" },
        }},
        { "IN-GAME MENUS", {
            { "Pause",       "P" },
            { "Inventory",   "I" },
            { "Quests",      "Q" },
            { "Daily Tasks", "T" },
            { "Character",   "C" },
        }},
        { "MENU NAVIGATION", {
            { "Close Panel",         "X" },
            { "Next / Prev Tab",     "E / Q" },
            { "Prev / Next Character", "W / S" },
        }},
        { "BATTLE", {
            { "Select Skill",     "1 / 2 / 3" },
            { "Confirm Skill",    "Enter" },
            { "Switch Character", "S" },
            { "Switch Left / Right", "L / R" },
            { "Skip Turn",        "X" },
            { "Forfeit Yes / No", "Y / N / Esc" },
        }},
    };

    const float winX = 360.0f, winY = 120.0f, winW = 1200.0f, winH = 840.0f;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

    int oldTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    int oldAlign = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    if(GuiWindowBox((Rectangle){ winX, winY, winW, winH }, "KEYBINDINGS")) showKeybindings = false;

    const float colW = winW / 2.0f;
    const float rowH = 34.0f;
    const float groupGap = 20.0f;
    const float labelPad = 40.0f;
    const float startY = winY + 60.0f;

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    float colY[2] = { startY, startY };
    for(size_t g = 0; g < groups.size(); g++){
        int col = (g < 3) ? 0 : 1;
        float baseX = winX + colW * col;
        float& y = colY[col];

        GuiSetStyle(DEFAULT, TEXT_SIZE, 22);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(GOLD));
        GuiLabel((Rectangle){ baseX + labelPad, y, colW - labelPad * 2, rowH }, groups[g].title);
        GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
        y += rowH;

        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        for(const KeyRow& r : groups[g].rows){
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
            GuiLabel((Rectangle){ baseX + labelPad, y, colW * 0.55f, rowH }, r.action);
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
            GuiLabel((Rectangle){ baseX + colW * 0.5f, y, colW * 0.5f - labelPad, rowH }, r.key);
            y += rowH;
        }
        y += groupGap;
    }

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    buttonPressed[buttons::KEYBINDINGS_CLOSE] =
        GuiSoundedButton((Rectangle){ winX + winW / 2.0f - 100.0f, winY + winH - 70.0f, 200.0f, 50.0f }, "CLOSE");

    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(RAYWHITE));
    GuiSetStyle(DEFAULT, TEXT_SIZE, oldTextSize);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, oldAlign);
}
