#pragma once

#include <map>
#include <string>
#include <vector>

#include "Components.h"

enum class KeyCode {
    KEY_UNKNOWN,

    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

    KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
    KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE,

    KEY_SPACE,
    KEY_ENTER,
    KEY_ESCAPE,
    KEY_TAB,
    KEY_LEFT_SHIFT,
    KEY_RIGHT_SHIFT,
    KEY_LEFT_CONTROL,
    KEY_RIGHT_CONTROL,

    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,

    GAMEPAD_BUTTON_1
};

class IInputHandler {
public:
    virtual ~IInputHandler() = default;

    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0;
    virtual bool IsKeyReleased(KeyCode key) const = 0;

    virtual bool IsMouseButtonDown(int buttonIndex) const = 0;
    virtual bool IsMouseButtonPressed(int buttonIndex) const = 0;
    virtual bool IsMouseButtonReleased(int buttonIndex) const = 0;
    virtual Vec2 GetMousePosition() const = 0;
    virtual Vec2 GetMouseDelta() const = 0;
};

class InputManager final : public IInputHandler {
public:
    InputManager();

    void UpdateState();

    bool IsKeyDown(KeyCode key) const override;
    bool IsKeyPressed(KeyCode key) const override;
    bool IsKeyReleased(KeyCode key) const override;

    bool IsMouseButtonDown(int buttonIndex) const override;
    bool IsMouseButtonPressed(int buttonIndex) const override;
    bool IsMouseButtonReleased(int buttonIndex) const override;
    Vec2 GetMousePosition() const override;
    Vec2 GetMouseDelta() const override;

    void BindAction(const std::string& actionName, KeyCode keyCode);
    bool IsActionActive(const std::string& actionName) const;
    bool IsActionPressed(const std::string& actionName) const;
    bool IsActionReleased(const std::string& actionName) const;

private:
    std::map<KeyCode, bool> currentKeyStates;
    std::map<KeyCode, bool> previousKeyStates;

    std::map<int, bool> currentMouseButtons;
    std::map<int, bool> previousMouseButtons;

    Vec2 mousePosition;
    Vec2 previousMousePosition;
    Vec2 mouseDelta;
    std::map<std::string, KeyCode> actionsMap;
    std::vector<KeyCode> trackedKeys;

    int ToRaylibKey(KeyCode key) const;
    int ToRaylibMouseButton(KeyCode key) const;
};
