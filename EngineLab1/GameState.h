#pragma once

class RenderAdapter;
class Application;

class GameState {
public:
    virtual ~GameState() = default;

    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;

    virtual void Update(float deltaTime, Application& app) = 0;
    virtual void Render(RenderAdapter& renderer, Application& app) = 0;
};
