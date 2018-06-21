#pragma once

#undef SDL_JOYSTICK_DINPUT
#define SDL_JOYSTICK_WINMM 1
#undef SDL_HAPTIC_DINPUT
#define SDL_HAPTIC_DISABLED 1

#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_events.h>

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

namespace raptr {

enum class Button {
  x, y, a, b, bump_left, bump_right, joy_left, joy_right,
  dpad_left, dpad_right, dpad_down, dpad_up
};

using ButtonCallback = std::function<bool(const Button& button)>;
using JoyCallback = std::function<bool(int32_t joystick, float angle, float magnitude, float x, float y)>;

class Controller : public std::enable_shared_from_this<Controller> {
public:
  void on_button_down(const ButtonCallback& callback);
  void on_button_up(const ButtonCallback& callback);
  void on_left_joy(const JoyCallback& callback);
  void on_right_joy(const JoyCallback& callback);

  static std::shared_ptr<Controller> open(int32_t controller_id);

public:
  std::vector<ButtonCallback> button_down_callbacks;
  std::vector<ButtonCallback> button_up_callbacks;
  std::vector<JoyCallback> left_joy_callbacks;
  std::vector<JoyCallback> right_joy_callbacks;

  void process_event(const SDL_Event& e);
  int32_t id() { return sdl.controller_id;  }

private:

  struct SDLInternal {
    ~SDLInternal()
    {
      if (controller) {
        SDL_GameControllerClose(controller);
      }
    }
    int32_t controller_id;
    SDL_GameController* controller;
    SDL_Joystick* joystick;
  } sdl;

};

};
