#include "PlayState.h"
#include "RenderAdapter.h"
#include "Logger.h"
#include "Application.h"
#include "raylib.h"

void PlayState::OnEnter() {
    Logger::Info("Entered PlayState");
}

void PlayState::OnExit() {
    Logger::Info("Exited PlayState");
}

void PlayState::Update(float deltaTime, Application& app) {
    (void)deltaTime;

    if (IsKeyPressed(KEY_ESCAPE)) {
        app.ChangeStateToMenu();
    }
}

void PlayState::Render(RenderAdapter& renderer) {
    renderer.DrawTextOnScreen("PLAY STATE", 300, 40, 30);
    renderer.DrawTextOnScreen("Press ESC to return to menu", 210, 80, 22);
    renderer.DrawTriangle();
}