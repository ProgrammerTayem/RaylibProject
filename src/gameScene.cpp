#include "gameScene.h"
#include "raylib.h"
#include "raygui.h"
#include <raymath.h>

#define TILE_SIZE 32
#define NPC_COUNT 10

inline bool IsInInteractionRange(const Vector2& a, const Vector2& b){
    return Vector2Distance(a, b) < 2*TILE_SIZE;
}

void UI::TypewriterEffect(const std::string& text, int x, int y, int fontSize, Color color, float dt){
    static float timer = 0.0f;
    static size_t charIndex = 0;

    timer += dt;
    if(timer >= 0.05f && charIndex < text.size()){
        charIndex++;
        timer = 0.0f;
    }
    DrawText(text.substr(0, charIndex).c_str(), x, y, fontSize, color);
}

Player::Player() : speed(128.0f) {
    position = { 128.0f, 128.0f };
}

void Player::Update(float dt, World& world){
    Vector2 movement = {0.0f, 0.0f};
    if(IsKeyDown(KEY_W)) movement.y -= 1.0f;
    if(IsKeyDown(KEY_S)) movement.y += 1.0f;
    if(IsKeyDown(KEY_A)) movement.x -= 1.0f;
    if(IsKeyDown(KEY_D)) movement.x += 1.0f;

    if(Vector2Length(movement) > 0.0f) movement = Vector2Normalize(movement);
    Vector2 nextPosition =
    {
        position.x + movement.x * speed * dt,
        position.y + movement.y * speed * dt
    };
    int tileX = static_cast<int>(nextPosition.x / TILE_SIZE), tileY = static_cast<int>(nextPosition.y / TILE_SIZE);
    if(world.IsWalkable(tileX, tileY)) position = nextPosition;
}

void Player::Draw() const{
    DrawCircleV(position, TILE_SIZE / 2 - 4, RED);
}

NPC::NPC(){
    position = { 0.0f, 0.0f };
    name = "NPC";
    dialogue = "Hello there!";
    dialogueActive = false;
    canInteract = false;
}

void NPC::HandleInteraction(Player& player){
    playerRelPosition = player.GetPosition().y > position.y ? -1 : 1; 
    if(IsInInteractionRange(player.GetPosition(), position)) canInteract = true;
    else canInteract = false;
    if(canInteract && IsKeyDown(KEY_F)){
        dialogueActive = true;
    }
    if((dialogueActive && IsKeyDown(KEY_ENTER)) || (dialogueActive && !IsInInteractionRange(player.GetPosition(), position))){
        dialogueActive = false;
    }
}

void NPC::Update(float dt, World& world){

}

void NPC::Draw() const{
    DrawCircleV(position, TILE_SIZE / 2 - 4, BLUE);
    if(canInteract && !dialogueActive){
        if(playerRelPosition == -1){
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 40, 40, Fade(WHITE, 0.6f));
            DrawText("[F]", position.x + 10, position.y + 10 - TILE_SIZE - 40, 20, WHITE);
            DrawText("Interact", position.x + 50, position.y + 10 - TILE_SIZE - 40, 20, WHITE);
        }
        else{
            DrawRectangle(position.x, position.y + TILE_SIZE, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y + TILE_SIZE, 40, 40, Fade(WHITE, 0.6f));
            DrawText("[F]", position.x + 10, position.y + 10 + TILE_SIZE, 20, WHITE);
            DrawText("Interact", position.x + 50, position.y + 10 + TILE_SIZE, 20, WHITE);
        }
    }
}

void World::GenerateTestMap(){
    for(int x = 0; x < width; x++){
        for(int y = 0; y < height; y++){
            if(x == 0 || y == 0 || x == width - 1 || y == height - 1){
                map[x][y] = { WATER, false };
            }
            else if((x > 5 && x < 15) && (y > 5 && y < 15)){
                map[x][y] = { SAND, true };
            }
            else{
                map[x][y] = { GRASS, true };
            }
        }
    }
}

void World::Update(float dt){
    
}

void World::Draw(){
    for(int x = 0; x < width; x++){
        for(int y = 0; y < height; y++){
            Color tileColor;
            switch(map[x][y].ttype){
                case GRASS: tileColor = GREEN; break;
                case WATER: tileColor = BLUE; break;
                case SAND: tileColor = YELLOW; break;
            }
            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, tileColor);
        }
    }
}

bool World::IsWalkable(int x, int y){
    if(x < 0 || x >= width || y < 0 || y >= height) return false;
    return map[x][y].isWalkable;
}

GameScene::GameScene(){
    world.GenerateTestMap();
    npcs.resize(NPC_COUNT);
    npcs[0].SetPosition({ 10 * TILE_SIZE, 10 * TILE_SIZE });
    camera.target = player.GetPosition();
    camera.offset = { 400.0f, 225.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

Scene* GameScene::Update(float dt){
    world.Update(dt);
    player.Update(dt, world);
    float t = 1.0f - expf(-smoothSpeed * dt);

    camera.target.x += (player.GetPosition().x - camera.target.x) * t;
    camera.target.y += (player.GetPosition().y - camera.target.y) * t;

    const int mapWidthPixels = 200 * TILE_SIZE, mapHeightPixels = 200 * TILE_SIZE;
    const float viewWidth = 800.0f / camera.zoom, viewHeight = 450.0f / camera.zoom;
    
    
    float minX = camera.offset.x / camera.zoom, maxX = mapWidthPixels - camera.offset.x / camera.zoom;
    camera.target.x = Clamp(camera.target.x, minX, maxX);
    
    float minY = camera.offset.y / camera.zoom, maxY = mapHeightPixels - camera.offset.y / camera.zoom;
    camera.target.y = Clamp(camera.target.y, minY, maxY);

    for(auto& npc : npcs){
        npc.Update(dt, world);
        npc.HandleInteraction(player);
    }
    return this;
}

void GameScene::Draw(){
    BeginMode2D(camera);
    world.Draw();
    player.Draw();
    for(const auto& npc : npcs){
        npc.Draw();
    }
    EndMode2D();
    for(const auto& npc : npcs){
        if(npc.IsDialogueActive()){
            int boxWidth = GetScreenWidth(), boxHeight = 200;
            int x = 0, y = GetScreenHeight() - boxHeight;
            DrawRectangle(x, y, boxWidth, boxHeight, DARKGRAY);
            DrawRectangleLines(x, y, boxWidth, boxHeight, WHITE);
            // TypewriterEffect(npc.GetName() + ": " + npc.GetDialogue(), x + 20, y + 20, 20, WHITE, GetFrameTime());
            break;
        }
    }
}

