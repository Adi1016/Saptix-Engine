#pragma once

struct GameObject;

class Component
{
public:
    GameObject* owner = nullptr;

    virtual void Update(float deltaTime) {}
    virtual ~Component() {}
};
