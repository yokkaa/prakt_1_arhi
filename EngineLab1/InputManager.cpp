#include "InputManager.h"
#include "raylib.h"

#include <algorithm>

InputManager::InputManager()
    : trackedKeys{
        KeyCode::KEY_A, KeyCode::KEY_B, KeyCode::KEY_C, KeyCode::KEY_D, KeyCode::KEY_E,
        KeyCode::KEY_F, KeyCode::KEY_G, KeyCode::KEY_H, KeyCode::KEY_I, KeyCode::KEY_J,
        KeyCode::KEY_K, KeyCode::KEY_L, KeyCode::KEY_M, KeyCode::KEY_N, KeyCode::KEY_O,
        KeyCode::KEY_P, KeyCode::KEY_Q, KeyCode::KEY_R, KeyCode::KEY_S, KeyCode::KEY_T,
        KeyCode::KEY_U, KeyCode::KEY_V, KeyCode::KEY_W, KeyCode::KEY_X, KeyCode::KEY_Y,
        KeyCode::KEY_Z,

        KeyCode::KEY_ZERO, KeyCode::KEY_ONE, KeyCode::KEY_TWO, KeyCode::KEY_THREE,
        KeyCode::KEY_FOUR, KeyCode::KEY_FIVE, KeyCode::KEY_SIX, KeyCode::KEY_SEVEN,
        KeyCode::KEY_EIGHT, KeyCode::KEY_NINE,

        KeyCode::KEY_SPACE, KeyCode::KEY_ENTER, KeyCode::KEY_ESCAPE, KeyCode::KEY_TAB,
        KeyCode::KEY_LEFT_SHIFT, KeyCode::KEY_RIGHT_SHIFT,
        KeyCode::KEY_LEFT_CONTROL, KeyCode::KEY_RIGHT_CONTROL,
        KeyCode::KEY_UP, KeyCode::KEY_DOWN, KeyCode::KEY_LEFT, KeyCode::KEY_RIGHT,
        KeyCode::KEY_F1, KeyCode::KEY_F2, KeyCode::KEY_F3, KeyCode::KEY_F4,
        KeyCode::KEY_F5, KeyCode::KEY_F6, KeyCode::KEY_F7, KeyCode::KEY_F8,
        KeyCode::KEY_F9, KeyCode::KEY_F10, KeyCode::KEY_F11, KeyCode::KEY_F12,
        KeyCode::MOUSE_LEFT, KeyCode::MOUSE_RIGHT, KeyCode::MOUSE_MIDDLE
    } {
    for (KeyCode key : trackedKeys) {
        currentKeyStates[key] = false;
        previousKeyStates[key] = false;
    }

    currentMouseButtons[MOUSE_BUTTON_LEFT] = false;
    currentMouseButtons[MOUSE_BUTTON_RIGHT] = false;
    currentMouseButtons[MOUSE_BUTTON_MIDDLE] = false;

    previousMouseButtons = currentMouseButtons;
}

void InputManager::UpdateState() {
    previousKeyStates = currentKeyStates;
    previousMouseButtons = currentMouseButtons;
    previousMousePosition = mousePosition;

    for (KeyCode key : trackedKeys) {
        int raylibKey = ToRaylibKey(key);
        int mouseButton = ToRaylibMouseButton(key);

        if (raylibKey != 0) {
            currentKeyStates[key] = ::IsKeyDown(raylibKey);
        }
        else if (mouseButton != -1) {
            currentKeyStates[key] = ::IsMouseButtonDown(mouseButton);
        }
    }

    currentMouseButtons[MOUSE_BUTTON_LEFT] = ::IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    currentMouseButtons[MOUSE_BUTTON_RIGHT] = ::IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    currentMouseButtons[MOUSE_BUTTON_MIDDLE] = ::IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);

    Vector2 position = ::GetMousePosition();
    mousePosition = { position.x, position.y };
    mouseDelta = {
        mousePosition.x - previousMousePosition.x,
        mousePosition.y - previousMousePosition.y
    };
}

bool InputManager::IsKeyDown(KeyCode key) const {
    auto it = currentKeyStates.find(key);
    return it != currentKeyStates.end() && it->second;
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    auto current = currentKeyStates.find(key);
    auto previous = previousKeyStates.find(key);

    bool isCurrentDown = current != currentKeyStates.end() && current->second;
    bool wasPreviousDown = previous != previousKeyStates.end() && previous->second;

    return isCurrentDown && !wasPreviousDown;
}

bool InputManager::IsKeyReleased(KeyCode key) const {
    auto current = currentKeyStates.find(key);
    auto previous = previousKeyStates.find(key);

    bool isCurrentDown = current != currentKeyStates.end() && current->second;
    bool wasPreviousDown = previous != previousKeyStates.end() && previous->second;

    return !isCurrentDown && wasPreviousDown;
}

bool InputManager::IsMouseButtonDown(int buttonIndex) const {
    auto it = currentMouseButtons.find(buttonIndex);
    return it != currentMouseButtons.end() && it->second;
}

bool InputManager::IsMouseButtonPressed(int buttonIndex) const {
    auto current = currentMouseButtons.find(buttonIndex);
    auto previous = previousMouseButtons.find(buttonIndex);

    bool isCurrentDown = current != currentMouseButtons.end() && current->second;
    bool wasPreviousDown = previous != previousMouseButtons.end() && previous->second;

    return isCurrentDown && !wasPreviousDown;
}

bool InputManager::IsMouseButtonReleased(int buttonIndex) const {
    auto current = currentMouseButtons.find(buttonIndex);
    auto previous = previousMouseButtons.find(buttonIndex);

    bool isCurrentDown = current != currentMouseButtons.end() && current->second;
    bool wasPreviousDown = previous != previousMouseButtons.end() && previous->second;

    return !isCurrentDown && wasPreviousDown;
}

Vec2 InputManager::GetMousePosition() const {
    return mousePosition;
}

Vec2 InputManager::GetMouseDelta() const {
    return mouseDelta;
}

void InputManager::BindAction(const std::string& actionName, KeyCode keyCode) {
    actionsMap[actionName] = keyCode;

    if (std::find(trackedKeys.begin(), trackedKeys.end(), keyCode) == trackedKeys.end()) {
        trackedKeys.push_back(keyCode);
        currentKeyStates[keyCode] = false;
        previousKeyStates[keyCode] = false;
    }
}

bool InputManager::IsActionActive(const std::string& actionName) const {
    auto it = actionsMap.find(actionName);
    return it != actionsMap.end() && IsKeyDown(it->second);
}

bool InputManager::IsActionPressed(const std::string& actionName) const {
    auto it = actionsMap.find(actionName);
    return it != actionsMap.end() && IsKeyPressed(it->second);
}

bool InputManager::IsActionReleased(const std::string& actionName) const {
    auto it = actionsMap.find(actionName);
    return it != actionsMap.end() && IsKeyReleased(it->second);
}

int InputManager::ToRaylibKey(KeyCode key) const {
    switch (key) {
    case KeyCode::KEY_A: return KEY_A;
    case KeyCode::KEY_B: return KEY_B;
    case KeyCode::KEY_C: return KEY_C;
    case KeyCode::KEY_D: return KEY_D;
    case KeyCode::KEY_E: return KEY_E;
    case KeyCode::KEY_F: return KEY_F;
    case KeyCode::KEY_G: return KEY_G;
    case KeyCode::KEY_H: return KEY_H;
    case KeyCode::KEY_I: return KEY_I;
    case KeyCode::KEY_J: return KEY_J;
    case KeyCode::KEY_K: return KEY_K;
    case KeyCode::KEY_L: return KEY_L;
    case KeyCode::KEY_M: return KEY_M;
    case KeyCode::KEY_N: return KEY_N;
    case KeyCode::KEY_O: return KEY_O;
    case KeyCode::KEY_P: return KEY_P;
    case KeyCode::KEY_Q: return KEY_Q;
    case KeyCode::KEY_R: return KEY_R;
    case KeyCode::KEY_S: return KEY_S;
    case KeyCode::KEY_T: return KEY_T;
    case KeyCode::KEY_U: return KEY_U;
    case KeyCode::KEY_V: return KEY_V;
    case KeyCode::KEY_W: return KEY_W;
    case KeyCode::KEY_X: return KEY_X;
    case KeyCode::KEY_Y: return KEY_Y;
    case KeyCode::KEY_Z: return KEY_Z;

    case KeyCode::KEY_ZERO: return KEY_ZERO;
    case KeyCode::KEY_ONE: return KEY_ONE;
    case KeyCode::KEY_TWO: return KEY_TWO;
    case KeyCode::KEY_THREE: return KEY_THREE;
    case KeyCode::KEY_FOUR: return KEY_FOUR;
    case KeyCode::KEY_FIVE: return KEY_FIVE;
    case KeyCode::KEY_SIX: return KEY_SIX;
    case KeyCode::KEY_SEVEN: return KEY_SEVEN;
    case KeyCode::KEY_EIGHT: return KEY_EIGHT;
    case KeyCode::KEY_NINE: return KEY_NINE;

    case KeyCode::KEY_SPACE: return KEY_SPACE;
    case KeyCode::KEY_ENTER: return KEY_ENTER;
    case KeyCode::KEY_ESCAPE: return KEY_ESCAPE;
    case KeyCode::KEY_TAB: return KEY_TAB;
    case KeyCode::KEY_LEFT_SHIFT: return KEY_LEFT_SHIFT;
    case KeyCode::KEY_RIGHT_SHIFT: return KEY_RIGHT_SHIFT;
    case KeyCode::KEY_LEFT_CONTROL: return KEY_LEFT_CONTROL;
    case KeyCode::KEY_RIGHT_CONTROL: return KEY_RIGHT_CONTROL;
    case KeyCode::KEY_UP: return KEY_UP;
    case KeyCode::KEY_DOWN: return KEY_DOWN;
    case KeyCode::KEY_LEFT: return KEY_LEFT;
    case KeyCode::KEY_RIGHT: return KEY_RIGHT;

    case KeyCode::KEY_F1: return KEY_F1;
    case KeyCode::KEY_F2: return KEY_F2;
    case KeyCode::KEY_F3: return KEY_F3;
    case KeyCode::KEY_F4: return KEY_F4;
    case KeyCode::KEY_F5: return KEY_F5;
    case KeyCode::KEY_F6: return KEY_F6;
    case KeyCode::KEY_F7: return KEY_F7;
    case KeyCode::KEY_F8: return KEY_F8;
    case KeyCode::KEY_F9: return KEY_F9;
    case KeyCode::KEY_F10: return KEY_F10;
    case KeyCode::KEY_F11: return KEY_F11;
    case KeyCode::KEY_F12: return KEY_F12;

    default: return 0;
    }
}

int InputManager::ToRaylibMouseButton(KeyCode key) const {
    switch (key) {
    case KeyCode::MOUSE_LEFT: return MOUSE_BUTTON_LEFT;
    case KeyCode::MOUSE_RIGHT: return MOUSE_BUTTON_RIGHT;
    case KeyCode::MOUSE_MIDDLE: return MOUSE_BUTTON_MIDDLE;
    default: return -1;
    }
}
