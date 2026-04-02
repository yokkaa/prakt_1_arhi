#pragma once
#include <memory>

class RenderAdapter;
class GameState;

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

private:
    std::unique_ptr<RenderAdapter> renderer;
    std::unique_ptr<GameState> currentState;

    bool isRunning;
    float deltaTime;
};