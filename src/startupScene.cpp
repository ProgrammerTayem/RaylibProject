#include "startupScene.h"
#include "menuScene.h"
#include "raylib.h"

StartupScene::StartupScene() : timer(0.0f), state(IN) {}

Scene* StartupScene::Update(float dt){
    timer += dt;

    switch(state){
        case IN:
            if(timer > 2.0f) { timer = 0; state = HOLD; }
            break;

        case HOLD:
            if(timer > 1.0f) { timer = 0; state = OUT; }
            break;

        case OUT:
            if(timer > 2.0f) return new MenuScene();
            break;

        default:
            break;
    }

    if(IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return new MenuScene();

    return this;
}

void StartupScene::Draw()
{
    ClearBackground(BLACK);
    float alpha = 1.0f;

    if(state == IN) alpha = timer / 2.0f;
    else if(state == OUT) alpha = 1.0f - (timer / 2.0f);

    if(alpha < 0) alpha = 0;
    if(alpha > 1) alpha = 1;

    const char* text = "GAME STUDIO";
    int fontSize = 30;
    int textWidth = MeasureText(text, fontSize);
    int x = (GetScreenWidth() - textWidth) / 2;
    int y = (GetScreenHeight() - fontSize) / 2;

    DrawText(text, x, y, fontSize, Fade(WHITE, alpha));
}