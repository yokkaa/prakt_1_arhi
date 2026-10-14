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
#include "BenchmarkRecorder.h"

#include "tracy/Tracy.hpp"
#include "common/TracySystem.hpp"

#include <chrono>
#include <string>
#include <utility>

Application::Application()
    : isRunning(false),
      deltaTime(0.0f) {
}

Application::~Application() {
    Shutdown();
}

bool Application::Init() {
    tracy::SetThreadName("Main Thread");
    ZoneScopedN("Application::Init");

    Logger::Info("Application initialization started");

    jobSystem = std::make_unique<JobSystem>();

    Logger::Info(
        "JobSystem started. Workers: " +
        std::to_string(jobSystem->GetWorkerCount())
    );

    auto startupJob = jobSystem->Submit([]() {
        return 6 * 7;
    });

    Logger::Info(
        "JobSystem startup check: " +
        std::to_string(startupJob.get())
    );

    renderer = std::make_unique<RaylibRenderAdapter>();

    if (!renderer->Init(800, 600, "Engine Lab 2")) {
        Logger::Error("Renderer initialization failed");
        return false;
    }

    world = std::make_unique<World>();
    renderSystem = std::make_unique<RenderSystem>();
    inputManager = std::make_unique<InputManager>();
    eventBus = std::make_unique<EventBus>();

    inputManager->BindAction(
        "Start", KeyCode::KEY_ENTER
    );
    inputManager->BindAction(
        "BackToMenu", KeyCode::KEY_ESCAPE
    );
    inputManager->BindAction(
        "SaveScene", KeyCode::KEY_F5
    );
    inputManager->BindAction(
        "ToggleDebugColliders", KeyCode::KEY_F3
    );

    inputManager->BindAction(
        "CameraForward", KeyCode::KEY_UP
    );
    inputManager->BindAction(
        "CameraBackward", KeyCode::KEY_DOWN
    );
    inputManager->BindAction(
        "CameraLeft", KeyCode::KEY_LEFT
    );
    inputManager->BindAction(
        "CameraRight", KeyCode::KEY_RIGHT
    );
    inputManager->BindAction(
        "CameraUp", KeyCode::KEY_E
    );
    inputManager->BindAction(
        "CameraDown", KeyCode::KEY_Q
    );
    inputManager->BindAction(
        "CameraLook", KeyCode::MOUSE_RIGHT
    );

    inputManager->BindAction(
        "PlayerForward", KeyCode::KEY_W
    );
    inputManager->BindAction(
        "PlayerBackward", KeyCode::KEY_S
    );
    inputManager->BindAction(
        "PlayerLeft", KeyCode::KEY_A
    );
    inputManager->BindAction(
        "PlayerRight", KeyCode::KEY_D
    );
    inputManager->BindAction(
        "PlayerJump", KeyCode::KEY_SPACE
    );

    auto getEntityName = [this](Entity entity) {
        if (world && world->HasTag(entity)) {
            return world->GetTag(entity).name;
        }

        return std::string("Entity #") +
            std::to_string(entity);
    };

    eventBus->Subscribe<CollisionEvent>(
        [](const CollisionEvent&) {
            // Не печатаем столкновения каждый кадр.
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
    using Clock = std::chrono::steady_clock;

    auto lastTime = Clock::now();
    BenchmarkRecorder& recorder = BenchmarkRecorder::Get();

    while (isRunning && !renderer->ShouldClose()) {
        recorder.BeginFrame();

        const auto frameStarted = Clock::now();

        deltaTime = std::chrono::duration<float>(
            frameStarted - lastTime
        ).count();

        lastTime = frameStarted;

        {
            ZoneScopedN("Application::Frame");

            {
                ZoneScopedN("Application::Input");

                if (inputManager) {
                    inputManager->UpdateState();
                }
            }

            {
                ZoneScopedN("Application::Update");

                if (currentState) {
                    currentState->Update(deltaTime, *this);
                }
            }

            {
                ZoneScopedN("Application::Render");

                renderer->BeginFrame();
                renderer->Clear();

                if (currentState) {
                    currentState->Render(*renderer, *this);
                }
            }

            {
                ZoneScopedN("Application::EndFrame");

                renderer->EndFrame();
            }
        }

        const auto frameFinished = Clock::now();

        const double frameMs =
            std::chrono::duration<double, std::milli>(
                frameFinished - frameStarted
            ).count();

        FrameMark;

        // Сборщик сам пропускает прогрев.
        // Сохранение CSV после завершения прогона
        // не входит в измеренное время кадра.
        recorder.RecordFrame(frameMs);
    }
}

void Application::Shutdown() {
    ZoneScopedN("Application::Shutdown");

    BenchmarkRecorder::Get().Cancel();

    // Завершаем фоновые задачи до уничтожения ресурсов.
    if (jobSystem) {
        jobSystem->Shutdown();
    }

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

void Application::ChangeState(
    std::unique_ptr<GameState> newState
) {
    ZoneScopedN("Application::ChangeState");

    // Смена состояния прерывает текущий замер,
    // чтобы в него не попали кадры другой сцены или меню.
    BenchmarkRecorder::Get().Cancel();

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