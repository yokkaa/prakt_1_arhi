#include "Application.h"
#include "RenderAdapter.h"
#include "RaylibRenderAdapter.h"
#include "GameState.h"
#include "MenuState.h"
#include "PlayState.h"
#include "Logger.h"
#include "World.h"
#include "RenderSystem.h"
#include "InputManager.h"
#include "EventBus.h"
#include "CollisionEvent.h"
#include "TriggerEvent.h"

#include <chrono>
#include <string>

Application::Application()
    : isRunning(false), deltaTime(0.0f) {
}

Application::~Application() {
    Shutdown();
}

bool Application::Init() {
    Logger::Info("Application initialization started");

    renderer = std::make_unique<RaylibRenderAdapter>();

    if (!renderer->Init(800, 600, "Engine Lab 2")) {
        Logger::Error("Renderer initialization failed");
        return false;
    }

    world = std::make_unique<World>();
    renderSystem = std::make_unique<RenderSystem>();
    inputManager = std::make_unique<InputManager>();
    eventBus = std::make_unique<EventBus>();

    inputManager->BindAction("Start", KeyCode::KEY_ENTER);
    inputManager->BindAction("BackToMenu", KeyCode::KEY_ESCAPE);
    inputManager->BindAction("SaveScene", KeyCode::KEY_F5);
    inputManager->BindAction("ToggleDebugColliders", KeyCode::KEY_F3);
    inputManager->BindAction("CameraForward", KeyCode::KEY_UP);
    inputManager->BindAction("CameraBackward", KeyCode::KEY_DOWN);
    inputManager->BindAction("CameraLeft", KeyCode::KEY_LEFT);
    inputManager->BindAction("CameraRight", KeyCode::KEY_RIGHT);
    inputManager->BindAction("CameraUp", KeyCode::KEY_E);
    inputManager->BindAction("CameraDown", KeyCode::KEY_Q);
    inputManager->BindAction("CameraLook", KeyCode::MOUSE_RIGHT);

    inputManager->BindAction("PlayerForward", KeyCode::KEY_W);
    inputManager->BindAction("PlayerBackward", KeyCode::KEY_S);
    inputManager->BindAction("PlayerLeft", KeyCode::KEY_A);
    inputManager->BindAction("PlayerRight", KeyCode::KEY_D);
    inputManager->BindAction("PlayerJump", KeyCode::KEY_SPACE);

    auto getEntityName = [this](Entity entity) {
        if (world && world->HasTag(entity)) {
            return world->GetTag(entity).name;
        }

        return std::string("Entity #") + std::to_string(entity);
    };

    eventBus->Subscribe<CollisionEvent>(
        [getEntityName](const CollisionEvent& event) {
            Logger::Info(
                "Collision detected between " +
                getEntityName(event.entityA) +
                " and " +
                getEntityName(event.entityB)
            );
        }
    );

    eventBus->Subscribe<TriggerEvent>(
        [getEntityName](const TriggerEvent& event) {
            Logger::Info(
                "Trigger entered: " +
                getEntityName(event.triggerEntity) +
                " with " +
                getEntityName(event.otherEntity)
            );
        }
    );

    ChangeState(std::make_unique<MenuState>());

    isRunning = true;
    Logger::Info("Application initialization completed");
    return true;
}

void Application::Run() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    while (isRunning && !renderer->ShouldClose()) {
        auto currentTime = clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        if (inputManager) {
            inputManager->UpdateState();
        }

        if (currentState) {
            currentState->Update(deltaTime, *this);
        }

        renderer->BeginFrame();
        renderer->Clear();

        if (currentState) {
            currentState->Render(*renderer, *this);
        }

        renderer->EndFrame();
    }
}

void Application::Shutdown() {
    if (currentState) {
        currentState->OnExit();
        currentState.reset();
    }

    eventBus.reset();
    inputManager.reset();
    renderSystem.reset();
    world.reset();

    if (renderer) {
        renderer->Shutdown();
        renderer.reset();
    }

    isRunning = false;
}

void Application::ChangeState(std::unique_ptr<GameState> newState) {
    if (currentState) {
        currentState->OnExit();
    }

    currentState = std::move(newState);

    if (currentState) {
        currentState->OnEnter();
    }
}

void Application::ChangeStateToMenu() {
    ChangeState(std::make_unique<MenuState>());
}

void Application::ChangeStateToPlay() {
    ChangeState(std::make_unique<PlayState>());
}

World& Application::GetWorld() {
    return *world;
}

RenderAdapter& Application::GetRenderer() {
    return *renderer;
}

InputManager& Application::GetInput() {
    return *inputManager;
}

EventBus& Application::GetEventBus() {
    return *eventBus;
}

float Application::GetDeltaTime() const {
    return deltaTime;
}