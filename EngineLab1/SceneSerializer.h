#pragma once

#include <string>

#include "World.h"
#include "ResourceManager.h"
#include "RenderAdapter.h"

class SceneSerializer {
public:
    static bool SaveScene(
        const std::string& path,
        World& world
    );

    static bool LoadScene(
        const std::string& path,
        World& world,
        ResourceManager& resourceManager,
        RenderAdapter& renderer
    );
};