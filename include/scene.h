#pragma once

class Scene{
    public:
        virtual ~Scene() = default;
        virtual Scene* Update(float dt) = 0;
        virtual void Draw() = 0;
};