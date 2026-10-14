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

    // Только чтение файла и создание CPU-данных.
    static bool LoadSceneCPU(
        const std::string& path,
        World& world
    );

    // Сохраняем прежний интерфейс для существующего кода.
    static bool LoadScene(
        const std::string& path,
        World& world,
        ResourceManager& resourceManager,
        RenderAdapter& renderer
    );
};