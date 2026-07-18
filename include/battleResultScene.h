#pragma once
#include "scene.h"
#include "gameData.h"
#include "battleScene.h"
#include <raylib.h>

class BattleResultScene : public Scene {
public:
    explicit BattleResultScene(const BattleContext& ctx, bool victory);
    ~BattleResultScene() override;
    Scene* Update(float dt) override;
    void Draw() override;

private:
    BattleContext battle;
    bool victory;
    float timer = 0.0f;
    float fadeAlpha = 0.0f;
    float exitAlpha = 0.0f;
    bool exiting = false;
    bool buttonsEnabled = false;
    bool continueClicked = false;
    bool retryClicked = false;
    bool returnClicked = false;

    void DrawVictory() const;
    void DrawDefeat() const;
    void DrawStatsPanel() const;
    void DrawRewards() const;
    void DrawButtons() const;
};
