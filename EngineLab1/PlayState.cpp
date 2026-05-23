#include "PlayState.h"
#include "SceneSerializer.h"

#include "RenderAdapter.h"
#include "Logger.h"
#include "Application.h"
#include "World.h"
#include "RenderSystem.h"
#include "raylib.h"

#include <cmath>

namespace {
    bool sceneCreated = false;
    Entity movingEntity = INVALID_ENTITY;
    float movementTime = 0.0f;
}

void PlayState::OnEnter() {
    Logger::Info("Entered PlayState");
    sceneCreated = false;
    movingEntity = INVALID_ENTITY;
    movementTime = 0.0f;
}

void PlayState::OnExit() {
    Logger::Info("Exited PlayState");
}

void PlayState::Update(float deltaTime, Application& app) {
    World& world = app.GetWorld();

    if (!sceneCreated) {
        SceneSerializer::LoadScene(
            "Assets/Scenes/demo_scene.json",
            world,
            resourceManager,
            app.GetRenderer()
        );

        sceneCreated = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        app.ChangeStateToMenu();
        return;
    }

    if (IsKeyPressed(KEY_F5)) {
        SceneSerializer::SaveScene(
            "Assets/Scenes/saved_scene.json",
            world
        );
    }

    if (movingEntity != INVALID_ENTITY && world.HasTransform(movingEntity)) {
        movementTime += deltaTime;

        TransformComponent& transform = world.GetTransform(movingEntity);
        transform.rotation.y += deltaTime * 45.0f;
        transform.position.y = std::sinf(movementTime * 2.0f) * 0.5f;
    }
}

void PlayState::Render(RenderAdapter& renderer, Application& app) {
    resourceManager.ReloadChangedShaders(renderer);

    renderer.Begin3D();

    RenderSystem renderSystem;
    renderSystem.Update(
        app.GetWorld(),
        renderer,
        resourceManager,
        app.GetDeltaTime()
    );

    renderer.End3D();

    renderer.DrawTextOnScreen("PLAY STATE (ECS + RESOURCES)", 160, 20, 30);
    renderer.DrawTextOnScreen("Mesh + Texture + Shader through ResourceManager", 120, 60, 20);
    renderer.DrawTextOnScreen("ESC - return to menu", 250, 90, 20);
}