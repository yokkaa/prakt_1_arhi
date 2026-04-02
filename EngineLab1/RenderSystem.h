#pragma once
#include "World.h"

class RenderAdapter;

class RenderSystem {
public:
    void Update(World& world, RenderAdapter& renderer, float deltaTime);
};
