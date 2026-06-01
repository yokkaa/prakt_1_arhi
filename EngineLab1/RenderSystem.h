#pragma once

#include "World.h"

class RenderAdapter;
class ResourceManager;

class RenderSystem {
public:
    void Update(
        World& world,
        RenderAdapter& renderer,
        ResourceManager& resourceManager,
        float deltaTime
    );
};
