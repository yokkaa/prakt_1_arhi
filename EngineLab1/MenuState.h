#pragma once
#include "GameState.h"

class MenuState : public GameState {
public:
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime, Application& app) override;
    void Render(RenderAdapter& renderer, Application& app) override;
};