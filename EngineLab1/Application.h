#pragma once
#include <memory>

class RenderAdapter;
class GameState;
class World;
class RenderSystem;

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
    float GetDeltaTime() const;

private:
    std::unique_ptr<RenderAdapter> renderer;
    std::unique_ptr<GameState> currentState;
    std::unique_ptr<World> world;
    std::unique_ptr<RenderSystem> renderSystem;

    bool isRunning;
    float deltaTime;
};