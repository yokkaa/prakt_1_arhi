#include "MenuState.h"
#include "RenderAdapter.h"
#include "Logger.h"
#include "Application.h"
#include "InputManager.h"

void MenuState::OnEnter() {
    Logger::Info("Entered MenuState");
}

void MenuState::OnExit() {
    Logger::Info("Exited MenuState");
}

void MenuState::Update(float deltaTime, Application& app) {
    (void)deltaTime;

    InputManager& input = app.GetInput();

    if (input.IsActionPressed("Start")) {
        app.ChangeStateToPlay();
    }
}

void MenuState::Render(RenderAdapter& renderer, Application& app) {
    (void)app;
    renderer.DrawTextOnScreen("MENU STATE", 300, 150, 30);
    renderer.DrawTextOnScreen("Press ENTER to start ECS scene", 180, 220, 24);
}