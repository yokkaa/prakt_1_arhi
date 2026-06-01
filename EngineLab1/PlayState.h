#pragma once

#include "GameState.h"
#include "ResourceManager.h"
#include "PhysicsSystem.h"
#include "DebugRenderSystem.h"
#include "CameraSystem.h"
#include "PlayerControlSystem.h"

class PlayState : public GameState {
public:
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime, Application& app) override;
    void Render(RenderAdapter& renderer, Application& app) override;

private:
    ResourceManager resourceManager;
    PhysicsSystem physicsSystem;
    DebugRenderSystem debugRenderSystem;
    CameraSystem cameraSystem;
    PlayerControlSystem playerControlSystem;
    bool debugCollidersEnabled = false;
};
