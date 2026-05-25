#pragma once
#include "scene.h"

class StartupScene : public Scene{
    public:
        StartupScene();
        Scene* Update(float dt) override;
        void Draw() override;

    private:
        float timer;
        enum StartupState{ IN, HOLD, OUT, EXIT } state;
};