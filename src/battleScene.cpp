#include "battleScene.h"
#include "battleResultScene.h"
#include "gameData.h"
#include "skillvm.h"
#include "raygui.h"
#include <raymath.h>

SkillVM svm;

BattleUI::BattleUI(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    layout.bottomBar = {0.0f, (float)(sH - 160), (float)sW, 160.0f};

    float skillStartX = 40.0f;
    float skillY = layout.bottomBar.y + 20.0f;
    float skillSize = 80.0f;
    float skillGap = 12.0f;
    layout.skillButton = {skillStartX, skillY, skillSize, skillSize};

    float portraitSize = 80.0f;
    layout.characterSelector.x = sW / 2.0f - portraitSize / 2.0f;
    layout.characterSelector.y = skillY;
    layout.characterSelector.width = portraitSize;
    layout.characterSelector.height = portraitSize;

    float actionW = 100.0f;
    float actionH = 40.0f;
    float actionY = skillY + (skillSize - actionH) / 2.0f;
    layout.switchCharacter = {(float)(sW - 220), actionY, actionW, actionH};
    layout.skipTurn = {(float)(sW - 110), actionY, actionW, actionH};
    layout.forfeit = {(float)(sW - 340), actionY, 90.0f, actionH};

    layout.characterSelectorPressed = layout.skillButtonPressed = layout.skipTurnPressed = layout.switchCharacterPressed = false;
    selectedSkill = -1;

    layout.playerPanel = {20.0f, 20.0f, 140.0f, 280.0f};
    layout.enemyPanel = {(float)(sW - 160), 20.0f, 140.0f, 280.0f};
    float slotSize = 64.0f;
    float slotGap = 12.0f;
    float slotStartY = 40.0f;
    for(int i = 0; i < 3; i++){
        layout.playerSlots[i] = {layout.playerPanel.x + 38.0f, slotStartY + (slotSize + slotGap) * i, slotSize, slotSize};
        layout.enemySlots[i] = {layout.enemyPanel.x + 38.0f, slotStartY + (slotSize + slotGap) * i, slotSize, slotSize};
    }

    float playerHudX = sW / 2.0f - 175.0f - 36.0f;
    float enemyHudX = sW / 2.0f + 175.0f - 36.0f;
    float hudY = (float)sH / 2.0f + 78.0f;
    layout.playerHudName = {playerHudX, hudY, 200.0f, 12.0f};
    layout.playerHudBar = {playerHudX, hudY + 32.0f, 200.0f, 24.0f};
    layout.playerHudText = {playerHudX, hudY + 16.0f, 200.0f, 12.0f};
    layout.enemyHudName = {enemyHudX, hudY, 200.0f, 12.0f};
    layout.enemyHudBar = {enemyHudX, hudY + 32.0f, 200.0f, 24.0f};
    layout.enemyHudText = {enemyHudX, hudY + 16.0f, 200.0f, 12.0f};
}

static bool IsPassive(const Skill& skill){
    return skill.type == "PASSIVE";
}

void BattleUI::UpdateSkillButtonOffsets(const BattleContext& ctx, float dt){
    const float targetOffset = layout.skillButton.height + 100.0f;
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
    if(forfeitConfirmation){
        if(IsKeyPressed(KEY_Y)){
            ctx.state = BattleState::Defeat;
            ctx.forfeit = true;
            forfeitConfirmation = false;
        }
        else if(IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)){
            forfeitConfirmation = false;
        }
        else{
            int sW = GetScreenWidth();
            float boxWidth = 400.0f, boxHeight = 180.0f;
            float boxX = (sW - boxWidth) / 2.0f, boxY = (GetScreenHeight() - boxHeight) / 2.0f;
            float buttonWidth = 140.0f, buttonHeight = 50.0f, spacing = 30.0f;
            float buttonsStartX = boxX + (boxWidth - (buttonWidth * 2.0f + spacing)) / 2.0f;
            float buttonsY = boxY + 100.0f;
            Rectangle yesBounds = { buttonsStartX, buttonsY, buttonWidth, buttonHeight };
            Rectangle noBounds = { buttonsStartX + buttonWidth + spacing, buttonsY, buttonWidth, buttonHeight };
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), yesBounds)){
                ctx.state = BattleState::Defeat;
                ctx.forfeit = true;
                forfeitConfirmation = false;
            }
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), noBounds)){
                forfeitConfirmation = false;
            }
        }
        return;
    }
    switch(ctx.state){
    case BattleState::Entering:
        timer += dt;
        if(timer >= IntroOut) ctx.state = BattleState::TurnStart;
        break;
    case BattleState::TurnStart:
        timer += dt;
        if(timer >= TurnStartDur) ctx.state = BattleState::BeginTurn;
        break;
    case BattleState::WaitingForInput:
        if(ctx.currentTurn == BattleSide::Player){
            showPlayerDependantUi = true;
            for(int i = 0; i < 3; i++){
                if(IsPassive(ctx.player.active().skills[i])) continue;
                Rectangle rec = layout.skillButton;
                rec.x = layout.skillButton.x + (layout.skillButton.width + 12.0f) * i;
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
            bool canSwitch = ctx.player.AvailableCount() > 0;
            layout.switchCharacterPressed = canSwitch && ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.switchCharacter)) || (IsKeyPressed(KEY_S)));
            if(layout.switchCharacterPressed) ctx.state = BattleState::SwitchingCharacter;
            layout.skipTurnPressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.skipTurn)) || (IsKeyPressed(KEY_X));
            if(layout.skipTurnPressed) ctx.state = BattleState::EndTurn;
            layout.forfeitPressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.forfeit));
            if(layout.forfeitPressed && !forfeitConfirmation) forfeitConfirmation = true;
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
            rec.x = layout.skillButton.x + (layout.skillButton.width + 12.0f) * selectedSkill;
            if((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)) || IsKeyPressed(KEY_ENTER)){
                ctx.pendingAction.bActType = BattleActionType::Skill;
                ctx.pendingAction.skill = &ctx.player.active().skills[selectedSkill];
                ctx.state = BattleState::ExecutingSkill;
    layout.characterSelectorPressed = layout.skillButtonPressed = layout.skipTurnPressed = layout.switchCharacterPressed = layout.forfeitPressed = false;
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
    {
        auto switchRects = GetSwitchPortraitRects(ctx.player);
        int selectedSwitch = -1;

        if(IsKeyPressed(KEY_L)){
            for(int i = ctx.player.activeIndex - 1; i >= 0; i--){
                if(ctx.player.IsAvailable(i)){ selectedSwitch = i; break; }
            }
        }
        else if(IsKeyPressed(KEY_R)){
            for(int i = ctx.player.activeIndex + 1; i < 3; i++){
                if(ctx.player.IsAvailable(i)){ selectedSwitch = i; break; }
            }
        }

        int availIdx = 0;
        for(int i = 0; i < 3; i++){
            if(ctx.player.activeIndex == i) continue;
            if(ctx.player.defeated[i]) continue;
            if(!ctx.player.IsAvailable(i)) continue;

            Rectangle rec = switchRects[availIdx];
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), rec)){
                selectedSwitch = i;
            }
            availIdx++;
        }

        if(selectedSwitch != -1){
            ctx.pendingAction.bActType = BattleActionType::Switch;
            ctx.pendingAction.switchOutIndex = ctx.player.activeIndex;
            ctx.player.activeIndex = selectedSwitch;
            ctx.state = BattleState::ExecutingSkill;
        }
    }
    if(ctx.state == BattleState::SwitchingCharacter && 
       ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), layout.switchCharacter)) || IsKeyPressed(KEY_S))){
        ctx.state = BattleState::WaitingForInput;
        selectedSkill = -1;
    }
    break;
    }
}

void BattleUI::Draw(const BattleContext &ctx) const {
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    if(ctx.state == BattleState::Entering){
        DrawEntering(sW, sH);
        return;
    }
    if(ctx.state == BattleState::TurnStart){
        DrawTurnStart(sW, sH, ctx.turnNumber);
        return;
    }

    bool inputState = (ctx.state == BattleState::WaitingForInput ||
                       ctx.state == BattleState::SelectingSkill ||
                       ctx.state == BattleState::SwitchingCharacter);

    if(inputState) DrawTurnIndicator(ctx);

    DrawActiveHUD(ctx.player.active(), {(float)GetScreenWidth() / 2.0f - 175.0f, (float)GetScreenHeight() / 2.0f}, ctx.player.active().name.c_str(), true);
    DrawActiveHUD(ctx.enemy.active(), {(float)GetScreenWidth() / 2.0f + 175.0f, (float)GetScreenHeight() / 2.0f}, ctx.enemy.active().name.c_str(), false);

    DrawTeamPanel(ctx.player, true, {layout.playerPanel.x, layout.playerPanel.y});
    DrawTeamPanel(ctx.enemy, false, {layout.enemyPanel.x, layout.enemyPanel.y});

    if(inputState){
        DrawBottomBar(ctx);
        DrawSkillButtons(ctx);
    }

    if(forfeitConfirmation){
        DrawRectangle(0, 0, sW, sH, Fade(BLACK, 0.4f));
        float boxWidth = 400.0f, boxHeight = 180.0f;
        float boxX = (sW - boxWidth) / 2.0f, boxY = (sH - boxHeight) / 2.0f;
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 30, 30, 45, 240 });
        DrawRectangleLinesEx((Rectangle){ boxX, boxY, boxWidth, boxHeight }, 3.0f, (Color){ 171, 155, 211, 255 });
        const char* titleText = "FORFEIT?";
        Vector2 titleSize = MeasureTextEx(GuiGetFont(), titleText, 32, 2.0f);
        float titleX = boxX + (boxWidth - titleSize.x) / 2.0f;
        float titleY = boxY + 30.0f;
        DrawTextEx(GuiGetFont(), titleText, (Vector2){ titleX, titleY }, 32.0f, 2.0f, WHITE);
        float buttonWidth = 140.0f, buttonHeight = 50.0f, spacing = 30.0f;
        float totalButtonsWidth = (buttonWidth * 2.0f) + spacing;
        float buttonsStartX = boxX + (boxWidth - totalButtonsWidth) / 2.0f;
        float buttonsY = boxY + 100.0f;
        Rectangle yesBounds = { buttonsStartX, buttonsY, buttonWidth, buttonHeight };
        Rectangle noBounds = { buttonsStartX + buttonWidth + spacing, buttonsY, buttonWidth, buttonHeight };
        int oldTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
        GuiButton(yesBounds, "Yes");
        GuiButton(noBounds, "No");
        GuiSetStyle(DEFAULT, TEXT_SIZE, oldTextSize);
    }
}

void BattleUI::DrawEntering(int sW, int sH) const {
    const float in = 1.5f, hold = 1.1f;
    DrawRectangle(0, (sH-200)/2, sW, 200, Fade(GRAY, 0.6));
    int stWidth = MeasureText("BATTLE START", 50);
    static float x = -static_cast<float>(stWidth), y = sH/2;
    float vel = (sW + stWidth)/(in + hold);
    if(timer <= in){
        x = vel*timer - stWidth;
        DrawText("BATTLE START", x, y, 50, WHITE);
    }
    else if(timer > in && timer <= in + hold){
        float centeredX = (sW - stWidth) / 2.0f;
        DrawText("BATTLE START", centeredX, y, 50, WHITE);
    }
}

void BattleUI::DrawTurnStart(int sW, int sH, int turnNumber) const {
    const float inT = 0.25f, holdT = 0.2f, outT = 0.25f;
    int fontSize = 44;
    std::string label = "Turn " + std::to_string(turnNumber);
    int stWidth = MeasureText(label.c_str(), fontSize);
    float pillW = static_cast<float>(stWidth + 60);
    float pillH = static_cast<float>(fontSize + 24);
    float pillX = (sW - pillW) / 2.0f;
    float restY = (sH - pillH) / 2.0f;
    float offY = -pillH - 10.0f;
    float y = restY;
    if(timer <= inT){
        float t = timer / inT;
        float e = t * t * (3.0f - 2.0f * t);
        y = Lerp(offY, restY, e);
    }
    else if(timer > inT && timer <= inT + holdT){
        y = restY;
    }
    else{
        float t = (timer - inT - holdT) / outT;
        float e = t * t * (3.0f - 2.0f * t);
        y = Lerp(restY, offY, e);
    }
    DrawRectangleRounded({pillX, y, pillW, pillH}, 0.3f, 8.0f, Fade(GRAY, 0.7f));
    DrawText(label.c_str(), static_cast<int>(pillX + 30.0f), static_cast<int>(y + (pillH - fontSize) / 2.0f), fontSize, WHITE);
}

void BattleUI::DrawTurnIndicator(const BattleContext& ctx) const {
    int sW = GetScreenWidth();
    std::string text;
    if(ctx.state == BattleState::WaitingForInput){
        text = "Turn " + std::to_string(ctx.turnNumber) + " — " + (ctx.currentTurn == BattleSide::Player ? "Player" : "Enemy");
    }
    else if(ctx.state == BattleState::SelectingSkill){
        text = "Select a skill...";
    }
    else if(ctx.state == BattleState::SwitchingCharacter){
        text = "Switch character...";
    }
    else return;

    int fontSize = 20;
    int tw = MeasureText(text.c_str(), fontSize);
    float w = tw + 40.0f;
    float h = fontSize + 16.0f;
    float x = (sW - w) / 2.0f;
    float y = 80.0f;
    DrawRectangleRounded({x, y, w, h}, 0.3f, 8.0f, Fade(DARKGRAY, 0.7f));
    DrawText(text.c_str(), static_cast<int>(x + 20.0f), static_cast<int>(y + (h - fontSize) / 2.0f), fontSize, WHITE);
}

void BattleUI::DrawBottomBar(const BattleContext& ctx) const {
    int sW = GetScreenWidth();
    float alpha = (ctx.currentTurn == BattleSide::Player) ? 0.4f : 0.2f;
    DrawRectangleRec(layout.bottomBar, Fade(DARKGRAY, alpha));

    if(ctx.currentTurn == BattleSide::Player || ctx.state == BattleState::SwitchingCharacter){
        Rectangle activePortrait = layout.characterSelector;
        DrawRectangleRec(activePortrait, Fade(WHITE, 0.1f));
        DrawRectangleLinesEx(activePortrait, 2.0f, YELLOW);
        if(ctx.player.active().portrait){
            Rectangle src = {0, 0, 16, 16};
            DrawTexturePro(*ctx.player.active().portrait, src, activePortrait, Vector2{0, 0}, 0.0f, WHITE);
        }
        DrawText(ctx.player.active().name.c_str(), static_cast<int>(activePortrait.x), static_cast<int>(activePortrait.y - 14), 10, WHITE);

        auto switchRects = GetSwitchPortraitRects(ctx.player);
        int availIdx = 0;
        for(int i = 0; i < 3; i++){
            if(i == ctx.player.activeIndex) continue;
            if(ctx.player.defeated[i]) continue;
            if(!ctx.player.IsAvailable(i)) continue;

            Rectangle rec = switchRects[availIdx];
            DrawRectangleRec(rec, Fade(DARKGRAY, 0.5f));
            DrawRectangleLinesEx(rec, 1.0f, GRAY);
            if(ctx.player.members[i].portrait){
                Rectangle src = {0, 0, 16, 16};
                DrawTexturePro(*ctx.player.members[i].portrait, src, rec, Vector2{0, 0}, 0.0f, WHITE);
            }
            DrawText(ctx.player.members[i].name.c_str(), static_cast<int>(rec.x), static_cast<int>(rec.y - 12), 8, WHITE);
            availIdx++;
        }
    }

    bool canSwitch = ctx.player.AvailableCount() > 0;
    DrawRectangleRec(layout.skipTurn, WHITE);
    DrawRectangleRec(layout.switchCharacter, canSwitch ? WHITE : DARKGRAY);
    int swTextW = MeasureText("SWITCH", 15);
    DrawText("SWITCH", (int)(layout.switchCharacter.x + (layout.switchCharacter.width - swTextW) / 2),
             (int)(layout.switchCharacter.y + (layout.switchCharacter.height - 15) / 2), 15, canSwitch ? BLACK : GRAY);
    int skTextW = MeasureText("SKIP", 15);
    DrawText("SKIP", (int)(layout.skipTurn.x + (layout.skipTurn.width - skTextW) / 2),
             (int)(layout.skipTurn.y + (layout.skipTurn.height - 15) / 2), 15, BLACK);
    DrawRectangleRec(layout.forfeit, WHITE);
    int ftTextW = MeasureText("FORFEIT", 15);
    DrawText("FORFEIT", (int)(layout.forfeit.x + (layout.forfeit.width - ftTextW) / 2),
             (int)(layout.forfeit.y + (layout.forfeit.height - 15) / 2), 15, BLACK);
}

void BattleUI::DrawTeamPanel(const BattleTeam& team, bool isPlayer, Vector2 origin) const {
    DrawRectangleRounded({origin.x, origin.y, layout.playerPanel.width, layout.playerPanel.height}, 0.2f, 8.0f, Fade(BLACK, 0.3f));

    const char* sideName = isPlayer ? "PLAYER" : "ENEMY";
    DrawText(sideName, static_cast<int>(origin.x + 10), static_cast<int>(origin.y + 8), 10, WHITE);

    for(int i = 0; i < 3; i++){
        Rectangle slot = isPlayer ? layout.playerSlots[i] : layout.enemySlots[i];
        bool isActive = (team.activeIndex == i);
        bool defeated = team.defeated[i];

        if(isActive){
            DrawRectangleRounded(slot, 0.1f, 4.0f, Fade(YELLOW, 0.15f));
            DrawRectangleLinesEx(slot, 2.0f, YELLOW);
        }
        else{
            DrawRectangleRounded(slot, 0.1f, 4.0f, Fade(DARKGRAY, 0.3f));
            DrawRectangleLinesEx(slot, 1.0f, GRAY);
        }

        if(!team.members[i].name.empty()){
            Color tint = defeated ? GRAY : WHITE;
            if(team.members[i].portrait){
                Rectangle src = {0, 0, 16, 16};
                DrawTexturePro(*team.members[i].portrait, src, slot, Vector2{0, 0}, 0.0f, tint);
            }
            if(defeated){
                DrawText("DEFEATED", static_cast<int>(slot.x), static_cast<int>(slot.y + slot.height + 4), 8, RED);
            }
            else{
                DrawText(team.members[i].name.c_str(), static_cast<int>(slot.x), static_cast<int>(slot.y + slot.height + 4), 8, WHITE);
            }

            if(!defeated && team.members[i].stats.maxHP > 0){
                float hpPercent = (float)team.members[i].stats.HP / (float)team.members[i].stats.maxHP;
                Rectangle miniBar = {slot.x, slot.y + slot.height + 16.0f, slot.width * hpPercent, 4.0f};
                DrawRectangleRec(miniBar, LIME);
                DrawRectangleLinesEx({slot.x, slot.y + slot.height + 16.0f, (float)slot.width, 4.0f}, 1.0f, WHITE);
            }
        }
    }
}

void BattleUI::DrawSkillButtons(const BattleContext& ctx) const {
    for(int i = 0; i < 4; i++){
        if(IsPassive(ctx.player.active().skills[i])) continue;
        Rectangle rec = layout.skillButton;
        rec.x = layout.skillButton.x + (layout.skillButton.width + 12.0f) * i;
        if(ctx.state == BattleState::SelectingSkill && i == selectedSkill) rec.y -= skillButtonOffset[i];

        DrawRectangleRec(rec, Fade(DARKGRAY, 0.6f));
        DrawRectangleLinesEx(rec, 2.0f, (i == selectedSkill) ? YELLOW : WHITE);

        Rectangle iconRec = {rec.x, rec.y, rec.width, rec.height - 16.0f};
        Rectangle src = {0, 0, 32, 32};
        Color tint = ctx.player.active().skills[i].cooldown > 0 ? GRAY : WHITE;
        DrawTexturePro(*ctx.player.active().skills[i].icon, src, iconRec, Vector2{0, 0}, 0.0f, tint);

        bool isSelected = ctx.state == BattleState::SelectingSkill && i == selectedSkill;

        if(!isSelected){
            std::string title = ctx.player.active().skills[i].title;
            int maxTextWidth = rec.width - 8;
            while(MeasureText(title.c_str(), 8) > maxTextWidth && title.length() > 3){
                title.pop_back();
            }
            if(title != ctx.player.active().skills[i].title) title += ".";
            DrawText(title.c_str(), static_cast<int>(rec.x + 4), static_cast<int>(rec.y + rec.height - 14), 8, WHITE);
        }

        if(ctx.player.active().skills[i].cooldown > 0){
            std::string cdText = std::to_string(ctx.player.active().skills[i].cooldown);
            int cdW = MeasureText(cdText.c_str(), 20);
            DrawText(cdText.c_str(), static_cast<int>(rec.x + (rec.width - cdW) / 2), static_cast<int>(rec.y + rec.height / 2.0f - 10), 20, WHITE);
        }

        std::string shortcut = std::to_string(i + 1);
        DrawText(shortcut.c_str(), static_cast<int>(rec.x + rec.width - 12), static_cast<int>(rec.y + 2), 8, YELLOW);

        if(isSelected){
            const float textAreaX = rec.x + rec.width + 20.0f;
            const float textAreaY = rec.y + 8.0f;
            const float maxTextWidth = 320.0f;
            const int titleFontSize = 16;
            const int descFontSize = 12;
            const float padding = 10.0f;
            const float lineSpacing = 4.0f;

            std::string title = ctx.player.active().skills[i].title;
            std::string desc = ctx.player.active().skills[i].desc;

            std::vector<std::string> titleLines;
            {
                std::string line = "";
                for(size_t c = 0; c < title.size(); c++){
                    std::string testLine = line + title[c];
                    if(MeasureText(testLine.c_str(), titleFontSize) > maxTextWidth && !line.empty()){
                        titleLines.push_back(line);
                        line = std::string(1, title[c]);
                    } else {
                        line = testLine;
                    }
                }
                if(!line.empty()) titleLines.push_back(line);
            }

            std::vector<std::string> descLines;
            {
                std::string line = "";
                for(size_t c = 0; c < desc.size(); c++){
                    std::string testLine = line + desc[c];
                    if(MeasureText(testLine.c_str(), descFontSize) > maxTextWidth && !line.empty()){
                        descLines.push_back(line);
                        line = std::string(1, desc[c]);
                    } else {
                        line = testLine;
                    }
                }
                if(!line.empty()) descLines.push_back(line);
            }

            float contentHeight = titleLines.size() * titleFontSize + 
                                  descLines.size() * descFontSize + 
                                  (titleLines.size() + descLines.size() - 1) * lineSpacing;

            float maxLineWidth = 0.0f;
            for(const auto& line : titleLines){
                int w = MeasureText(line.c_str(), titleFontSize);
                if(w > maxLineWidth) maxLineWidth = (float)w;
            }
            for(const auto& line : descLines){
                int w = MeasureText(line.c_str(), descFontSize);
                if(w > maxLineWidth) maxLineWidth = (float)w;
            }

            float boxWidth = maxLineWidth + padding * 2;
            float boxHeight = contentHeight + padding * 2;
            float boxX = textAreaX;
            float boxY = textAreaY;

            DrawRectangleRec({boxX, boxY, boxWidth, boxHeight}, Fade(BLACK, 0.8f));
            DrawRectangleLinesEx({boxX, boxY, boxWidth, boxHeight}, 1.0f, Fade(WHITE, 0.3f));

            float drawY = boxY + padding;
            for(const auto& line : titleLines){
                DrawText(line.c_str(), static_cast<int>(boxX + padding), static_cast<int>(drawY), titleFontSize, WHITE);
                drawY += titleFontSize + lineSpacing;
            }
            if(!titleLines.empty() && !descLines.empty()) drawY += lineSpacing * 0.5f;
            for(const auto& line : descLines){
                DrawText(line.c_str(), static_cast<int>(boxX + padding), static_cast<int>(drawY), descFontSize, Fade(WHITE, 0.8f));
                drawY += descFontSize + lineSpacing;
            }
        }
    }
}

void BattleUI::DrawActiveHUD(const C& c, Vector2 position, const char* label, bool isPlayer) const {
    Rectangle hudName = isPlayer ? layout.playerHudName : layout.enemyHudName;
    Rectangle hudBar = isPlayer ? layout.playerHudBar : layout.enemyHudBar;
    Rectangle hudText = isPlayer ? layout.playerHudText : layout.enemyHudText;

    float hpPercent = (c.stats.maxHP > 0) ? (float)c.stats.HP / (float)c.stats.maxHP : 0.0f;

    DrawText(label, static_cast<int>(hudName.x), static_cast<int>(hudName.y), 10, WHITE);

    DrawRectangleRec(hudBar, GRAY);
    if(hpPercent > 0.0f){
        Rectangle fill = hudBar;
        fill.width *= hpPercent;
        DrawRectangleRec(fill, LIME);
    }
    DrawRectangleLinesEx(hudBar, 1.0f, WHITE);

    std::string hpText = std::to_string(c.stats.HP) + " / " + std::to_string(c.stats.maxHP);
    DrawText(hpText.c_str(), static_cast<int>(hudText.x), static_cast<int>(hudText.y), 10, WHITE);

    for(const auto& buff : c.stats.activeBuffs){
        Color col = (buff.delta > 0) ? GREEN : RED;
        std::string buffText = (buff.delta > 0 ? "+" : "") + buff.stat + (buff.label.empty() ? "" : ("(" + buff.label + ")"));
        DrawText(buffText.c_str(), static_cast<int>(hudBar.x + hudBar.width + 4), static_cast<int>(hudBar.y), 8, col);
    }
}

std::array<Rectangle, 3> BattleUI::GetSwitchPortraitRects(const BattleTeam& team) const {
    std::array<Rectangle, 3> rects = {};
    rects.fill({0, 0, 0, 0});
    int availIdx = 0;
    for(int i = 0; i < 3; i++){
        if(team.activeIndex == i) continue;
        if(team.defeated[i]) continue;
        if(!team.IsAvailable(i)) continue;

        float offsetX = 0.0f;
        if(availIdx == 0) offsetX = -72.0f;
        else if(availIdx == 1) offsetX = 72.0f;

        rects[availIdx] = {layout.characterSelector.x + layout.characterSelector.width / 2.0f - 32.0f + offsetX,
                           layout.characterSelector.y + (layout.characterSelector.height - 64.0f) / 2.0f,
                           64.0f, 64.0f};
        availIdx++;
    }
    return rects;
}

void BattleRenderer::Draw(const BattleContext& ctx, const AttackAnimation& attack, const SwitchAnimation& switchAnim, const DeathAnimation& deathAnim) const{
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    Rectangle src = {0, 0, 16, 16};
    float fieldCenterX = sW / 2.0f;
    float fieldCenterY = sH / 2.0f + 250.0f;
    Rectangle enemyDest = {fieldCenterX + 175.0f, fieldCenterY, 128, 128};

    bool playerShaking = attack.active && attack.kind == AttackAnimation::Kind::Attack && attack.target == &ctx.player.active();
    bool enemyShaking = attack.active && attack.kind == AttackAnimation::Kind::Attack && attack.target == &ctx.enemy.active();

    float playerShakeX = 0.0f, enemyShakeX = 0.0f;
    if(playerShaking){
        float t = attack.shakeTime / AttackAnimation::SHAKE_DUR;
        playerShakeX = sinf(attack.shakeTime * 60.0f) * 8.0f * (1.0f - t);
    }
    if(enemyShaking){
        float t = attack.shakeTime / AttackAnimation::SHAKE_DUR;
        enemyShakeX = sinf(attack.shakeTime * 60.0f) * 8.0f * (1.0f - t);
    }

    float enemyOffX = 0.0f;
    if(attack.active && attack.attacker == &ctx.enemy.active()) enemyOffX = attack.attackerHome.x - (fieldCenterX + 175.0f);

    enemyDest.x += enemyShakeX + enemyOffX;

    float playerOffX = 0.0f;
    if(attack.active && attack.attacker == &ctx.player.active()) playerOffX = attack.attackerHome.x - (fieldCenterX - 175.0f);

    Color enemyTint = enemyShaking ? RED : WHITE;

    float enemyScale = 1.0f;
    bool enemyBuff = false;
    if(attack.active && attack.kind == AttackAnimation::Kind::Buff){
        float t = attack.buffTime / AttackAnimation::BUFF_DUR;
        float pulse = sinf(t * PI) * 0.18f;
        if(attack.target == &ctx.enemy.active()){ enemyScale = 1.0f + pulse; enemyBuff = true; }
    }
    if(enemyBuff && enemyTint.r == WHITE.r && enemyTint.g == WHITE.g && enemyTint.b == WHITE.b) enemyTint = BLUE;

    auto scaleDest = [](Rectangle r, float s) -> Rectangle {
        float cx = r.x + r.width / 2.0f;
        float cy = r.y + r.height / 2.0f;
        return { cx - (r.width * s) / 2.0f, cy - (r.height * s) / 2.0f, r.width * s, r.height * s };
    };
    Rectangle enemyDraw = scaleDest(enemyDest, enemyScale);

    if(switchAnim.active){
        DrawRectangle(0, 0, sW, sH, Fade(BLACK, 0.6f));
        float t = switchAnim.progress;
        float ease = t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        float offsetY = ease * 256.0f;
        float switchOutY = fieldCenterY + offsetY;
        float switchInY = fieldCenterY - 256.0f + offsetY;
        float switchOutAlpha = switchOutY > fieldCenterY + 128.0f ? 0.0f : 1.0f;
        float switchInAlpha = switchInY > fieldCenterY ? 0.0f : 1.0f;
        Rectangle switchOutDest = {fieldCenterX - 175.0f, switchOutY, 128.0f, 128.0f};
        Rectangle switchInDest = {fieldCenterX - 175.0f, switchInY, 128.0f, 128.0f};
        Color switchOutTint = {255, 255, 255, (unsigned char)(switchOutAlpha * 255)};
        Color switchInTint = {255, 255, 255, (unsigned char)(switchInAlpha * 255)};
        DrawTexturePro(*ctx.player.members[switchAnim.switchOutIndex].portrait, src, switchOutDest, Vector2{0, 0}, 0.0f, switchOutTint);
        DrawTexturePro(*ctx.player.members[switchAnim.switchInIndex].portrait, src, switchInDest, Vector2{0, 0}, 0.0f, switchInTint);
    }
    else if(deathAnim.active && deathAnim.isPlayer){
        DrawRectangle(0, 0, sW, sH, Fade(BLACK, 0.6f));
        const float slotW = 64.0f, slotGap = 12.0f;
        float slotStartY = 40.0f;
        float playerSlotX = 58.0f;
        for(int i = 0; i < 3; i++){
            if(ctx.player.defeated[i] && i != deathAnim.deadIndex) continue;
            Rectangle rec = {playerSlotX, slotStartY + (slotW + slotGap) * i, slotW, slotW};
            float alpha = 1.0f;
            if(i == deathAnim.deadIndex){
                if(deathAnim.sliding) continue;
                alpha = 1.0f - deathAnim.fade;
            }
            else if(deathAnim.sliding && i > deathAnim.deadIndex){
                rec.x -= (slotGap + slotW) * deathAnim.slide;
            }
            Color tint = {255, 255, 255, (unsigned char)(alpha * 255)};
            DrawTexturePro(*ctx.player.members[i].portrait, {0, 0, 16, 16}, rec, Vector2{0, 0}, 0.0f, tint);
            DrawRectangleLinesEx(rec, 1.5f, GRAY);
        }
    }
    else if(deathAnim.active && !deathAnim.isPlayer){
        DrawRectangle(0, 0, sW, sH, Fade(BLACK, 0.6f));
        const float slotW = 64.0f, slotGap = 12.0f;
        float slotStartY = 40.0f;
        float enemySlotX = (float)sW - 102.0f;
        for(int i = 0; i < 3; i++){
            if(ctx.enemy.defeated[i] && i != deathAnim.deadIndex) continue;
            Rectangle rec = {enemySlotX, slotStartY + (slotW + slotGap) * i, slotW, slotW};
            float alpha = 1.0f;
            if(i == deathAnim.deadIndex){
                if(deathAnim.sliding) continue;
                alpha = 1.0f - deathAnim.fade;
            }
            else if(deathAnim.sliding && i > deathAnim.deadIndex){
                rec.x -= (slotGap + slotW) * deathAnim.slide;
            }
            Color tint = {255, 255, 255, (unsigned char)(alpha * 255)};
            DrawTexturePro(*ctx.enemy.members[i].portrait, {0, 0, 16, 16}, rec, Vector2{0, 0}, 0.0f, tint);
            DrawRectangleLinesEx(rec, 1.5f, GRAY);
        }
    }
    else{
        Rectangle playerDest = {fieldCenterX - 175.0f, fieldCenterY, 128, 128};
        playerDest.x += playerShakeX + playerOffX;
        Color playerTint = playerShaking ? RED : WHITE;
        float playerScale = 1.0f;
        bool playerBuff = false;
        if(attack.active && attack.kind == AttackAnimation::Kind::Buff){
            float t = attack.buffTime / AttackAnimation::BUFF_DUR;
            float pulse = sinf(t * PI) * 0.18f;
            if(attack.target == &ctx.player.active()){ playerScale = 1.0f + pulse; playerBuff = true; }
        }
        if(playerBuff && playerTint.r == WHITE.r && playerTint.g == WHITE.g && playerTint.b == WHITE.b) playerTint = BLUE;
        Rectangle playerDraw = scaleDest(playerDest, playerScale);
        DrawTexturePro(*ctx.player.active().portrait, src, playerDraw, Vector2{0, 0}, 0.0f, playerTint);

        DrawTexturePro(*ctx.enemy.active().portrait, src, enemyDraw, Vector2{0, 0}, 0.0f, enemyTint);

        if(attack.active && attack.kind == AttackAnimation::Kind::Attack && attack.damageTime > 0.0f){
            float t = attack.damageTime / AttackAnimation::DAMAGE_DUR;
            float alpha = 1.0f;
            float rise = 0.0f;
            if(t < 0.25f){
                alpha = t / 0.25f;
            }
            else if(t > 0.75f){
                alpha = (1.0f - t) / 0.25f;
            }
            rise = t * 60.0f;
            int txtSize = attack.isCrit ? 40 : 30;
            std::string dmgStr = std::to_string(attack.pendingDamage);
            int tw = MeasureText(dmgStr.c_str(), txtSize);
            float baseX = (attack.target == &ctx.player.active()) ? (fieldCenterX - 175.0f) : (fieldCenterX + 175.0f);
            float baseY = (attack.target == &ctx.player.active()) ? fieldCenterY : fieldCenterY;
            float x = baseX + (128.0f - tw) / 2.0f;
            float y = baseY - 40.0f - rise;
            DrawText(dmgStr.c_str(), static_cast<int>(x), static_cast<int>(y), txtSize, Fade(RED, alpha));
        }
    }
}


BattleScene::BattleScene(){
    int playerIdx = 0;
    for(const auto &ch : gameData.characters){
        if(ch.isEnemy) continue;
        if(playerIdx >= 3) break;
        battle.player.members[playerIdx].name = ch.name;
        battle.player.members[playerIdx].portrait = ch.portrait;
        battle.player.members[playerIdx].skills = ch.skills;
        battle.player.members[playerIdx].stats = ch.stats;
        if(battle.player.members[playerIdx].stats.maxHP <= 0)
            battle.player.members[playerIdx].stats.maxHP = battle.player.members[playerIdx].stats.HP;
        playerIdx++;
    }
    int rosterCount = 0;
    for(int i = 0; i < 3; i++){
        if(!gameData.inBattleCharacters[i]) continue;
        battle.enemy.members[i].name = gameData.inBattleCharacters[i]->name;
        battle.enemy.members[i].portrait = gameData.inBattleCharacters[i]->portrait;
        battle.enemy.members[i].skills = gameData.inBattleCharacters[i]->skills;
        battle.enemy.members[i].stats = gameData.inBattleCharacters[i]->stats;
        if(battle.enemy.members[i].stats.maxHP <= 0)
            battle.enemy.members[i].stats.maxHP = battle.enemy.members[i].stats.HP;
        rosterCount++;
    }
    if(rosterCount > 0){
        int start = GetRandomValue(0, rosterCount - 1);
        int seen = 0;
        for(int i = 0; i < 3; i++){
            if(!battle.enemy.members[i].name.empty() && seen++ == start){
                battle.enemy.activeIndex = i;
                break;
            }
        }
    }
    background = gameData.res.GetTexture("battle_map");
    battle.background = background;
    if(background){
        SetTextureFilter(*background, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(*background, TEXTURE_WRAP_CLAMP);
    }
    ShowCursor();
    battle.currentTurn = BattleSide::Player;

    if(Music* gameBg = gameData.res.GetMusic("game_background_music")) StopMusicStream(*gameBg);
    battleMusic = gameData.res.GetMusic("game_battle_music");
    if(battleMusic){
        PlayMusicStream(*battleMusic);
        battleMusic->looping = true;
    }
}

BattleScene::~BattleScene(){
    if(battleMusic) StopMusicStream(*battleMusic);
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
    svm.BeginAction();
    if(battle.pendingAction.bActType == BattleActionType::Switch){
        switchAnim.active = true;
        switchAnim.switchOutIndex = battle.pendingAction.switchOutIndex;
        switchAnim.switchInIndex = battle.player.activeIndex;
        switchAnim.progress = 0.0f;
        battle.state = BattleState::SwitchAnimating;
    }
    else{
        battle.pendingAction.actor = (battle.currentTurn == BattleSide::Player) ? &battle.player.active() : &battle.enemy.active();
        if(battle.pendingAction.skill->target == "self") battle.pendingAction.target = battle.pendingAction.actor;
        else if(battle.pendingAction.skill->target == "enemy_single") battle.pendingAction.target = (battle.currentTurn == BattleSide::Player) ? &battle.enemy.active() : &battle.player.active();
        battle.state = BattleState::ExecutingAnimation;
    }
}

void BattleScene::UpdateAnimation(float dt){
    if(!attack.active) return;
    if(attack.kind == AttackAnimation::Kind::Buff){
        attack.buffTime += dt;
        if(attack.buffTime >= AttackAnimation::BUFF_DUR){
            attack.active = false;
            battle.state = BattleState::EndTurn;
        }
        return;
    }
    attack.lungeTime += dt;
    if(attack.lungeTime >= AttackAnimation::LUNGE_DUR){
        attack.shakeTime += dt;
        if(attack.shakeTime >= AttackAnimation::SHAKE_DUR)
            attack.damageTime += dt;
    }

    Vector2 targetPos = (attack.target == &battle.player.active()) ? Vector2{GetScreenWidth() / 2.0f - 175, GetScreenHeight() / 2.0f} : Vector2{GetScreenWidth() / 2.0f + 175, GetScreenHeight() / 2.0f};
    Vector2 homePos = attack.attackerOrigin;
    Vector2 lungePos = {(homePos.x + targetPos.x) / 2.0f, homePos.y};
    if(attack.lungeTime < AttackAnimation::LUNGE_DUR){
        float t = attack.lungeTime / AttackAnimation::LUNGE_DUR;
        attack.attackerHome.x = Lerp(homePos.x, lungePos.x, t * t * (3.0f - 2.0f * t));
    }
    else if(attack.shakeTime < AttackAnimation::SHAKE_DUR){
        float t = attack.shakeTime / AttackAnimation::SHAKE_DUR;
        attack.attackerHome.x = Lerp(lungePos.x, homePos.x, t * t * (3.0f - 2.0f * t));
    }
    else{
        attack.attackerHome.x = homePos.x;
    }

    if(attack.damageTime >= AttackAnimation::DAMAGE_DUR){
        attack.active = false;
        battle.state = BattleState::EndTurn;
    }
}

void BattleScene::CheckDefeats(){
    auto process = [&](BattleTeam& team, bool isPlayer){
        for(int i = 0; i < 3; i++){
            if(!team.defeated[i] && team.members[i].stats.HP <= 0){
                team.defeated[i] = true;
                if(!isPlayer && !team.members[i].name.empty())
                    battle.defeatedEnemyNames.push_back(team.members[i].name);
            }
        }
        if(!team.defeated[team.activeIndex]) return;
        int next = isPlayer ? team.FirstAvailable() : team.RandomAvailable();
        if(!isPlayer && next < 0 && battle.state == BattleState::EndTurn){
            battle.state = BattleState::Victory;
            return;
        }
        deathAnim.active = true;
        deathAnim.isPlayer = isPlayer;
        deathAnim.deadIndex = team.activeIndex;
        deathAnim.nextIndex = next;
        deathAnim.fade = 0.0f;
        deathAnim.slide = 0.0f;
        deathAnim.sliding = false;
        battle.state = BattleState::CharacterFading;
    };
    process(battle.player, true);
    process(battle.enemy, false);
}

void BattleScene::EndTurn(){
    svm.CooldownAndDurationManager(battle);
    CheckDefeats();
    if(battle.state == BattleState::Defeat || battle.state == BattleState::Victory || battle.state == BattleState::CharacterFading) return;
    FinishTurnTransition();
}

void BattleScene::FinishTurnTransition(){
    battle.currentTurn = (battle.currentTurn == BattleSide::Player) ? BattleSide::Enemy : BattleSide::Player;
    battle.turnNumber++;
    battle.battleStats.turnsElapsed = battle.turnNumber;
    battle.state = BattleState::TurnStart;
    ui.StartTurnTransition();
}

Scene* BattleScene::Update(float dt){
    if(battleMusic) UpdateMusicStream(*battleMusic);
    HandleInput(dt);
    switch(battle.state){
    case BattleState::Entering:
        ui.Update(battle, dt);
        if(battle.state == BattleState::TurnStart) ui.StartTurnTransition();
        if(battle.battleStats.turnsElapsed == 0){
            battle.battleStats = BattleStats{};
            battle.battleStats.turnsElapsed = 1;
        }
        break;
        case BattleState::BeginTurn:
            battle.state = BattleState::WaitingForInput;
            break;
        case BattleState::TurnStart:
            ui.Update(battle, dt);
            break;
        case BattleState::ExecutingSkill:
            ExecuteAction();
            break;
        case BattleState::SwitchAnimating:
            if(!switchAnim.active) break;
            switchAnim.progress += dt / SwitchAnimation::DURATION;
            if(switchAnim.progress >= 1.0f){
                switchAnim.progress = 1.0f;
                switchAnim.active = false;
                battle.state = BattleState::EndTurn;
            }
            break;
        case BattleState::CharacterFading:
            if(!deathAnim.active) break;
            if(!deathAnim.sliding){
                deathAnim.fade += dt / DeathAnimation::FADE_DUR;
                if(deathAnim.fade >= 1.0f){
                    deathAnim.fade = 1.0f;
                if(deathAnim.nextIndex < 0){
                    deathAnim.active = false;
                    battle.state = deathAnim.isPlayer ? BattleState::Defeat : BattleState::Victory;
                }
                    else{
                        deathAnim.sliding = true;
                    }
                }
            }
            else{
                deathAnim.slide += dt / DeathAnimation::SLIDE_DUR;
                if(deathAnim.slide >= 1.0f){
                    deathAnim.slide = 1.0f;
                    deathAnim.active = false;
                    if(deathAnim.isPlayer){
                        battle.player.activeIndex = deathAnim.nextIndex;
                    }
                    else{
                        battle.enemy.activeIndex = deathAnim.nextIndex;
                    }
                    FinishTurnTransition();
                }
            }
            break;
    case BattleState::ExecutingAnimation:
            if(!attack.active){
                attack.active = true;
                bool hasDamage = false;
                for(const auto &mod : battle.pendingAction.skill->modifiers){
                    if(mod.type == "damage"){ hasDamage = true; break; }
                }
                attack.kind = hasDamage ? AttackAnimation::Kind::Attack : AttackAnimation::Kind::Buff;
                attack.attacker = battle.pendingAction.actor;
                attack.target = battle.pendingAction.target;
                attack.attackerHome = (attack.attacker == &battle.player.active()) ? Vector2{GetScreenWidth() / 2.0f - 175, GetScreenHeight() / 2.0f} : Vector2{GetScreenWidth() / 2.0f + 175, GetScreenHeight() / 2.0f};
                attack.attackerOrigin = attack.attackerHome;
                attack.lungeTime = 0.0f;
                attack.shakeTime = 0.0f;
                attack.damageTime = 0.0f;
                attack.buffTime = 0.0f;
                attack.applied = false;
                attack.pendingDamage = svm.ComputeDamage(&battle.pendingAction.actor->stats, &battle.pendingAction.target->stats, battle.pendingAction.skill);
                attack.isCrit = svm.lastWasCrit;
            }
            UpdateAnimation(dt);
            if(!attack.active && !attack.applied){
                int targetHPBefore = battle.pendingAction.target->stats.HP;
                svm.Resolve(&battle.pendingAction.actor->stats, &battle.pendingAction.target->stats,
                            battle.pendingAction.skill, &battle.pendingAction.actor->skills, true);
                battle.pendingAction.skill->cooldown = battle.pendingAction.skill->baseCooldown;
                attack.applied = true;

                battle.battleStats.skillsUsed++;
                if(attack.isCrit) battle.battleStats.criticalHits++;
                if(attack.pendingDamage > 0){
                    if(battle.currentTurn == BattleSide::Player){
                        battle.battleStats.totalDamageDealt += attack.pendingDamage;
                    }else{
                        battle.battleStats.totalDamageReceived += attack.pendingDamage;
                    }
                }
                int targetHPAfter = battle.pendingAction.target->stats.HP;
                int hpDelta = targetHPAfter - targetHPBefore;
                if(hpDelta > 0) battle.battleStats.totalHealingDone += hpDelta;
            }
            break;
        case BattleState::EndTurn:
            EndTurn();
            break;
        case BattleState::Victory:
        case BattleState::Defeat:
            if(!transitioning){
                AwardRewards();
                transitioning = true;
                transitionTimer = 0.0f;
            }
            transitionTimer += dt;
            if(transitionTimer >= 0.5f){
                return new BattleResultScene(battle, battle.state == BattleState::Victory);
            }
            break;
    }
    return this;
}

void BattleScene::Draw(){
    ClearBackground(BLACK);
    if(background){
        const float sW = (float)GetScreenWidth();
        const float sH = (float)GetScreenHeight();
        const float texW = (float)background->width;
        const float texH = (float)background->height;
        const float screenAspect = sW / sH;
        const float texAspect = texW / texH;

        Rectangle src;
        if(texAspect > screenAspect){
            src.height = texH;
            src.width = texH * screenAspect;
            src.x = (texW - src.width) / 2.0f;
            src.y = 0.0f;
        }
        else{
            src.width = texW;
            src.height = texW / screenAspect;
            src.x = 0.0f;
            src.y = (texH - src.height) / 2.0f;
        }
        Rectangle dst = {0.0f, 0.0f, sW, sH};
        DrawTexturePro(*background, src, dst, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }
    else{
        ClearBackground(GREEN);
    }
    ui.Draw(battle);
    renderer.Draw(battle, attack, switchAnim, deathAnim);
}

void BattleScene::AwardRewards(){
    if(battle.state != BattleState::Victory) return;

    BattleReport report;
    report.victory = true;
    report.defeatedEnemyNames = battle.defeatedEnemyNames;
    gameData.OnBattleResolved(report);

    battle.rewards = {100, 10, 50};
    std::vector<Rewards> r;
    std::string coinId = "coins_heap";
    std::string crystalId = "crystal";
    std::string expId = "exp";
    const InventoryItem* coinItem = gameData.inventory.GetItem(coinId);
    if(coinItem) r.push_back({coinItem, battle.rewards.coins});
    const InventoryItem* crystalItem = gameData.inventory.GetItem(crystalId);
    if(crystalItem) r.push_back({crystalItem, battle.rewards.crystals});
    const InventoryItem* expItem = gameData.inventory.GetItem(expId);
    if(expItem) r.push_back({expItem, battle.rewards.exp});
    gameData.inventory.AddToInventory(r);
}
