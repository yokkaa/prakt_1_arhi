#include "Application.h"
#include "RenderAdapter.h"
#include "RaylibRenderAdapter.h"
#include "GameState.h"
#include "MenuState.h"
#include "PlayState.h"
#include "Logger.h"
#include "World.h"
#include "RenderSystem.h"

#include <chrono>

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

float Application::GetDeltaTime() const {
    return deltaTime;
}