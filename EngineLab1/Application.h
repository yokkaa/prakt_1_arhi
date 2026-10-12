#pragma once

#include <memory>
#include "JobSystem.h"

class RenderAdapter;
class GameState;
class World;
class RenderSystem;
class InputManager;
class EventBus;

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();
    void Shutdown();

    void ChangeState(std::unique_ptr<GameState> newState);
    void ChangeStateToMenu();
    void ChangeStateToPlay();

    World& GetWorld();
    RenderAdapter& GetRenderer();
    InputManager& GetInput();
    EventBus& GetEventBus();
    float GetDeltaTime() const;

    JobSystem& GetJobs() {
        return *jobSystem;
    }

private:
    std::unique_ptr<JobSystem> jobSystem;
    std::unique_ptr<RenderAdapter> renderer;
    std::unique_ptr<GameState> currentState;
    std::unique_ptr<World> world;
    std::unique_ptr<RenderSystem> renderSystem;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<EventBus> eventBus;

    bool isRunning;
    float deltaTime;
};