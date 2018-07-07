#include <iostream>
#include <memory>
#include <string>
#include <limits>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>

#include <raptr/common/logging.hpp>
#include <raptr/input/controller.hpp>

namespace { auto logger = raptr::_get_logger(__FILE__); };

namespace raptr {


void Controller::on_button_down(const ControllerCallback& callback, int32_t priority)
{
  button_down_callbacks.push_back(std::make_pair(priority, callback));
  std::sort(button_down_callbacks.begin(), button_down_callbacks.end());
}

void Controller::on_button_up(const ControllerCallback& callback, int32_t priority)
{
  button_up_callbacks.push_back(std::make_pair(priority, callback));
  std::sort(button_up_callbacks.begin(), button_up_callbacks.end());
}

void Controller::on_left_joy(const ControllerCallback& callback, int32_t priority)
{
  left_joy_callbacks.push_back(std::make_pair(priority, callback));
  std::sort(left_joy_callbacks.begin(), left_joy_callbacks.end());
}

void Controller::on_right_joy(const ControllerCallback& callback, int32_t priority)
{
  right_joy_callbacks.push_back(std::make_pair(priority, callback));
  std::sort(right_joy_callbacks.begin(), right_joy_callbacks.end());
}

ControllerState state_from_joystick_event(SDL_Joystick* controller, const SDL_Event& e)
{
  float mag[2] = {0.0f, 0.0f};
  ControllerState state;

  int16_t primary_axis = 0;

  float angle = 0.0;
  for (size_t i = 0, k = 0; i <= 1; ++i, ++k) {
    const int16_t deadzone = 4000;
    const int16_t value = SDL_JoystickGetAxis(controller, i);
    if (value < -deadzone || value > deadzone) {
      float int16_t_max = 27000; //static_cast<float>(std::numeric_limits<int16_t>::max());
      mag[k] = std::max(-1.0f, std::min(value / int16_t_max, 1.0f));
    }
  }

  float x = mag[0];
  float y = mag[1];
  float magnitude = sqrt(x * x + y * y);
  angle = static_cast<float>(atan2(y, x)  * 180.0f / M_PI);
  if (angle < 0) {
    angle += 360.0f;
  }

  state.x = x;
  state.y = y;
  state.magnitude = magnitude;
  state.angle = angle;
  state.joystick = primary_axis;

  if (e.jbutton.button == 0) {
    state.button = Button::a;
  } else if (e.jbutton.button == 1) {
    state.button = Button::b;
  } else {
    state.button = Button::not_set;
  }

  return state;
}
ControllerState state_from_joystick_event(SDL_GameController* controller, const SDL_Event& e)
{
  float mag[2] = {0.0f, 0.0f};
  ControllerState state;

  int16_t primary_axis = 0;

  switch (e.caxis.axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
    case SDL_CONTROLLER_AXIS_LEFTY:
      primary_axis = static_cast<int16_t>(SDL_CONTROLLER_AXIS_LEFTX);
      break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
    case SDL_CONTROLLER_AXIS_RIGHTY:
      primary_axis = static_cast<int16_t>(SDL_CONTROLLER_AXIS_RIGHTX);
      break;
    default:
      logger->debug("Unhandled axis event for the controller");
      primary_axis = 0;
  }

  float angle = 0.0;
  for (size_t i = primary_axis, k = 0; i <= primary_axis + 1; ++i, ++k) {
    SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(i);
    const int16_t deadzone = 4000;
    const int16_t value = SDL_GameControllerGetAxis(controller, axis);
    if (value < -deadzone || value > deadzone) {
      float int16_t_max = static_cast<float>(std::numeric_limits<int16_t>::max());
      mag[k] = std::max(-1.0f, std::min(value / int16_t_max, 1.0f));
    }
  }

  float x = mag[0];
  float y = mag[1];
  float magnitude = sqrt(x * x + y * y);
  angle = static_cast<float>(atan2(y, x)  * 180.0f / M_PI);
  if (angle < 0) {
    angle += 360.0f;
  }

  state.x = x;
  state.y = y;
  state.magnitude = magnitude;
  state.angle = angle;
  state.joystick = primary_axis;

  if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
    state.button = Button::a;
  } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
    state.button = Button::b;
  } else {
    state.button = Button::not_set;
  }

  return state;
}

void Controller::process_event(const SDL_Event& e)
{
  ControllerState state;
  switch (e.type) {
    case SDL_JOYBUTTONDOWN:
      state = state_from_joystick_event(sdl.joystick, e);
      for (auto& callback : button_down_callbacks) {
        // If a callback returns false it means do not bubble
        if (!callback.second(state)) {
          break;
        }
      }
      break;
    case SDL_CONTROLLERBUTTONDOWN:
      state = state_from_joystick_event(sdl.controller, e);
      for (auto& callback : button_down_callbacks) {
        // If a callback returns false it means do not bubble
        if (!callback.second(state)) {
          break;
        }
      }
      break;
    case SDL_JOYAXISMOTION:
      state = state_from_joystick_event(sdl.joystick, e);
      // If a callback returns false it means do not bubble
      for (auto& callback : left_joy_callbacks) {
        if (!callback.second(state)) {
          break;
        }
      }
      break;
    case SDL_CONTROLLERAXISMOTION:
      int16_t axis = static_cast<int16_t>(e.caxis.axis) / 2;
      if (axis == 0 || axis == 1) {
        state = state_from_joystick_event(sdl.controller, e);
        switch (axis) {
          case 0:
            // If a callback returns false it means do not bubble
            for (auto& callback : left_joy_callbacks) {
              if (!callback.second(state)) {
                break;
              }
            }
            break;
          case 1:
            // If a callback returns false it means do not bubble
            for (auto& callback : right_joy_callbacks) {
              if (!callback.second(state)) {
                break;
              }
            }
            break;
        }
      }
      break;
  }
}

bool Controller::is_gamepad()
{
  return sdl.controller != nullptr;
}

std::shared_ptr<Controller> Controller::open(const FileInfo& game_root, int controller_id)
{
  if (SDL_IsGameController(controller_id)) {
    SDL_GameController* sdl_controller = SDL_GameControllerOpen(controller_id);
    char* mapping = SDL_GameControllerMapping(sdl_controller);

    if (!mapping) {
      logger->error("There are no controller mappings available for {}", SDL_GameControllerName(sdl_controller));
      SDL_GameControllerClose(sdl_controller);
      throw std::runtime_error("No available controller mappings");
    } else {
      SDL_free(mapping);
    }

    std::shared_ptr<Controller> controller(new Controller());
    controller->sdl.controller = sdl_controller;
    controller->sdl.joystick = SDL_GameControllerGetJoystick(sdl_controller);
    controller->sdl.controller_id = SDL_JoystickGetDeviceInstanceID(controller_id);
    logger->info("Registered {} as a controller with device id {}",
      SDL_GameControllerName(sdl_controller),
      controller->sdl.controller_id);

    SDL_GameControllerEventState(SDL_ENABLE);

    return controller;
  } else {
    std::shared_ptr<Controller> controller(new Controller());
    controller->sdl.controller = nullptr;
    controller->sdl.joystick = SDL_JoystickOpen(controller_id);
    controller->sdl.controller_id = SDL_JoystickGetDeviceInstanceID(controller_id);
    SDL_JoystickEventState(SDL_ENABLE);
    logger->info("Registered {} as a joystick with device id {}",
      SDL_JoystickName(controller->sdl.joystick),
      controller->sdl.controller_id);
    return controller;
  }
}

Controller::SDLInternal::~SDLInternal() {
  if (controller) {
    SDL_GameControllerClose(controller);
  }

  if (joystick) {
    SDL_JoystickClose(joystick);
  }
}

} // namespace raptr
