#include "raylib.h"
#include "Scene.h"
#include "StartupScene.h"
#include "MenuScene.h"
#include "gameData.h"
#include "Loader.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "lavanda/style_lavanda.h"
#include <vector>
#include <fstream>

static int LoadStartupFps(int& vsync){
    vsync = 0;
    int fpsIndex = 1;
    std::ifstream file(ResolveDataPath("data/system/settings.json"));
    if(file.is_open()){
        try{
            json j;
            file >> j;
            if(j.contains("vsync")) vsync = j["vsync"].get<int>() ? 1 : 0;
            if(j.contains("fpsIndex")) fpsIndex = j["fpsIndex"].get<int>();
        }
        catch(...){}
    }
    switch(fpsIndex){
        case 0: return 120;
        case 1: return 60;
        default: return 30;
    }
}

int main()
{
    InitWindow(1800, 1000, "Raylib Project");

    int vsync = 0;
    int targetFps = LoadStartupFps(vsync);
    if(vsync) SetConfigFlags(FLAG_VSYNC_HINT);
    else SetTargetFPS(targetFps);

    InitAudioDevice();
    SetRandomSeed(static_cast<unsigned int>(time(NULL)));
    SetExitKey(KEY_NULL);
    gameData.LoadAll();
    GuiLoadStyleLavanda();
    ToggleBorderlessWindowed();
    Scene* scene = new StartupScene();

    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();

        Scene* next = scene->Update(dt);

        BeginDrawing();

        scene->Draw();

        EndDrawing();

        if(next != scene){
            delete scene;
            scene = next;
        }

        if(!scene) break;
    }

    delete scene;
    gameData.SaveGame();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}