#include "raylib.h"
#include "Scene.h"
#include "StartupScene.h"
#include "MenuScene.h"
#include "gameData.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "lavanda/style_lavanda.h"
#include <vector>

int main()
{
    InitWindow(1800, 1000, "Raylib Project");
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
    CloseAudioDevice();
    CloseWindow();

    return 0;
}