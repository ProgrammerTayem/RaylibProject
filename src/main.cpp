#include "raylib.h"
#include "Scene.h"
#include "StartupScene.h"
#include "MenuScene.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "lavanda/style_lavanda.h"
#include <vector>

int main()
{
    InitWindow(800, 450, "Raylib Project");
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
    CloseWindow();

    return 0;
}