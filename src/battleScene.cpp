#include "battleScene.h"
#include "gameData.h"
#include "skillvm.h"
#include "raygui.h"
#include <raymath.h>

SkillVM svm;

BattleUI::BattleUI(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    layout.skillButton = {250.0f, sH - 400.0f, 32.0f, 32.0f};
    layout.characterSelector = {250.0f, sH - 150.0f, 64.0f, 64.0f};
    layout.switchCharacter = {sW/2 - 50.0f, sH - 86.0f, 40.0f, 40.0f};
    layout.skipTurn = {sW/2 + 10.0f, sH - 86.0f, 40.0f, 40.0f};
    layout.characterSelectorPressed = layout.skillButtonPressed = layout.skipTurnPressed = layout.switchCharacterPressed = false;
    selectedSkill = -1;
}

static bool IsPassive(const Skill& skill){
    return skill.type == "PASSIVE";
}

void BattleUI::UpdateSkillButtonOffsets(const BattleContext& ctx, float dt){
    const float targetOffset = (layout.skillButton.height + 10.0f);
    for(int i = 0; i < 4; i++){
        float target = 0.0f;
        if(ctx.state == BattleState::SelectingSkill && i == selectedSkill) target = targetOffset;
        float speed = 12.0f;
        float diff = target - skillButtonOffset[i];
        skillButtonOffset[i] += diff * speed * dt;
        if(std::abs(diff) < 0.5f) skillButtonOffset[i] = target;
    }
}

void BattleUI::Update(BattleContext& ctx, float dt){
    UpdateSkillButtonOffsets(ctx, dt);
    switch(ctx.state){
    case BattleState::Entering:
        timer += dt;
        if(timer >= IntroOut) ctx.state = BattleState::BeginTurn;
        break;
    case BattleState::WaitingForInput:
        if(ctx.currentTurn == BattleSide::Player){
            showPlayerDependantUi = true;
            for(int i = 0; i < 4; i++){
                if(IsPassive(ctx.player.active().skills[i])) continue;
                Rectangle rec = layout.skillButton;
                rec.x = layout.skillButton.x + (20 + layout.skillButton.width) * i;
                layout.skillButtonPressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) || (IsKeyPressed(KEY_ONE + i));
                if(layout.skillButtonPressed){
                    selectedSkill = i;
                    if(ctx.player.active().skills[i].cooldown > 0){
                        selectedSkill = -1;
                        ctx.state = BattleState::WaitingForInput;
                    }
                    else ctx.state = BattleState::SelectingSkill;
                }
            }
            layout.switchCharacterPressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.switchCharacter)) || (IsKeyPressed(KEY_S));
            if(layout.switchCharacterPressed) ctx.state = BattleState::SwitchingCharacter;
            layout.skipTurnPressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.skipTurn)) || (IsKeyPressed(KEY_X));
            if(layout.skipTurnPressed) ctx.state = BattleState::EndTurn;
        }
        else{
            showPlayerDependantUi = false;
            selectedSkill = -1;
            const std::array<int, 3> priority = {2, 1, 0};
            for(int i : priority){
                if(IsPassive(ctx.enemy.active().skills[i])) continue;
                if(ctx.enemy.active().skills[i].cooldown <= 0){
                    selectedSkill = i;
                    break;
                }
            }
            if(selectedSkill == -1) ctx.state = BattleState::WaitingForInput;
            else ctx.state = BattleState::SelectingSkill;
        }
        break;
    case BattleState::SelectingSkill:
        if(ctx.currentTurn == BattleSide::Player){
            Rectangle rec =  layout.skillButton;
            rec.x = layout.skillButton.x + (20 + layout.skillButton.width) * selectedSkill;
            if((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) || IsKeyPressed(KEY_ENTER)){
                ctx.pendingAction.bActType = BattleActionType::Skill;
                ctx.pendingAction.skill = &ctx.player.active().skills[selectedSkill];
                ctx.state = BattleState::ExecutingSkill;
                layout.characterSelectorPressed = layout.skillButtonPressed = layout.skipTurnPressed = layout.switchCharacterPressed = false;
            }
            else if((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), rec))) ctx.state = BattleState::WaitingForInput;
        }
        else{
            ctx.pendingAction.bActType = BattleActionType::Skill;
            ctx.pendingAction.skill = &ctx.enemy.active().skills[selectedSkill];
            ctx.state = BattleState::ExecutingSkill;
        }
        break;
    case BattleState::SwitchingCharacter:
        int r = 0;
        for(int i = 0; i < 3; i++){
            if(ctx.player.activeIndex == i) continue;
             Rectangle rec = layout.characterSelector;
             rec.x = layout.characterSelector.x + (20 + layout.characterSelector.width) * r;
             if((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) || IsKeyPressed(KEY_A + r)){
                ctx.pendingAction.bActType = BattleActionType::Switch;
                ctx.player.activeIndex = i;
                ctx.state = BattleState::ExecutingSkill;
            }
            r++;
        }
        if(ctx.state == BattleState::SwitchingCharacter && IsKeyPressed(KEY_S)) ctx.state = BattleState::WaitingForInput;
        break;
    }
}

void BattleUI::Draw(const BattleContext &ctx) const {
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    if(ctx.state == BattleState::Entering){
        const int in = 1.5f, hold = 1.1f;
        DrawRectangle(0, (sH-200)/2, sW, 200, Fade(GRAY, 0.6));
        int stWidth = MeasureText("BATTLE START", 50);
        static float x = -static_cast<float>(stWidth), y = sH/2; // (sH-200)/2 + 100 = sH/2
        float vel = (sW + stWidth)/(in + hold);
        if(timer <= in){
            x = vel*timer - stWidth;
            DrawText("BATTLE START", x, y, 50, WHITE);
        }
        else if(timer > in && timer <= in + hold){
            DrawText("BATTLE START", x, y, 50, WHITE);
        }
    }
    if(ctx.state == BattleState::WaitingForInput || ctx.state == BattleState::SelectingSkill || ctx.state == BattleState::SwitchingCharacter){
        if(showPlayerDependantUi){
            for(int i = 0; i < 4; i++){
                if(IsPassive(ctx.player.active().skills[i])) continue;
                Rectangle rec = layout.skillButton;
                Rectangle src = {0, 0, 32, 32};
                rec.x = layout.skillButton.x + (20 + layout.skillButton.width) * i;
                if(ctx.state == BattleState::SelectingSkill && i == selectedSkill) rec.y -= skillButtonOffset[i];
                DrawTexturePro(*ctx.player.active().skills[i].icon, src, rec, Vector2{0, 0}, 0.0f, ctx.player.active().skills[i].cooldown > 0 ? GRAY : WHITE);
                DrawRectangleLinesEx(rec, 1.5f, WHITE);
            }
            int r = 0;
            for(int i = 0; i < 3; i++){
                if(ctx.player.activeIndex == i) continue;
                Rectangle rec = layout.characterSelector;
                Rectangle src = {0, 0, 16, 16};
                rec.x = layout.characterSelector.x + (20 + layout.characterSelector.width) * r;
                if(ctx.state == BattleState::SwitchingCharacter) DrawRectangle(rec.x - 25, rec.y - 10, 2*(rec.width + 20), rec.height + 20, YELLOW);
                DrawTexturePro(*ctx.player.members[i].portrait, src, rec, Vector2{0, 0}, 0.0f, WHITE);
                DrawRectangleLinesEx(rec, 1.5f, GRAY);
                r++;
            }
            DrawRectangleRec(layout.skipTurn, WHITE);
            DrawRectangleRec(layout.switchCharacter, WHITE);
        }
    }
}

void BattleRenderer::Draw(const BattleContext& ctx) const{
    Rectangle src = {0, 0, 16, 16};
    Rectangle playerDest = {300, 300, 128, 128};
    Rectangle enemyDest = {650, 300, 128, 128};
    DrawTexturePro(*ctx.player.active().portrait, src, playerDest, Vector2{0, 0}, 0.0f, WHITE);
    DrawText(std::to_string(ctx.player.active().stats.HP).c_str(), 300, 220, 25, BLACK);
    DrawTexturePro(*ctx.enemy.active().portrait, src, enemyDest, Vector2{0, 0}, 0.0f, WHITE);
    DrawText(std::to_string(ctx.enemy.active().stats.HP).c_str(), 600, 220, 25, BLACK);
}


BattleScene::BattleScene(){
    for(int i = 0; i < 3; i++){
        battle.player.members[i].name = gameData.characters[i].name;
        battle.player.members[i].portrait = gameData.characters[i].portrait;
        battle.player.members[i].skills = gameData.characters[i].skills;
        battle.player.members[i].stats = gameData.characters[i].stats;
        battle.enemy.members[i].name = gameData.inBattleCharacters[i]->name;
        battle.enemy.members[i].portrait = gameData.inBattleCharacters[i]->portrait;
        battle.enemy.members[i].skills = gameData.inBattleCharacters[i]->skills;
        battle.enemy.members[i].stats = gameData.inBattleCharacters[i]->stats;
    }
    ShowCursor();
    battle.currentTurn = BattleSide::Player;
}

BattleScene::~BattleScene(){

}

void BattleScene::BeginTurn(){
    battle.state = BattleState::WaitingForInput;
}

void BattleScene::HandleInput(float dt){
    switch(battle.state){
        case BattleState::WaitingForInput:
        case BattleState::SelectingSkill:
        case BattleState::SwitchingCharacter:
            ui.Update(battle, dt);
            break;
    }
}

void BattleScene::ExecuteAction(){
    if(battle.pendingAction.bActType == BattleActionType::Switch){
        battle.state = BattleState::EndTurn;
    }
    else{
        battle.pendingAction.actor = (battle.currentTurn == BattleSide::Player) ? &battle.player.active() : &battle.enemy.active();
        if(battle.pendingAction.skill->target == "self") battle.pendingAction.target = battle.pendingAction.actor;
        else if(battle.pendingAction.skill->target == "enemy_single") battle.pendingAction.target = (battle.currentTurn == BattleSide::Player) ? &battle.enemy.active() : &battle.player.active();
        battle.state = BattleState::ProcessingSkillEffect;
    }
}

void BattleScene::ProcessEffects(){
    svm.Resolve(&battle.pendingAction.actor->stats, &battle.pendingAction.target->stats, battle.pendingAction.skill);
    svm.Resolve(&battle.pendingAction.actor->stats, &battle.pendingAction.target->stats, battle.pendingAction.skill, &battle.pendingAction.actor->skills);
    battle.pendingAction.skill->cooldown = battle.pendingAction.skill->baseCooldown;
    battle.state = BattleState::EndTurn;
}

void BattleScene::EndTurn(){
    svm.CooldownAndDurationManager(battle);
    battle.currentTurn = (battle.currentTurn == BattleSide::Player) ? BattleSide::Enemy : BattleSide::Player;
    battle.state = BattleState::BeginTurn;
}

Scene* BattleScene::Update(float dt){
    HandleInput(dt);
    switch(battle.state){
        case BattleState::Entering:
            ui.Update(battle, dt);
            break;
        case BattleState::BeginTurn:
            BeginTurn();
            break;
        case BattleState::ExecutingSkill:
            ExecuteAction();
            break;
        case BattleState::ProcessingSkillEffect:
            ProcessEffects();
            break;
        case BattleState::EndTurn:
            EndTurn();
            break;
    }
    return this;
}

void BattleScene::Draw(){
    ClearBackground(GREEN);
    ui.Draw(battle);
    renderer.Draw(battle);
}