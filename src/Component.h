#pragma once
#include <string>

struct GameObject;

class Component
{
public:
    GameObject* owner = nullptr;

    virtual void Update(float deltaTime) {}
    virtual std::string GetName() const = 0;
    virtual ~Component() {}
};
