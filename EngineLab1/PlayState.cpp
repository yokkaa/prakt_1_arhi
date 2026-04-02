#include "PlayState.h"
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
        Entity e1 = world.CreateEntity();
        world.AddTag(e1, { "Red Triangle" });
        world.AddTransform(e1, { {200.0f, 300.0f}, 0.0f, {1.0f, 1.0f} });
        world.AddMeshRenderer(e1, { PrimitiveType::Triangle, {255, 0, 0, 255} });

        Entity e2 = world.CreateEntity();
        world.AddTag(e2, { "Blue Rectangle" });
        world.AddTransform(e2, { {420.0f, 300.0f}, 0.0f, {1.0f, 1.0f} });
        world.AddMeshRenderer(e2, { PrimitiveType::Rectangle, {0, 120, 255, 255} });

        Entity e3 = world.CreateEntity();
        world.AddTag(e3, { "Green Triangle" });
        world.AddTransform(e3, { {620.0f, 250.0f}, 0.0f, {0.8f, 0.8f} });
        world.AddMeshRenderer(e3, { PrimitiveType::Triangle, {0, 200, 100, 255} });

        movingEntity = e3;
        sceneCreated = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        app.ChangeStateToMenu();
        return;
    }

    if (movingEntity != INVALID_ENTITY && world.HasTransform(movingEntity)) {
        movementTime += deltaTime;
        TransformComponent& transform = world.GetTransform(movingEntity);
        transform.position.y = 250.0f + std::sinf(movementTime * 2.0f) * 50.0f;
    }
}

void PlayState::Render(RenderAdapter& renderer, Application& app) {
    renderer.DrawTextOnScreen("PLAY STATE (ECS)", 240, 20, 30);
    renderer.DrawTextOnScreen("Three entities with different transforms", 150, 60, 22);
    renderer.DrawTextOnScreen("ESC - return to menu", 250, 90, 20);

    RenderSystem renderSystem;
    renderSystem.Update(app.GetWorld(), renderer, app.GetDeltaTime());
}