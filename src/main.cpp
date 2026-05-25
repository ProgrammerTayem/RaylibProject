#include "raylib.h"
#include "Scene.h"
#include "StartupScene.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main()
{
    InitWindow(800, 450, "Raylib Project");
    ToggleBorderlessWindowed();
    Scene* scene = new StartupScene();

    while(!WindowShouldClose())
    {
        float dt = GetFrameTime();

        Scene* next = scene->Update(dt);

        BeginDrawing();

        ClearBackground(BLACK);
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