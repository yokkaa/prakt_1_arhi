#include "PlayState.h"
#include "SceneSerializer.h"

#include "RenderAdapter.h"
#include "Logger.h"
#include "Application.h"
#include "World.h"
#include "RenderSystem.h"
#include "PhysicsSystem.h"
#include "InputManager.h"
#include "DebugRenderSystem.h"
#include "CameraSystem.h"
#include "PlayerControlSystem.h"

#include <cmath>
#include <string>

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
    debugCollidersEnabled = false;
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

        bool hasCamera = false;
        for (Entity entity : world.GetEntities()) {
            if (world.HasCamera(entity)) {
                hasCamera = true;
                break;
            }
        }

        if (!hasCamera) {
            Entity cameraEntity = world.CreateEntity();
            world.AddTag(cameraEntity, Tag{ "Main Camera" });
            world.AddCamera(cameraEntity, CameraComponent{});
        }

        cameraSystem.Update(world, app.GetInput(), app.GetRenderer(), 0.0f);

        sceneCreated = true;
    }

    InputManager& input = app.GetInput();

    if (input.IsActionPressed("BackToMenu")) {
        app.ChangeStateToMenu();
        return;
    }

    if (input.IsActionPressed("SaveScene")) {
        SceneSerializer::SaveScene(
            "Assets/Scenes/saved_scene.json",
            world
        );
    }

    if (input.IsActionPressed("ToggleDebugColliders")) {
        debugCollidersEnabled = !debugCollidersEnabled;
        Logger::Info(
            std::string("Debug collision rendering ") +
            (debugCollidersEnabled ? "enabled" : "disabled")
        );
    }

    playerControlSystem.Update(world, input, deltaTime);
    physicsSystem.Update(world, deltaTime, &app.GetEventBus());
    cameraSystem.Update(world, input, app.GetRenderer(), deltaTime);

    if (movingEntity != INVALID_ENTITY && world.HasTransform(movingEntity)) {
        movementTime += deltaTime;

        TransformComponent& transform = world.GetTransform(movingEntity);
        transform.rotation.y += deltaTime * 45.0f;
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

    if (debugCollidersEnabled) {
        debugRenderSystem.RenderColliders(app.GetWorld(), renderer);
    }

    renderer.End3D();

    renderer.DrawTextOnScreen("DEMO SCENE: PHYSICS + INPUT + DEBUG", 95, 20, 30);
    renderer.DrawTextOnScreen("WASD + Space - control Player Cube", 145, 60, 20);
    renderer.DrawTextOnScreen("Arrows/QE - camera, hold RMB - look, Shift - faster", 70, 90, 20);
    renderer.DrawTextOnScreen("F3 - debug colliders/triggers, F5 - save scene, ESC - menu", 80, 120, 20);
    renderer.DrawTextOnScreen("Green sphere = SphereCollider, yellow = Trigger Zone", 105, 150, 20);

    if (debugCollidersEnabled) {
        renderer.DrawTextOnScreen("DEBUG COLLIDERS: ON", 20, 550, 20);
    }
}