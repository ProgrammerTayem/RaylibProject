#include "gameScene.h"
#include "gameData.h"
#include "battleScene.h"
#include "menuScene.h"
#include "raygui.h"
#include <raymath.h>
#include <fstream>
#include <ctime>
#include <algorithm>

Typewriter typeWriter;

static void PlayButtonSfx(GameData &data) {
    if(Sound *sfx = data.res.GetSfx("button_click")) PlaySound(*sfx);
}

static void WriteErrorLog(const std::string& message) {
    std::ofstream logFile("ErrorLog", std::ios::app);
    if (logFile.is_open()) {
        std::time_t now = std::time(nullptr);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        logFile << "[" << timestamp << "] " << message << '\n';
    }
}

static std::string WrappedText(const std::string& text, float fontSize, float maxWidth, float spacing, float *height = nullptr, Font font = GuiGetFont()){
    std::string result, line, word;

    for(size_t i = 0; i <= text.size(); i++){

        char c = (i < text.size()) ? text[i] : ' ';
        if(c == ' ' || c == '\n'){

            std::string testLine = line + word + " ";
            Vector2 size = MeasureTextEx(font, testLine.c_str(), fontSize, spacing);

            if(size.x > maxWidth && !line.empty()){
                result += line + '\n';
                line = word + " ";
            }
            else line = testLine;

            word.clear();

            if(c == '\n'){
                result += line + '\n';
                line.clear();
            }
        }
        else word += c;
    }

    result += line;
    if(height != nullptr) *height = fontSize * (std::count(result.begin(), result.end(), '\n') + 1);
    return result;
}

inline bool IsInInteractionRange(const Vector2& a, const Vector2& b){
    return Vector2Distance(a, b) < 2*TILE_SIZE;
}

void Typewriter::TypeWriterUpdate(const std::string& text, float dt){
    if(text != currentText){
        currentText = text;
        charIndex = 0;
        timer = 0.0f;
        typewriterFinished = false;
    }
    
    timer += dt;
    if(timer >= 0.05f && charIndex < text.size()){
        charIndex++;
        timer = 0.0f;
    }
    
    renderingPart = text.substr(0, charIndex);
    if(charIndex >= text.size()){
        typewriterFinished = true;
    }
}

void Typewriter::Draw() const{
    if(renderingPart.empty() || !isActive) return;
    int boxWidth = GetScreenWidth(), boxHeight = 200;
    int x = 0, y = GetScreenHeight() - boxHeight;
    DrawRectangle(x, y, boxWidth, boxHeight, DARKGRAY);
    DrawRectangleLines(x, y, boxWidth, boxHeight, WHITE);
    DrawTextEx(GuiGetFont(), renderingPart.c_str(), Vector2(x + 20, y + 20), 30, 1.0f ,WHITE);
}

OverlayUI::OverlayUI(GameData &data) : GUI(false, data, true){
    charactersIcon = data.res.GetTexture("char_icon");
    dailyTasksIcon = data.res.GetTexture("tasks_icon");
    questsIcon = data.res.GetTexture("quests_icon");
    inventoryIcon = data.res.GetTexture("inventory_icon");
    HideCursor();
}

void OverlayUI::TransitionBreatheInOut(Rectangle dest, Color peak){
    float factor = dest.width/transitionLife;
    DrawRectangleGradientH(factor*time, dest.y, factor/2, dest.height, Fade(WHITE, 0.1), Fade(WHITE, 0.3));
    DrawRectangleGradientH(factor*time + factor/2, dest.y, factor/2, dest.height, Fade(WHITE, 0.3), Fade(WHITE, 0.1));

}


GUI* OverlayUI::Update(float dt){
    if(IsKeyPressed(KEY_P)) return new PauseUI(gameData);
    if(typeWriter.isActive) return this;
    if(gameData.newQuest != nullptr){
        time += dt;
        if(time >= transitionLife){
            time = 0.0f;
            gameData.newQuest = nullptr;
        }
    }
    if(IsKeyPressed(KEY_I)) return new InventoryUI(gameData);
    if(IsKeyPressed(KEY_Q)) return new QuestsUI(gameData);
    if(IsKeyPressed(KEY_T)) return new DailyTasksUI(gameData);
    if(IsKeyPressed(KEY_C)) return new CharacterUI(gameData);
    return this;
}

void OverlayUI::Draw(){
    if(typeWriter.isActive) return;
    int x = GetScreenWidth() - 400, y = 25;
    if(charactersIcon) DrawTexture(*charactersIcon, x, y, WHITE);
    if(dailyTasksIcon) DrawTexture(*dailyTasksIcon, x + 100, y, WHITE);
    if(questsIcon) DrawTextureEx(*questsIcon, (Vector2){ x + 200, y }, 0.0f, 2.0f, WHITE);
    if(inventoryIcon) DrawTextureEx(*inventoryIcon, (Vector2){ x + 300, y }, 0.0f, 2.0f, WHITE);

    if(gameData.newQuest != nullptr){
        GuiDrawIcon(ICON_STAR, 50, 150, 1, YELLOW);
        std::string t = gameData.newQuest->questTitle;
        DrawTextEx(GuiGetFont(), "NEW QUEST: ", Vector2(85.0f, 150.0f), 25, 2.0f, DARKBLUE);
        DrawTextEx(GuiGetFont(), t.c_str(), Vector2(85.0f, 180.0f), 25, 2.0f, WHITE);
        TransitionBreatheInOut((Rectangle){45, 140, 250, 80}, WHITE);
    }
    else if(gameData.trackingQuest != nullptr){
        GuiDrawIcon(ICON_STAR, 50, 150, 1, YELLOW);
        std::string t = gameData.trackingQuest->questTitle;
        DrawTextEx(GuiGetFont(), "CURRENT QUEST: ", Vector2(85.0f, 150.0f), 25, 2.0f, YELLOW);
        DrawTextEx(GuiGetFont(), t.c_str(), Vector2(85.0f, 180.0f), 25, 2.0f, WHITE);
        if(gameData.trackingQuest->complete){
            DrawTextEx(GuiGetFont(), "COMPLETED! ", Vector2(85.0f, 210.0f), 25, 2.0f, GREEN);
        }
    }

    for(int i = 0; i < 4; i++){
        DrawRectangle(x + i * 100, y + 40, 35, 25, Fade(GRAY, 0.8f));
        DrawText((char[2]){shortCutKey[i], '\0'}, x + i * 100 + 10, y + 45, 20, WHITE);
    }
}

InventoryUI::InventoryUI(GameData &data) : GUI(true, data), crossClicked(false), showDesc(-1), clickedTab(-1), selectedTab(ItemCategory::ALL){
    panels = data.res.GetTexture("inventory_panel_icon");
    tabText = {
        "ALL", "QUEST ITEMS", "CRAFTING", "POTIONS", "OTHERS"
    };
    ShowCursor();
}

GUI* InventoryUI::Update(float){
    if(IsKeyPressed(KEY_X) || crossClicked){
        PlayButtonSfx(gameData);
        return new OverlayUI(gameData);
    }
    if(IsKeyPressed(KEY_E)){
        showDesc = -1;
        selectedTab = static_cast<Tabs>((static_cast<int>(selectedTab)+1)%5);
        PlayButtonSfx(gameData);
    }
    else if(IsKeyPressed(KEY_Q)){
        showDesc = -1;
        if(selectedTab == static_cast<Tabs>(0)) selectedTab = static_cast<Tabs>(4);
        else selectedTab = static_cast<Tabs>((static_cast<int>(selectedTab)-1));
        PlayButtonSfx(gameData);
    }
    if(clickedTab > -1){
        showDesc = -1;
        selectedTab = static_cast<Tabs>(clickedTab);
        clickedTab = -1;
        PlayButtonSfx(gameData);
    }
    return this;
}

void InventoryUI::Draw(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    DrawRectangleGradientV(0, 0, sW, sH, BLACK, DARKPURPLE);
    DrawTextEx(GuiGetFont(), "INVENTORY", Vector2(100.0f, 40.0f), 45, 2.0f, WHITE);
    GuiDrawIcon(ICON_CROSS, sW-150, 40, 3, WHITE);
    crossClicked = static_cast<bool>(GuiLabelButton(Rectangle(sW-150, 40, 1.5*TILE_SIZE, 1.5*TILE_SIZE), nullptr));
    GuiLine((Rectangle){0, 115, sW, 2.0f}, nullptr);
    DrawText("[Q]", 120, 135, 25, YELLOW);
    GuiDrawIcon(ICON_ARROW_LEFT, 90, 130, 2, YELLOW);
    DrawLineV(Vector2(280, 115), Vector2(280, 185), GRAY);
    GuiLine((Rectangle){0, 185, sW, 2.0f}, nullptr);
    DrawText("[E]", sW - 120, 135, 25, YELLOW);
    GuiDrawIcon(ICON_ARROW_RIGHT, sW - 90, 130, 2, YELLOW);
    DrawLineV(Vector2(sW - 280, 115), Vector2(sW - 280, 185), GRAY);
    for(int i = 0; i < tabText.size(); i++){
        float x = 280 + (sW/7.0f)*i, y = 115;
        float h = 70, w = sW/7.0f;
        if(static_cast<Tabs>(i) == selectedTab){
            DrawRectangle(x, y, w, h, GRAY);
        }
        DrawTextEx(GuiGetFont(), tabText[i].c_str(), Vector2(x + 20, y + 20), 25, 1.5f, WHITE);
        DrawLineV(Vector2(x, y), Vector2(x, y + 65), GRAY);
        if(static_cast<bool>(GuiLabelButton((Rectangle){x, y, w, h}, nullptr))) clickedTab = i;
    }
    Rectangle source = {
        .x = 612,
        .y = 0,
        .width = 65.0f,
        .height = 65.0f
    };

    if(panels){
        auto it = gameData.inventory.slots.begin();
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                if(it == gameData.inventory.slots.end()) break;
                if(selectedTab != ItemCategory::ALL && it->first->category != selectedTab) continue; 
                int linear_index = i*rows + j;
                Rectangle dest = {
                    .x = 100 + 2*TILE_SIZE*j,
                    .y = 300 + 2*TILE_SIZE*i,
                    .width = 2*TILE_SIZE,
                    .height = 2*TILE_SIZE
                };
                DrawTexturePro(*panels, source, dest, (Vector2){0.0, 0.0}, 0.0f, WHITE);
                DrawTextureEx(*(it->first->icon), Vector2(dest.x + 10, dest.y + 10), 0.0f, 1.3f, WHITE);
                const char* amt = std::to_string(it->second).c_str();
                Vector2 text = MeasureTextEx(GuiGetFont(), amt, 16, 1.0f);
                float textX = dest.x + 2*TILE_SIZE - 12 - text.x, textY = dest.y + 2*TILE_SIZE - 5 - text.y;
                DrawTextEx(GuiGetFont(), amt, Vector2(textX, textY), 16.0f, 1.0f, WHITE);
                if(static_cast<bool>(GuiLabelButton(dest, nullptr))) showDesc = linear_index;
                if(showDesc >= 0 && linear_index == showDesc){
                    float _temp = 0, _T;
                    DrawLineV(Vector2(1450, 185), Vector2(1450, sH), GRAY);
                    std::string text = WrappedText(it->first->name, 25, 400, 1.5, &_temp);
                    DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1480, 250), 25, 1.5, WHITE);
                    DrawTextureEx(*(it->first->icon), Vector2(1480, 250 + _temp + 30), 4.0f, 1.5f, WHITE);
                    _T = _temp + 250 + 30;
                    text = WrappedText("Quantity: " + std::to_string(it->second), 25, 400, 1.5, &_temp);
                    DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1480, _T + 60 + 10), 25, 1.5, GREEN);
                    _T = _T + 60 + 10 + _temp;
                    text = WrappedText(it->first->description, 25, 400, 1.5, &_temp);
                    DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1480, _T + 15), 25, 1.5, WHITE);
                }
                else if(showDesc < 0){
                    DrawLineV(Vector2(1450, 185), Vector2(1450, sH), GRAY);
                    DrawTextEx(GuiGetFont(), "[?]", Vector2(1620, 410), 160, 1.5f, Fade(GRAY, 0.5));
                    std::string text = WrappedText("SELECT AN ITEM TO GAIN ADDITIONAL INFO", 35, 400, 1.5);
                    DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1490, 610), 35, 1.5, Fade(GRAY, 0.5));
                }
                ++it;
            }
            if(it == gameData.inventory.slots.end()) break;
        }
    }
}

QuestsUI::QuestsUI(GameData &data) : GUI(true, data), crossClicked(false), trackClicked(false), collectClicked(false), selectedQuest(-1), track(-1){
    panels = data.res.GetTexture("inventory_panel_icon");
    ShowCursor();
}

GUI* QuestsUI::Update(float){
    if(IsKeyPressed(KEY_X) || crossClicked){
        PlayButtonSfx(gameData);
        return new OverlayUI(gameData);
    }

    if(trackClicked){
        if(gameData.trackingQuest != nullptr){
            gameData.trackingQuest = nullptr;
            track = -1;
        }
        else{
            track = selectedQuest;
            gameData.trackingQuest = &gameData.quests[track];
        }
        PlayButtonSfx(gameData);
        trackClicked = false;
    }

    if(collectClicked){
        gameData.inventory.AddToInventory(gameData.trackingQuest->rewards);
        gameData.trackingQuest->rewardCollected = true;
        gameData.trackingQuest = nullptr;
        track = -1;
        PlaySound(*(gameData.res.GetSfx("reward_collect")));
        collectClicked = false;
    }
    return this;
}

void QuestsUI::Draw(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    ClearBackground(Fade(BLACK, 0.1f));
    DrawTextEx(GuiGetFont(), "QUESTS", Vector2(100.0f, 40.0f), 45, 2.0f, WHITE);
    crossClicked = static_cast<bool>(GuiLabelButton(Rectangle(sW-150, 40, 1.5*TILE_SIZE, 1.5*TILE_SIZE), nullptr));
    GuiDrawIcon(ICON_CROSS, sW-150, 40, 3, WHITE);
    GuiLine((Rectangle){0, 115, sW, 2.0f}, nullptr);

    Vector2 scroll = {0, 0};
    Rectangle view;
    Rectangle bounds = {100, 150, 450, 800};
    Rectangle content = {
        .x = 0,
        .y = 0,
        .width = bounds.width,
        .height = gameData.quests.size() * 80 + 40
    };
    GuiScrollPanel(bounds, "", content, &scroll, &view);
    DrawRectangleRec(bounds, BLACK);

    BeginScissorMode(view.x, view.y, view.width, view.height);

    int available = 0;

    for (int i = 0; i < static_cast<int>(gameData.quests.size()); i++){
        if(gameData.quests[i].locked || (gameData.quests[i].complete && gameData.quests[i].rewardCollected)) continue;
        available++;

        float x = bounds.x, y = bounds.y + scroll.y + i * 80 + 20, w = bounds.width, h = 80.0f;
        if(i == track){
            DrawRectangle(x, y, w, h, DARKBLUE);
        }
        else if(gameData.quests[i].complete) DrawRectangle(x, y, w, h, GREEN);
        else if(i == selectedQuest) DrawRectangle(x, y, w, h, GRAY);
        else GuiLine((Rectangle){x, y + 80, w, 2}, nullptr);
        DrawTextEx(GuiGetFont(), WrappedText(gameData.quests[i].questTitle, 35, w, 1.5f).c_str(), Vector2(x + 10, y + 30), 35, 1.5f, WHITE);
        if(static_cast<bool>(GuiLabelButton((Rectangle){x, y, w, h}, nullptr))) selectedQuest = i;
    }

    EndScissorMode();

    DrawLineV(Vector2(650, 150), Vector2(650, 950), WHITE);
    if(available < 1){
        DrawTextEx(GuiGetFont(), "[?]", Vector2(1220, 400), 160, 1.5f, Fade(GRAY, 0.5));
        std::string text = WrappedText("YOU CURRENTLY DON'T HAVE ANY ACTIVE QUESTS. EXPLORE MORE TO GET QUESTS!", 45, 900, 1.5);
        DrawTextEx(GuiGetFont(), text.c_str(), Vector2(850, 610), 45, 1.5, Fade(GRAY, 0.5));
    }
    else if(selectedQuest < 0){
        DrawTextEx(GuiGetFont(), "[?]", Vector2(1220, 450), 160, 1.5f, Fade(GRAY, 0.5));
        std::string text = WrappedText("SELECT A QUEST FOR DETAILS", 45, 900, 1.5);
        DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1050, 610), 45, 1.5, Fade(GRAY, 0.5));
    }

    else{
        float Height, _temp;
        std::string txt = WrappedText(gameData.quests[selectedQuest].questTitle, 45, 900, 1.5f, &Height);
        DrawTextEx(GuiGetFont(), txt.c_str(), Vector2(750, 150), 45, 1.5, WHITE);
        _temp = Height;
        txt = WrappedText(gameData.quests[selectedQuest].questDesc, 30, 900, 1.5f, &Height);
        DrawTextEx(GuiGetFont(), txt.c_str(), Vector2(750, 150 + _temp + 15), 30, 1.5, WHITE);
        _temp += Height + 15;
        txt = WrappedText(gameData.quests[selectedQuest].questTask, 30, 900, 1.5f, &Height);
        DrawTextEx(GuiGetFont(), txt.c_str(), Vector2(750, 150 + _temp + 30), 30, 1.5, YELLOW);
        _temp += Height + 30;
        DrawTextEx(GuiGetFont(), "REWARDS:", Vector2(750, 150 + _temp + 30), 35, 1.5, WHITE);
        _temp += 30;
        Rectangle source = {
            .x = 612,
            .y = 0,
            .width = 65.0f,
            .height = 65.0f
        };
        for(int i = 0; i < gameData.quests[selectedQuest].rewardCount; i++){
            Rectangle dest = {
            .x = 750 + (2*TILE_SIZE+35)*i,
            .y = 150 + _temp + 55,
            .width = 2*TILE_SIZE,
            .height = 2*TILE_SIZE
            };
            DrawTexturePro(*panels, source, dest, (Vector2){0.0, 0.0}, 0.0f, WHITE);
            //DrawTexturePro(*(gameData.quests[selectedQuest].rewards[i].icon), source, dest, (Vector2){0.0, 0.0}, 0.0f, WHITE);
            DrawTextureEx(*(gameData.quests[selectedQuest].rewards[i].item->icon), Vector2(dest.x + 10, dest.y + 10), 0.0f, 1.3f, WHITE);
            const char* amt = std::to_string(gameData.quests[selectedQuest].rewards[i].amount).c_str();
            Vector2 text = MeasureTextEx(GuiGetFont(), amt, 16, 1.0f);
            float textX = dest.x + 2*TILE_SIZE - 12 - text.x, textY = dest.y + 2*TILE_SIZE - 5 - text.y;
            DrawTextEx(GuiGetFont(), amt, Vector2(textX, textY), 16.0f, 1.0f, WHITE);
        }
        Rectangle rec = {
            750+900-150, 950-50, 150, 50
        };
        DrawRectangleRec(rec, GRAY);
        if(gameData.quests[selectedQuest].complete){
            DrawTextEx(GuiGetFont(), "CLAIM", Vector2(rec.x+40, rec.y+10), 30, 1.5f, WHITE);
            DrawRectangleLinesEx(rec, 2.0, DARKBLUE);
            if(static_cast<bool>(GuiLabelButton(rec, nullptr))){
                collectClicked = true;
            }
        }
        else if(gameData.trackingQuest != nullptr && gameData.quests[selectedQuest].id == gameData.trackingQuest->id){
            GuiDrawIcon(ICON_CROSS, rec.x+5, rec.y+9, 2, WHITE);
            DrawTextEx(GuiGetFont(), "UNTRACK", Vector2(rec.x+40, rec.y+10), 30, 1.5f, WHITE);
            DrawRectangleLinesEx(rec, 2.0, DARKBLUE);
            if(static_cast<bool>(GuiLabelButton(rec, nullptr))){
                trackClicked = true;
            }
        }
        else{
            GuiDrawIcon(ICON_STAR, rec.x+20, rec.y+7, 2, WHITE);
            DrawTextEx(GuiGetFont(), "TRACK", Vector2(rec.x+55, rec.y+12), 30, 1.5f, WHITE);
            DrawRectangleLinesEx(rec, 2.0, DARKBLUE);
            if(static_cast<bool>(GuiLabelButton(rec, nullptr))){
                trackClicked = true;
            }
        }
    }
}

DailyTasksUI::DailyTasksUI(GameData &data) : GUI(true, data), crossClicked(false){
    panels = data.res.GetTexture("inventory_panel_icon");
    ShowCursor();
}

GUI* DailyTasksUI::Update(float){
    if(IsKeyPressed(KEY_X) || crossClicked){
        PlayButtonSfx(gameData);
        return new OverlayUI(gameData);
    }
    return this;
}

void DailyTasksUI::Draw(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    ClearBackground(Fade(BLACK, 0.1f));
    DrawTextEx(GuiGetFont(), "DAILY TASKS", Vector2(100.0f, 40.0f), 45, 2.0f, WHITE);
    crossClicked = static_cast<bool>(GuiLabelButton(Rectangle(sW-150, 40, 1.5*TILE_SIZE, 1.5*TILE_SIZE), nullptr));
    GuiDrawIcon(ICON_CROSS, sW-150, 40, 3, WHITE);
    GuiLine((Rectangle){0, 115, sW, 2.0f}, nullptr);

    Rectangle bounds = {100, 150, 1300, 900};
    Rectangle content = {
        .x = 0,
        .y = 0,
        .width = bounds.width,
        .height = gameData.dailyTasks.size() * 150
    };
    Vector2 scroll = {0, 0};
    Rectangle view;
    GuiScrollPanel(bounds, "", content, &scroll, &view);
    DrawRectangleRec(bounds, BLACK);

    BeginScissorMode(view.x, view.y, view.width, view.height);

    for (int i = 0; i < static_cast<int>(gameData.dailyTasks.size()); i++){
        
        float x = bounds.x, y = bounds.y + scroll.y + i * 150 + 20, w = bounds.width, h = 160.0f;
        GuiLine((Rectangle){x, y + h, w, 2}, nullptr);
        DrawTextEx(GuiGetFont(), WrappedText(gameData.dailyTasks[i].questTitle, 35, w, 1.5f).c_str(), Vector2(x + 10, y + 30), 35, 1.5f, WHITE);
        DrawTextEx(GuiGetFont(), "Progress: 0 / 100", Vector2(x + 10, y + 75), 30, 1.5f, GREEN);
        GuiProgressBar((Rectangle){x + 10, y + 115, 400, 30}, nullptr, nullptr, nullptr, 0, 100);
        DrawLineV(Vector2(1000, y), Vector2(1000, y + 150), WHITE);
        DrawTextEx(GuiGetFont(), "Rewards", Vector2(1000 + 15, y + 30), 35, 1.5f, WHITE);

        Rectangle source = {
            .x = 612,
            .y = 0,
            .width = 65.0f,
            .height = 65.0f
        };

        for(int j = 0; j < gameData.dailyTasks[i].rewardCount; j++){
            Rectangle dest = {
            .x = 1000 + 15 + (2*TILE_SIZE+15)*j,
            .y = y + 75,
            .width = 2*TILE_SIZE,
            .height = 2*TILE_SIZE
            };
            if(panels) DrawTexturePro(*panels, source, dest, (Vector2){0.0, 0.0}, 0.0f, WHITE);
        }

    }

    EndScissorMode();

}

CharacterUI::CharacterUI(GameData &data) : GUI(true, data), selectedCharacter(0), crossClicked(false), selectedSkill(-1){
    atk = data.res.GetTexture("atk_icon");
    def = data.res.GetTexture("def_icon");
    hp = data.res.GetTexture("hp_icon");
    crit_rate = data.res.GetTexture("crit_rate_icon");
    crit_dmg = data.res.GetTexture("crit_dmg_icon");
    ShowCursor();
}

GUI* CharacterUI::Update(float){
    const int characterCount = gameData.playableCharacterCount;
    if(characterCount == 0) return this;

    if(IsKeyPressed(KEY_X) || crossClicked){
        PlayButtonSfx(gameData);
        return new OverlayUI(gameData);
    }

    if(IsKeyPressed(KEY_W)){
        selectedCharacter--;
        if(selectedCharacter < 0) selectedCharacter = characterCount - 1;
        selectedSkill = -1;
        PlayButtonSfx(gameData);
    }
    else if(IsKeyPressed(KEY_S)){
        selectedCharacter = (selectedCharacter + 1) % characterCount;
        selectedSkill = -1;
        PlayButtonSfx(gameData);
    }
    return this;
}

void CharacterUI::Draw(){
    int sW = GetScreenWidth(), sH = GetScreenHeight();
    if(gameData.playableCharacterCount == 0) return;
    if(selectedCharacter >= gameData.playableCharacterCount) selectedCharacter = 0;

    ClearBackground(Fade(BLACK, 0.1f));
    DrawTextEx(GuiGetFont(), "CHARACTERS", Vector2(100.0f, 40.0f), 45, 2.0f, WHITE);
    crossClicked = static_cast<bool>(GuiLabelButton(Rectangle(sW-150, 40, 1.5*TILE_SIZE, 1.5*TILE_SIZE), nullptr));
    GuiDrawIcon(ICON_CROSS, sW-150, 40, 3, WHITE);
    GuiLine((Rectangle){0, 115, sW, 2.0f}, nullptr);

    DrawLineV(Vector2(70, 135), Vector2(70, sH - 150), WHITE);

    DrawText("[W]", 100, 165, 25, YELLOW);
    GuiDrawIcon(ICON_ARROW_UP, 100, 135, 2, YELLOW);
    DrawText("[S]", 100, sH - 200, 25, YELLOW);
    GuiDrawIcon(ICON_ARROW_DOWN, 100, sH - 170, 2, YELLOW);

    DrawLineV(Vector2(160, 135), Vector2(160, sH - 150), WHITE);

    Rectangle src = {
        0,
        0,
        16,
        16
    };

    int k = 0;
    for(int i = 0; i < gameData.characters.size(); i++){
        if(gameData.characters[i].isEnemy){
            continue;
        }
        Rectangle dest = {
            100,
            165 + 100*(k + 1),
            28,
            28
        };
        Texture2D *portrait = gameData.characters[i].portrait;
        if(k == selectedCharacter){
            DrawCircle(100 + 14, 165 + 100*(k + 1) + 14, 28, LIGHTGRAY);
            if(portrait) DrawTexturePro(*portrait, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
        }
        else {
            if(portrait) DrawTexturePro(*portrait, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
        }
        DrawCircleLines(100 + 14, 165 + 100*(k + 1) + 14, 28, WHITE);

        if(static_cast<bool>(GuiLabelButton(dest, nullptr))) selectedCharacter = k;
        k++;
    }

    const Character *currentPtr = nullptr;
    int playableIdx = 0;
    for(const auto &c : gameData.characters) {
        if(!c.isEnemy) {
            if(playableIdx == selectedCharacter) {
                currentPtr = &c;
                break;
            }
            playableIdx++;
        }
    }
    if(!currentPtr) return;
    const Character &current = *currentPtr;

    DrawTextEx(GuiGetFont(), current.name.c_str(), Vector2(200, 140), 50, 2.0f, WHITE);
    Rectangle dst = {
        200,
        240,
        128,
        128
    };
    if(current.portrait) DrawTexturePro(*current.portrait, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    DrawTextEx(GuiGetFont(), "ATTRIBUTES", Vector2(220, 400), 35, 2.0f, WHITE);
    GuiLine((Rectangle){220, 440, 600, 2}, nullptr);
    DrawTextEx(GuiGetFont(), ("ATK\t" + std::to_string(current.stats.ATK)).c_str(), Vector2(255, 455 + 20), 35, 2.0f, WHITE);
    DrawTextEx(GuiGetFont(), ("HP\t" + std::to_string(current.stats.HP)).c_str(), Vector2(255, 455 + 60), 35, 2.0f, WHITE);
    DrawTextEx(GuiGetFont(), ("DEF\t" + std::to_string(static_cast<int>(current.stats.DEF*100)) + "%").c_str(), Vector2(255, 455 + 100), 35, 2.0f, WHITE);
    DrawTextEx(GuiGetFont(), ("CRIT RATE\t" + std::to_string(static_cast<int>(current.stats.CRIT_RATE*100)) + "%").c_str(), Vector2(255, 455 + 140), 35, 2.0f, WHITE);
    DrawTextEx(GuiGetFont(), ("CRIT DAMAGE\t" + std::to_string(static_cast<int>(current.stats.CRIT_DMG*100)) + "%").c_str(), Vector2(255, 455 + 180), 35, 2.0f, WHITE);

    if(atk) DrawTextureEx(*atk, Vector2(220, 455 + 20 + 3), 0.0, 1.5, WHITE);
    if(hp) DrawTextureEx(*hp, Vector2(220, 455 + 60 + 3), 0.0, 1.5, WHITE);
    if(def) DrawTextureEx(*def, Vector2(220, 455 + 100 + 3), 0.0, 1.5, WHITE);
    if(crit_rate) DrawTextureEx(*crit_rate, Vector2(220, 455 + 140 + 3), 0.0, 1.5, WHITE);
    if(crit_dmg) DrawTextureEx(*crit_dmg, Vector2(220, 455 + 180 + 3), 0.0, 1.5, WHITE);

    DrawTextEx(GuiGetFont(), "SKILLS", Vector2(1000, 150), 35, 2.0f, WHITE);
    const int skillCount = static_cast<int>(current.skills.size());
    for(int i = 0; i < skillCount && i < Character::SKILLS; i++){
        Rectangle skillDst = {
            1000 + (TILE_SIZE + 16 + 30)*i,
            220,
            TILE_SIZE + 16,
            TILE_SIZE + 16
        };
        if(current.skills[i].icon){
            DrawTexturePro(*current.skills[i].icon, src, skillDst, Vector2(0, 0), 0.0f, WHITE);
        }
        DrawRectangleLinesEx(skillDst, 3.0f, GRAY);
        if(static_cast<bool>(GuiLabelButton(skillDst, nullptr))) selectedSkill = i;
    }

    DrawRectangleLinesEx((Rectangle){1000, 220 + 48 + 48, 750, 700}, 1.5f, WHITE);

    if(selectedSkill < 0 || selectedSkill >= skillCount){
        DrawTextEx(GuiGetFont(), "[?]", Vector2(1320, 550), 160, 1.5f, Fade(GRAY, 0.5));
        std::string text = WrappedText("SELECT A SKILL FOR ADDITIONAL DETAILS", 45, 750 - 50, 1.5);
        DrawTextEx(GuiGetFont(), text.c_str(), Vector2(1050, 720), 45, 1.5, Fade(GRAY, 0.5));
    }
    else{
        const Skill &s = current.skills[selectedSkill];
        if(s.icon){
            DrawTextureEx(*s.icon, Vector2(1000 + 50, 316 + 50), 0.0f, 3.0f, WHITE);
            DrawRectangleLinesEx((Rectangle){1000 + 50, 316 + 50, 96, 96}, 1.5f, GRAY);
        }
        DrawTextEx(GuiGetFont(), s.title.c_str(), Vector2(1000 + 50 + 96 + 30, 316 + 50), 25, 1.5f, YELLOW);
        DrawTextEx(GuiGetFont(), s.type.c_str(), Vector2(1000 + 50 + 96 + 30, 316 + 50 + 30), 25, 1.5f, WHITE);
        std::string d = WrappedText(s.desc, 25, 750 - 100, 1.5f);
        DrawTextEx(GuiGetFont(), d.c_str(), Vector2(1000 + 50, 316 + 160), 25, 1.5f, WHITE);
    }
}

PauseUI::PauseUI(GameData &data) : GUI(true, data, true), resumeClicked(false), exitClicked(false) {
    ShowCursor();
}

GUI* PauseUI::Update(float) {
    if (IsKeyPressed(KEY_P) || resumeClicked) {
        return new OverlayUI(gameData);
    }
    if (exitClicked) {
        return nullptr;
    }
    return this;
}

void PauseUI::Draw() {
    int sW = GetScreenWidth();
    int sH = GetScreenHeight();
    
    DrawRectangle(0, 0, sW, sH, Fade(BLACK, 0.4f));
    
    float boxWidth = 400.0f;
    float boxHeight = 250.0f;
    float boxX = (sW - boxWidth) / 2.0f;
    float boxY = (sH - boxHeight) / 2.0f;
    
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, (Color){ 30, 30, 45, 240 });
    DrawRectangleLinesEx((Rectangle){ boxX, boxY, boxWidth, boxHeight }, 3.0f, (Color){ 171, 155, 211, 255 });
    
    const char* titleText = "Paused";
    Vector2 titleSize = MeasureTextEx(GuiGetFont(), titleText, 32, 2.0f);
    float titleX = boxX + (boxWidth - titleSize.x) / 2.0f;
    float titleY = boxY + 30.0f;
    DrawTextEx(GuiGetFont(), titleText, (Vector2){ titleX, titleY }, 32.0f, 2.0f, WHITE);
    
    float buttonWidth = 140.0f;
    float buttonHeight = 50.0f;
    float spacing = 30.0f;
    float totalButtonsWidth = (buttonWidth * 2.0f) + spacing;
    float buttonsStartX = boxX + (boxWidth - totalButtonsWidth) / 2.0f;
    float buttonsY = boxY + 130.0f;
    
    Rectangle resumeBounds = { buttonsStartX, buttonsY, buttonWidth, buttonHeight };
    Rectangle exitBounds = { buttonsStartX + buttonWidth + spacing, buttonsY, buttonWidth, buttonHeight };
    
    int oldTextSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    
    if (GuiButton(resumeBounds, "Resume")) {
        PlayButtonSfx(gameData);
        resumeClicked = true;
    }
    if (GuiButton(exitBounds, "Exit")) {
        PlayButtonSfx(gameData);
        exitClicked = true;
    }
    
    GuiSetStyle(DEFAULT, TEXT_SIZE, oldTextSize);
}

Player::Player(Resources &res) : speed(128.0f) , curAnimation(-1), spriteFrame(1){
    position = { 500.0f, 500.0f };
    animations.resize(ANIMS);
    sprites.resize(SPRITES);
    sprites[AnimList::RUN_LEFT] = res.GetTexture("shadow_run_left");
    sprites[AnimList::RUN_RIGHT] = res.GetTexture("shadow_run_right");
    sprites[AnimList::RUN_UP] = res.GetTexture("shadow_run_up");
    sprites[AnimList::RUN_DOWN] = res.GetTexture("shadow_run_down");
    sprites[AnimList::IDLE_DOWN] = res.GetTexture("shadow_idle_down");
    sprites[AnimList::IDLE_UP] = res.GetTexture("shadow_idle_up");
    sprites[AnimList::IDLE_RIGHT] = res.GetTexture("shadow_idle_right");
    sprites[AnimList::IDLE_LEFT] = res.GetTexture("shadow_idle_left");
    for(int i = 0; i < ANIMS; i++){
        animations[i] = Animation(6, 0.8f);
    }
}

void Player::Update(float dt, World& world){
    static Vector2 last_dir = {1, 0};
    if(curAnimation != -1) animations[curAnimation].step(dt);
    Vector2 movement = {0.0f, 0.0f};
    if(IsKeyDown(KEY_W)) movement.y -= 1.0f;
    if(IsKeyDown(KEY_S)) movement.y += 1.0f;
    if(IsKeyDown(KEY_A)) movement.x -= 1.0f;
    if(IsKeyDown(KEY_D)) movement.x += 1.0f;

    if(movement.y > 0.0f) curAnimation = AnimList::RUN_DOWN;
    else if(movement.y < 0.0f) curAnimation = AnimList::RUN_UP;
    else if(movement.x > 0.0f) curAnimation = AnimList::RUN_RIGHT;
    else if(movement.x < 0.0f) curAnimation = AnimList::RUN_LEFT;
    else{
        if(last_dir.y > 0.0f) curAnimation = AnimList::IDLE_DOWN;
        else if(last_dir.y < 0.0f) curAnimation = AnimList::IDLE_UP;
        else if(last_dir.x > 0.0f) curAnimation = AnimList::IDLE_RIGHT;
        else if(last_dir.x < 0.0f) curAnimation = AnimList::IDLE_LEFT;
        else curAnimation = -1;
    }

    if(Vector2Length(movement) > 0.0f) movement = Vector2Normalize(movement);
    if(movement != (Vector2){0, 0}) last_dir = movement;

    const Vector2 delta = { movement.x * speed * dt, movement.y * speed * dt };
    int nextElevation = elevation;

    if (delta.x != 0.0f) {
        const Vector2 tryX = { position.x + delta.x, position.y };
        if (world.TryMove(position, tryX, nextElevation)) elevation = nextElevation;
    }

    nextElevation = elevation;
    if (delta.y != 0.0f) {
        const Vector2 tryY = { position.x, position.y + delta.y };
        if (world.TryMove(position, tryY, nextElevation)) elevation = nextElevation;
    }

    position.x = Clamp(position.x, 0.0f, world.GetWidth() * TILE_SIZE - TILE_SIZE);
    position.y = Clamp(position.y, 0.0f, world.GetHeight() * TILE_SIZE - TILE_SIZE);
}

void Player::Draw() const{
    int animIndex = (curAnimation != -1) ? curAnimation : IDLE_DOWN;
    int frame = (curAnimation != -1) ? animations[curAnimation].curFrame() : (spriteFrame - 1);

    Rectangle source = {
        .x = (float)(frame * TILESET_TILE_SIZE),
        .y = 0,
        .width = TILESET_TILE_SIZE,
        .height = TILESET_TILE_SIZE
    };

    Rectangle dest = {
        .x = position.x,
        .y = position.y,
        .width = TILE_SIZE,
        .height = TILE_SIZE
    };

    if(sprites[animIndex]) DrawTexturePro(*sprites[animIndex], source, dest, (Vector2){0, 0}, 0.0f, WHITE);
}

NPC::NPC(Resources &res) : curAnimation(0), spriteFrame(1){
    position = { 0.0f, 0.0f };
    dialogueActive = false;
    canInteract = false;
    dialogueIndex = 0;
    animations.resize(ANIMS);
    sprites.resize(SPRITES);
    animations[AnimList::IDLE] = Animation(6, 1.0f);
    sprites[AnimList::IDLE] = res.GetTexture("npc_idle");
}

NPC::NPC(){
    position = { 0.0f, 0.0f };
    dialogueActive = false;
    canInteract = false;
    dialogueIndex = 0;
}

NPC NPC::MakeNPC(Resources &res, std::string& name, Vector2 position, std::vector<std::string>& DialogueLines) {
    NPC npc = NPC(res);
    npc.name = npc.id = name;
    npc.position = position;
    npc.dialogueLines = DialogueLines;
    return npc;
}

int InteractiveEntity::HandleInteraction(Vector2 playerPos, float dt){
    playerRelPosition = playerPos.y > position.y ? -1 : 1; 
    if(IsInInteractionRange(playerPos, position)) canInteract = true;
    else canInteract = false;
    if(canInteract && IsKeyPressed(KEY_F) && !dialogueActive){
        dialogueActive = true;
        typeWriter.isActive = true;
        typeWriter.ResetTypewriter();
        typeWriter.TypeWriterUpdate(name + ": " + dialogueLines[dialogueIndex], dt); 
    }
    if(dialogueActive) typeWriter.TypeWriterUpdate(name + ": " + dialogueLines[dialogueIndex], dt);
    if(dialogueActive && IsKeyPressed(KEY_ENTER)){
        std::string currentText = name + ": " + dialogueLines[dialogueIndex];
        if(!typeWriter.IsTypewriterComplete(currentText)){
            typeWriter.CompleteTypewriter(currentText);
        }
        else if(dialogueIndex < dialogueLines.size() - 1){
            dialogueIndex++;
            typeWriter.ResetTypewriter();
        }
        else{
            dialogueActive = false;
            typeWriter.isActive = false;
            dialogueIndex = 0;
            typeWriter.ResetTypewriter();
            return 1;
        }
    }
    if(dialogueActive && !IsInInteractionRange(playerPos, position)){
        dialogueActive = false;
        typeWriter.isActive = false;
        dialogueIndex = 0;
        typeWriter.ResetTypewriter();
    }

    return 0;
}

void NPC::Update(float dt, Player& player){
    if(curAnimation != -1) animations[curAnimation].step(dt);
    if(HandleInteraction(player.GetPosition(), dt)){
        gameData.QuestManager(EventType::TALK_TO_NPC, id);
    }
}

void NPC::Draw() const{
    int animIndex = (curAnimation != -1) ? curAnimation : IDLE;
    int frame = (curAnimation != -1) ? animations[curAnimation].curFrame() : (spriteFrame - 1);

    const int frameWidth = 96;
    const int frameHeight = 96;

    Rectangle source = {
        .x = (float)(frame * frameWidth),
        .y = 0,
        .width = (float)frameWidth,
        .height = (float)frameHeight
    };

    Rectangle dest = {
        .x = position.x,
        .y = position.y,
        .width = TILE_SIZE * 5,
        .height = TILE_SIZE * 5
    };

    if(sprites[animIndex]) DrawTexturePro(*sprites[animIndex], source, dest, (Vector2){0, 0}, 0.0f, WHITE);
    //if(dialogueActive) typeWriter.Draw();
    if(canInteract && !dialogueActive){
        if(playerRelPosition == -1){
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 40, 40, Fade(WHITE, 0.6f));
            DrawTextEx(GuiGetFont(), "[F]", Vector2(position.x + 10, position.y + 10 - TILE_SIZE - 40), 20, 1.5, WHITE);
            DrawTextEx(GuiGetFont(), "INTERACT", Vector2(position.x + 50, position.y + 10 - TILE_SIZE - 40), 20, 1.5f, WHITE);
        }
        else{
            DrawRectangle(position.x, position.y + TILE_SIZE, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y + TILE_SIZE, 40, 40, Fade(WHITE, 0.6f));
            DrawTextEx(GuiGetFont(), "[F]", Vector2(position.x + 10, position.y + 10 + TILE_SIZE), 20, 1.5f, WHITE);
            DrawTextEx(GuiGetFont(), "INTERACT", Vector2(position.x + 50, position.y + 10 + TILE_SIZE), 20, 1.5f, WHITE);
        }
    }
}

Enemy::Enemy(Resources &res): curAnimation(0), spriteFrame(1){
    position = { 0.0f, 0.0f };
    dialogueActive = false;
    canInteract = false;
    dialogueIndex = 0;
    animations.resize(ANIMS);
    sprites.resize(SPRITES);
    animations[AnimList::IDLE] = Animation(6, 1.0f);

}

Enemy::Enemy(Resources &res, const Vector2 position, const std::string &name,
             const std::vector<std::string>& dialogueLines, std::array<Character*, 3> inBattleCharacters, const std::string textureId) : Enemy(res) {
    this->position = position;
    this->name = name;
    this->id = name;
    this->inBattleCharacters = inBattleCharacters;
    this->sprites[AnimList::IDLE] = res.GetTexture(textureId);
    if (!dialogueLines.empty()) this->dialogueLines = dialogueLines;
}

void Enemy::Update(float dt, Player& player){
    if(curAnimation != -1) animations[curAnimation].step(dt);
    if(HandleInteraction(player.GetPosition(), dt)){
        //gameData.fighting = true;
        //gameData.QuestManager(EventType::KILL, id, 3);
        gameData.inBattleCharacters = inBattleCharacters;
        gameData.fighting = true;
    }
}

void Enemy::Draw() const{
    //if(dialogueActive) typeWriter.Draw();
    int animIndex = (curAnimation != -1) ? curAnimation : IDLE;
    int frame = (curAnimation != -1) ? animations[curAnimation].curFrame() : (spriteFrame - 1);

    Rectangle source = {
        .x = (float)(frame * TILESET_TILE_SIZE),
        .y = 0,
        .width = TILESET_TILE_SIZE,
        .height = TILESET_TILE_SIZE
    };

    Rectangle dest = {
        .x = position.x,
        .y = position.y,
        .width = TILE_SIZE,
        .height = TILE_SIZE
    };

    if(sprites[animIndex]) DrawTexturePro(*sprites[animIndex], source, dest, (Vector2){0, 0}, 0.0f, WHITE);

    if(canInteract && !dialogueActive){
        if(playerRelPosition == -1){
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y - TILE_SIZE - 40, 40, 40, Fade(WHITE, 0.6f));
            DrawText("[F]", position.x + 10, position.y + 10 - TILE_SIZE - 40, 20, WHITE);
            DrawText("Fight", position.x + 50, position.y + 10 - TILE_SIZE - 40, 20, WHITE);
        }
        else{
            DrawRectangle(position.x, position.y + TILE_SIZE, 160, 40, Fade(BLACK, 0.7f));
            DrawRectangle(position.x, position.y + TILE_SIZE, 40, 40, Fade(WHITE, 0.6f));
            DrawText("[F]", position.x + 10, position.y + 10 + TILE_SIZE, 20, WHITE);
            DrawText("Fight", position.x + 50, position.y + 10 + TILE_SIZE, 20, WHITE);
        }
    }
}

World::World(Resources &res) : res(res) {}

World::~World() = default;

void World::LoadFromTilemap(const std::string& filepath, World& world) {
    world.entities.clear();
    try {
        tilemapData = LoadFromFile(filepath, world, res);
        width = tilemapData.width;
        height = tilemapData.height;
    } catch (const std::exception& e) {
        tilemapData = {};
        width = 0;
        height = 0;
        WriteErrorLog(std::string("ERROR: Failed to load tilemap: ") + e.what());
    }
}

bool World::TileBlocksPath(int gid) const {
    if (gid <= 0) return false;
    const TileProperties& props = tilemapData.GetTileProps(gid);
    return props.blocksPath;
}

bool World::IsGroundPathTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    const int gid = tilemapData.GetGid("Path", x, y);
    if (gid <= 0) return false;
    const TileProperties& props = tilemapData.GetTileProps(gid);
    return !props.blocksPath;
}

bool World::IsStairTile(int x, int y) const {
    if (tilemapData.GetGid("Platform", x, y) <= 0) return false;
    if (IsGroundPathTile(x, y)) return true;

    const int dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    for (const auto& dir : dirs) {
        const int nx = x + dir[0];
        const int ny = y + dir[1];
        if (IsGroundPathTile(nx, ny)) return true;
    }
    return false;
}

static Rectangle GetPlayerBounds(Vector2 pos) {
    return { pos.x, pos.y, (float)TILE_SIZE, (float)TILE_SIZE };
}

bool World::IsBlockedByCollision(Rectangle playerBounds, int elevation) const {
    (void)elevation;
    for (const auto& rect : tilemapData.collisionRects) {
        if (CheckCollisionRecs(playerBounds, rect.bounds)) return true;
    }
    return false;
}

bool World::IsWalkableAtElevation(int x, int y, int elevation) const {
    if(x < 0 || x >= width || y < 0 || y >= height) return false;

    if (elevation == 1) {
        const int pathGid = tilemapData.GetGid("Path", x, y);
        if(pathGid <= 0) return false;
        if(TileBlocksPath(pathGid)) return false;

        return true;
    }

    const int platformGid = tilemapData.GetGid("Platform", x, y);
    if(platformGid <= 0) return false;
    if(TileBlocksPath(platformGid)) return false;

    return true;
}

bool World::TryMove(Vector2& position, Vector2 nextPosition, int& elevation) const {
    const Rectangle nextBounds = GetPlayerBounds(nextPosition);
    if (IsBlockedByCollision(nextBounds, elevation)) return false;

    const int nextTileX = static_cast<int>(floorf((nextPosition.x + TILE_SIZE * 0.5f) / TILE_SIZE));
    const int nextTileY = static_cast<int>(floorf((nextPosition.y + TILE_SIZE * 0.5f) / TILE_SIZE));
    const int currentTileX = static_cast<int>(floorf((position.x + TILE_SIZE * 0.5f) / TILE_SIZE));
    const int currentTileY = static_cast<int>(floorf((position.y + TILE_SIZE * 0.5f) / TILE_SIZE));

    if (elevation == 1) {
        if (IsStairTile(nextTileX, nextTileY) && IsWalkableAtElevation(nextTileX, nextTileY, 2)) {
            elevation = 2;
            position = nextPosition;
            return true;
        }
        if (IsWalkableAtElevation(nextTileX, nextTileY, 1)) {
            position = nextPosition;
            return true;
        }
        return false;
    }

    if (IsWalkableAtElevation(nextTileX, nextTileY, 2)) {
        position = nextPosition;
        return true;
    }

    if (IsStairTile(currentTileX, currentTileY) && IsWalkableAtElevation(nextTileX, nextTileY, 1)) {
        elevation = 1;
        position = nextPosition;
        return true;
    }

    return false;
}

Vector2 World::FindSpawnPosition() const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (IsWalkableAtElevation(x, y, 1)) {
                return {(float)(x * TILE_SIZE), (float)(y * TILE_SIZE)};
            }
        }
    }
    return {(float)(width * TILE_SIZE / 2), (float)(height * TILE_SIZE / 2)};
}

void World::Update(float dt, Player &player){
    for(auto &e : entities){
        e->Update(dt, player);
    }
}

void World::Draw(const Player &player){
    bool entitiesDrawn = false;

    for (const auto& layerName : tilemapData.renderLayerNames) {
        if (!entitiesDrawn && (layerName == "Treetops" || layerName == "Obstacles")) {
            player.Draw();
            for (auto &e : entities) {
                e->Draw();
            }
            entitiesDrawn = true;
        }

        const TileLayer* layer = tilemapData.GetLayer(layerName);
        if (!layer) continue;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int gid = layer->GetTile(x, y, width);
                if (gid <= 0) continue;

                const TileProperties& props = tilemapData.GetTileProps(gid);
                if (props.textureId.empty()) continue;

                Texture2D* texture = res.GetTexture(props.textureId);
                if (!texture) continue;

                const Rectangle destRec = {
                    (float)(x * TILE_SIZE),
                    (float)(y * TILE_SIZE),
                    (float)TILE_SIZE,
                    (float)TILE_SIZE
                };

                DrawTexturePro(*texture, props.source, destRec, {0, 0}, 0.0f, WHITE);
            }
        }
    }
}

void World::DrawCollisionDebug(Vector2 playerPos) const {
    const Rectangle playerBounds = GetPlayerBounds(playerPos);
    DrawRectangleLinesEx(playerBounds, 2.0f, GREEN);

    for (const auto& rect : tilemapData.collisionRects) {
        Color color = RED;
        if (rect.name == "Stairs_side") color = ORANGE;
        else if (rect.name == "Border") color = RED;

        DrawRectangleLinesEx(rect.bounds, 2.0f, color);
    }
}

void Cam::SetMapSize(int width, int height) {
    mapWidthPixels = width * TILE_SIZE;
    mapHeightPixels = height * TILE_SIZE;

    cam.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    cam.zoom = 4.0f;
}

void Cam::Update(float dt, const Vector2& playerPos){
    const float halfViewW = (GetScreenWidth() / cam.zoom) * 0.5f;
    const float halfViewH = (GetScreenHeight() / cam.zoom) * 0.5f;

    float t = 1.0f - expf(-smoothSpeed * dt);
    cam.target.x += (playerPos.x - cam.target.x) * t;
    cam.target.y += (playerPos.y - cam.target.y) * t;

    if (mapWidthPixels <= static_cast<int>(halfViewW * 2.0f)) {
        cam.target.x = mapWidthPixels / 2.0f;
    } else {
        cam.target.x = Clamp(cam.target.x, halfViewW, mapWidthPixels - halfViewW);
    }

    if (mapHeightPixels <= static_cast<int>(halfViewH * 2.0f)) {
        cam.target.y = mapHeightPixels / 2.0f;
    } else {
        cam.target.y = Clamp(cam.target.y, halfViewH, mapHeightPixels - halfViewH);
    }
}

GameScene::GameScene(Vector2 spawnPos, const std::string& mapPath) : world(gameData.res), player(gameData.res){
    LoadMap(mapPath, spawnPos);
    background = gameData.res.GetMusic("game_background_music");
    if(background) PlayMusicStream(*background);
    background->looping = true;
    gui = new OverlayUI(gameData);
    HideCursor();
}

GameScene::~GameScene(){
    gameData.SaveGame();
    delete gui;
}

void GameScene::LoadMap(const std::string& mapPath, Vector2 spawnPos){
    world.LoadFromTilemap(mapPath, world);
    currentMap = mapPath;
    gameData.currentMap = mapPath;
    Vector2 spawn = (spawnPos.x >= 0 && spawnPos.y >= 0) ? spawnPos : world.FindSpawnPosition();
    player.SetPosition(spawn);
    camera.SetMapSize(world.GetWidth(), world.GetHeight());
    camera.SetTarget(spawn);
    transitionProtection = 0.5f;
}

Scene* GameScene::Update(float dt){
    if(background) UpdateMusicStream(*background);
    if(gameData.fighting) {
        gameData.preBattleSpawnPosition = player.GetPosition();
        return new BattleScene();
    }

    if(transitionProtection > 0.0f) {
        transitionProtection -= dt;
    }

    if(transitioning) {
        if(fadeOut) {
            fadeAlpha += dt * 3.0f;
            if(fadeAlpha >= 1.0f) {
                fadeAlpha = 1.0f;
                world.LoadFromTilemap(pendingMap, world);
                currentMap = pendingMap;
                gameData.currentMap = pendingMap;
                if(pendingSpawn.x >= 0 && pendingSpawn.y >= 0) {
                    player.SetPosition(pendingSpawn);
                } else {
                    player.SetPosition(world.FindSpawnPosition());
                }
                camera.SetMapSize(world.GetWidth(), world.GetHeight());
                camera.SetTarget(player.GetPosition());
                transitionProtection = 0.5f;
                fadeOut = false;
            }
        } else {
            fadeAlpha -= dt * 3.0f;
            if(fadeAlpha <= 0.0f) {
                fadeAlpha = 0.0f;
                transitioning = false;
            }
        }
    } else {
        if(!gui->blockProgress){
            world.Update(dt, player);
            player.Update(dt, world);
            gameData.playerPosition = player.GetPosition();
            camera.Update(dt, player.GetPosition());
            if(transitionProtection <= 0.0f) {
                CheckTriggers();
            }
        }
    }

    GUI* next;
    next = gui->Update(dt);
    if(next != gui){
        PlayButtonSfx(gameData);
        if(next == nullptr) {
            if(background) StopMusicStream(*background);
            return new MenuScene();
        }
        delete gui;
        gui = next;
    }
    return this;
}

void GameScene::Draw(){
    ClearBackground((Color){ 20, 20, 30, 255 });
    BeginMode2D(camera.cam);
    if(gui->drawWorldUnderneath){
        world.Draw(player);
        // if(showCollisionDebug) world.DrawCollisionDebug(player.GetPosition());
    }
    EndMode2D();
    gui->Draw();
    typeWriter.Draw();
    // if(showCollisionDebug){
    //     DrawText("Collision Debug [F12]", 10, 10, 20, LIME);
    // }

    if(transitioning && fadeAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, fadeAlpha));
    }
}

void GameScene::CheckTriggers(){
    Vector2 playerPos = player.GetPosition();
    for (const auto& trigger : world.GetTriggerRects()) {
        Vector2 triggerPos = {trigger.bounds.x, trigger.bounds.y};
        if (IsInInteractionRange(playerPos, triggerPos)) {
            if (trigger.name == "Next_map" && IsKeyPressed(KEY_F)) {
                StartMapTransition();
                break;
            }
            if (trigger.name == "Spawn" && IsKeyPressed(KEY_F)) {
                StartMapTransition("data/map/tilemap/tilemap.tmj", {-1, -1});
                break;
            }
        }
    }
}

void GameScene::StartMapTransition(){
    if(currentMap == "data/map/tilemap/tilemap.tmj") {
        StartMapTransition("data/map/tilemap/untitled.tmj", {86.0f, 363.0f});
    } else {
        StartMapTransition("data/map/tilemap/tilemap.tmj", {-1, -1});
    }
}

void GameScene::StartMapTransition(const std::string& targetMap, Vector2 spawnPos){
    transitioning = true;
    fadeOut = true;
    fadeAlpha = 0.0f;
    pendingMap = targetMap;
    pendingSpawn = spawnPos;
}

