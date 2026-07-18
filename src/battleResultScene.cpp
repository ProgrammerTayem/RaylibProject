#include "battleResultScene.h"
#include "gameData.h"
#include "gameScene.h"
#include "raygui.h"
#include <raymath.h>

BattleResultScene::BattleResultScene(const BattleContext& ctx, bool victory) : battle(ctx), victory(victory){
    timer = 0.0f;
    exiting = false;
    buttonsEnabled = false;
    continueClicked = false;
    retryClicked = false;
    returnClicked = false;
    fadeAlpha = 0.0f;
    exitAlpha = 0.0f;
}

BattleResultScene::~BattleResultScene() = default;

Scene* BattleResultScene::Update(float dt){
    if(exiting){
        exitAlpha += dt / 0.3f;
            if(exitAlpha >= 1.0f){
            if(continueClicked || returnClicked) gameData.fighting = false;
            if(retryClicked) gameData.fighting = true;
            return new GameScene(gameData.preBattleSpawnPosition);
            }
        return this;
    }

    timer += dt;
    fadeAlpha = Clamp(timer / 0.5f, 0.0f, 1.0f);
    if(timer >= 1.5f && !buttonsEnabled) buttonsEnabled = true;

    if(!buttonsEnabled) return this;

    int oldTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);

    float buttonWidth = 160.0f;
    float buttonHeight = 50.0f;
    float spacing = 20.0f;
    float buttonsY = 880.0f;

    if(victory){
        float buttonsX = 1620.0f;
        Rectangle continueBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        if(GuiButton(continueBounds, "Continue")){
            PlaySound(*(gameData.res.GetSfx("button_click")));
            continueClicked = true;
            exiting = true;
        }
    }else if(battle.forfeit){
        float totalWidth = buttonWidth;
        float buttonsX = 1620.0f - totalWidth / 2.0f + buttonWidth / 2.0f;
        Rectangle returnBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        if(GuiButton(returnBounds, "Return to Map")){
            PlaySound(*(gameData.res.GetSfx("button_click")));
            returnClicked = true;
            exiting = true;
        }
    }else{
        float totalWidth = (buttonWidth * 2.0f) + spacing;
        float buttonsX = 1620.0f - totalWidth / 2.0f + buttonWidth / 2.0f;
        Rectangle retryBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        Rectangle returnBounds = { buttonsX + buttonWidth + spacing, buttonsY, buttonWidth, buttonHeight };
        if(GuiButton(retryBounds, "Retry")){
            PlaySound(*(gameData.res.GetSfx("button_click")));
            retryClicked = true;
            exiting = true;
        }
        if(GuiButton(returnBounds, "Return to Map")){
            PlaySound(*(gameData.res.GetSfx("button_click")));
            returnClicked = true;
            exiting = true;
        }
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, oldTextSize);
    return this;
}

void BattleResultScene::Draw(){
    int sW = GetScreenWidth();
    int sH = GetScreenHeight();

    if(battle.background){
        Rectangle src = {0, 0, (float)battle.background->width, (float)battle.background->height};
        Rectangle dst = {0.0f, 0.0f, (float)sW, (float)sH};
        DrawTexturePro(*battle.background, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }

    Color overlayColor = victory ? (Color){ 255, 215, 0, (unsigned char)(40 * fadeAlpha) } : (Color){ 40, 50, 70, (unsigned char)(80 * fadeAlpha) };
    DrawRectangle(0, 0, sW, sH, overlayColor);

    if(victory) DrawVictory();
    else DrawDefeat();

    DrawStatsPanel();
    if(victory) DrawRewards();
    DrawButtons();

    if(exiting){
        DrawRectangle(0, 0, sW, sH, Fade(BLACK, exitAlpha));
    }
}

void BattleResultScene::DrawVictory() const{
    const char* title = "VICTORY";
    int fontSize = 60;
    Vector2 size = MeasureTextEx(GuiGetFont(), title, fontSize, 2.0f);
    float x = (GetScreenWidth() - size.x) / 2.0f;
    float y = 80.0f;
    float scale = 0.8f + 0.2f * Clamp((timer - 0.3f) / 0.3f, 0.0f, 1.0f);
    float alpha = Clamp((timer - 0.3f) / 0.3f, 0.0f, 1.0f);
    DrawTextEx(GuiGetFont(), title, (Vector2){ x, y }, fontSize * scale, 2.0f, Fade(GOLD, alpha));
}

void BattleResultScene::DrawDefeat() const{
    const char* title = battle.forfeit ? "FORFEIT" : "DEFEAT";
    int fontSize = battle.forfeit ? 48 : 48;
    Vector2 size = MeasureTextEx(GuiGetFont(), title, fontSize, 2.0f);
    float x = (GetScreenWidth() - size.x) / 2.0f;
    float y = 100.0f;
    float alpha = Clamp((timer - 0.3f) / 0.5f, 0.0f, 1.0f);
    DrawTextEx(GuiGetFont(), title, (Vector2){ x, y }, fontSize, 2.0f, Fade(RED, alpha));
}

void BattleResultScene::DrawStatsPanel() const{
    int sW = GetScreenWidth();
    float panelX = sW / 2.0f - 200.0f;
    float panelY = 220.0f;
    float panelW = 400.0f;
    float panelH = 240.0f;

    float padX = 26.0f;
    float padTop = 50.0f;
    float padBottom = 24.0f;

    DrawRectangleRounded((Rectangle){ panelX, panelY, panelW, panelH }, 0.06f, 12, Fade(BLACK, 0.75f));
    DrawRectangleLinesEx((Rectangle){ panelX, panelY, panelW, panelH }, 2.0f, Fade(WHITE, 0.3f));

    const char* title = "BATTLE SUMMARY";
    Vector2 titleSize = MeasureTextEx(GuiGetFont(), title, 18, 1.0f);
    DrawTextEx(GuiGetFont(), title, (Vector2){ panelX + (panelW - titleSize.x) / 2.0f, panelY + 16.0f }, 18, 1.0f, Fade(WHITE, 0.8f));

    std::string lines[] = {
        ("Turns:     " + std::to_string(battle.battleStats.turnsElapsed)),
        ("Damage:    " + std::to_string(battle.battleStats.totalDamageDealt)),
        ("Received:  " + std::to_string(battle.battleStats.totalDamageReceived)),
        ("Crits:     " + std::to_string(battle.battleStats.criticalHits)),
        ("Skills:    " + std::to_string(battle.battleStats.skillsUsed))
    };

    float lineY = panelY + padTop;
    float lineGap = 30.0f;
    for(int i = 0; i < 5; i++){
        float animT = Clamp((timer - 0.5f - i * 0.1f) / 0.4f, 0.0f, 1.0f);
        float offsetX = (1.0f - animT) * 100.0f;
        float alpha = animT;
        DrawTextEx(GuiGetFont(), lines[i].c_str(), (Vector2){ panelX + padX + offsetX, lineY + i * lineGap }, 20, 1.0f, Fade(WHITE, alpha));
    }
}

void BattleResultScene::DrawRewards() const{
    int sW = GetScreenWidth();
    float centerX = sW / 2.0f;
    float iconSize = 48.0f;
    float rowY = 560.0f;

    Texture2D* coinTex = gameData.res.GetTexture("coins_heap");
    Texture2D* crystalTex = gameData.res.GetTexture("crystal");
    Texture2D* expTex = gameData.res.GetTexture("exp");

    float animT = Clamp((timer - 0.8f) / 1.0f, 0.0f, 1.0f);
    int coins = (int)(battle.rewards.coins * animT);
    int crystals = (int)(battle.rewards.crystals * animT);
    int exp = (int)(battle.rewards.exp * animT);

    struct RewardEntry { Texture2D* tex; const char* label; int value; };
    RewardEntry entries[] = {
        { coinTex,    "Coins",    coins },
        { crystalTex, "Crystals", crystals },
        { expTex,     "Exp",      exp }
    };

    int count = 3;
    float colSpacing = 200.0f;
    float startX = centerX - ((count - 1) * colSpacing) / 2.0f;

    for(int i = 0; i < count; i++){
        float colX = startX + i * colSpacing;
        float itemAlpha = Clamp((timer - 0.8f - i * 0.15f) / 0.6f, 0.0f, 1.0f);

        if(entries[i].tex){
            DrawTextureEx(*entries[i].tex, (Vector2){ colX - iconSize / 2.0f, rowY - iconSize / 2.0f }, 0.0f, 1.0f, Fade(WHITE, itemAlpha));
        }

        std::string valueText = std::to_string(entries[i].value);
        int valueFontSize = 22;
        Vector2 valueSize = MeasureTextEx(GuiGetFont(), valueText.c_str(), valueFontSize, 1.0f);
        DrawTextEx(GuiGetFont(), valueText.c_str(), (Vector2){ colX - valueSize.x / 2.0f, rowY + iconSize / 2.0f + 8.0f }, valueFontSize, 1.0f, Fade(WHITE, itemAlpha));

        int labelFontSize = 18;
        Vector2 labelSize = MeasureTextEx(GuiGetFont(), entries[i].label, labelFontSize, 1.0f);
        DrawTextEx(GuiGetFont(), entries[i].label, (Vector2){ colX - labelSize.x / 2.0f, rowY + iconSize / 2.0f + 38.0f }, labelFontSize, 1.0f, Fade(WHITE, 0.7f * itemAlpha));
    }
}

void BattleResultScene::DrawButtons() const{
    if(!buttonsEnabled) return;
    int oldTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);

    float buttonWidth = 160.0f;
    float buttonHeight = 50.0f;
    float spacing = 20.0f;
    float buttonsY = 880.0f;

    if(victory){
        float buttonsX = 1620.0f;
        Rectangle continueBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        GuiButton(continueBounds, "Continue");
    }else if(battle.forfeit){
        float totalWidth = buttonWidth;
        float buttonsX = 1620.0f - totalWidth / 2.0f + buttonWidth / 2.0f;
        Rectangle returnBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        GuiButton(returnBounds, "Return to Map");
    }else{
        float totalWidth = (buttonWidth * 2.0f) + spacing;
        float buttonsX = 1620.0f - totalWidth / 2.0f + buttonWidth / 2.0f;
        Rectangle retryBounds = { buttonsX, buttonsY, buttonWidth, buttonHeight };
        Rectangle returnBounds = { buttonsX + buttonWidth + spacing, buttonsY, buttonWidth, buttonHeight };
        GuiButton(retryBounds, "Retry");
        GuiButton(returnBounds, "Return to Map");
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, oldTextSize);
}
