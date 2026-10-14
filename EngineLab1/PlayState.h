#pragma once

#include <future>

#include "GameState.h"
#include "World.h"
#include "ResourceManager.h"
#include "RenderSystem.h"
#include "PhysicsSystem.h"
#include "DebugRenderSystem.h"
#include "CameraSystem.h"
#include "PlayerControlSystem.h"

class RenderAdapter;

class PlayState : public GameState {
public:
    void OnEnter() override;
    void OnExit() override;

    void Update(
        float deltaTime,
        Application& app
    ) override;

    void Render(
        RenderAdapter& renderer,
        Application& app
    ) override;

private:
    // Начинает загрузку обычной или замерочной сцены.
    void RequestScene(
        Application& app,
        bool benchmark
    );

    RenderAdapter* resourceRenderer = nullptr;

    RenderSystem renderSystem;
    ResourceManager resourceManager;

    PhysicsSystem physicsSystem;
    DebugRenderSystem debugRenderSystem;
    CameraSystem cameraSystem;
    PlayerControlSystem playerControlSystem;

    std::future<World> sceneFuture;

    // P — последовательный или параллельный расчёт.
    bool parallelRenderPreparation = true;
    bool asynchronousResourceLoading = true;

    // B — обычная или замерочная сцена.
    bool benchmarkSceneEnabled = false;

    bool sceneRequested = false;
    bool sceneReady = false;
    bool sceneFailed = false;
    bool debugCollidersEnabled = false;
};