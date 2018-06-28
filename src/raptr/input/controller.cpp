#include <iostream>
#include <memory>
#include <string>
#include <limits>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>

#include <raptr/common/logging.hpp>
#include <raptr/input/controller.hpp>

macro_enable_logger();

namespace raptr {


void Controller::on_button_down(const ControllerCallback& callback)
{
  button_down_callbacks.push_back(callback);
}

void Controller::on_button_up(const ControllerCallback& callback)
{
  button_up_callbacks.push_back(callback);
}

void Controller::on_left_joy(const ControllerCallback& callback)
{
  left_joy_callbacks.push_back(callback);
}

void Controller::on_right_joy(const ControllerCallback& callback)
{
  right_joy_callbacks.push_back(callback);
}


ControllerState state_from_event(SDL_GameController* controller, const SDL_Event& e)
{
  ControllerState state;

  int16_t axis = static_cast<int16_t>(e.caxis.axis);
  int16_t primary_axis = axis / 2;
  float mag[2] = {0, 0};
  float angle = 0.0;
  for (int i = primary_axis; i <= primary_axis + 1; ++i) {
    SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(i);
    const int16_t deadzone = 4000;
    const int16_t value = SDL_GameControllerGetAxis(controller, axis);
    if (value < -deadzone || value > deadzone) {
      float int16_t_max = static_cast<float>(std::numeric_limits<int16_t>::max());
      mag[i] = std::max(-1.0f, std::min(value / int16_t_max, 1.0f));
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

  if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
    state.button = Button::a;
  } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
    state.button = Button::b;
  }

  return state;
}

void Controller::process_event(const SDL_Event& e)
{
  ControllerState state;
  switch (e.type) {
    case SDL_CONTROLLERBUTTONDOWN:
      state = state_from_event(sdl.controller, e);
      for (auto& callback : button_down_callbacks) {
        callback(state);
      }
      break;
    case SDL_CONTROLLERAXISMOTION:
      state = state_from_event(sdl.controller, e);
      for (auto& callback : right_joy_callbacks) {
        callback(state);
      }
      break;
  }
}

std::shared_ptr<Controller> Controller::open(int controller_id)
{
  SDL_GameController* sdl_controller = SDL_GameControllerOpen(controller_id);
  SDL_GameControllerAddMappingsFromFile("C:/Users/justi/OneDrive/Documents/Visual Studio 2017/Projects/raptr/game/controls/gamecontrollerdb.txt");

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
}

Controller::SDLInternal::~SDLInternal() {
  if (controller) {
    SDL_GameControllerClose(controller);
  }
}

} // namespace raptr
