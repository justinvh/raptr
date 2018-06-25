#include <iostream>
#include <string>
#include <limits>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>

#include "controller.hpp"

namespace raptr {

void Controller::on_button_down(const ButtonCallback& callback)
{
  button_down_callbacks.push_back(callback);
}

void Controller::on_button_up(const ButtonCallback& callback)
{
  button_up_callbacks.push_back(callback);
}

void Controller::on_left_joy(const JoyCallback& callback)
{
  left_joy_callbacks.push_back(callback);
}

void Controller::on_right_joy(const JoyCallback& callback)
{
  right_joy_callbacks.push_back(callback);
}

void Controller::process_event(const SDL_Event& e)
{
  switch (e.type) {
    case SDL_CONTROLLERAXISMOTION:
    {
      int16_t axis = static_cast<int16_t>(e.caxis.axis);

      int16_t primary_axis = axis / 2;
      float mag[2] = {0, 0};
      float angle = 0.0;
      for (int i = primary_axis; i <= primary_axis + 1; ++i) {
        SDL_GameControllerAxis axis = static_cast<SDL_GameControllerAxis>(i);
        const int16_t deadzone = 4000;
        const int16_t value = SDL_GameControllerGetAxis(sdl.controller, axis);
        if (value < -deadzone || value > deadzone) {
          mag[i] = std::max(-1.0f, std::min(value / float(std::numeric_limits<int16_t>::max()), 1.0f));
        }
      }

      float x = mag[0];
      float y = mag[1]; 
      float magnitude = sqrt(x * x + y * y);
      angle = atan2(y, x)  * 180.0f / M_PI;
      if (angle < 0) {
        angle += 360.0f;
      }

      for (auto& callback : right_joy_callbacks) {
        callback(primary_axis, angle, magnitude, x, y);
      }

      break;
    }
  }
}

std::shared_ptr<Controller> Controller::open(int controller_id)
{
  SDL_GameController* sdl_controller = SDL_GameControllerOpen(controller_id);

  std::shared_ptr<Controller> controller(new Controller());
  controller->sdl.controller = sdl_controller;
  controller->sdl.joystick = SDL_GameControllerGetJoystick(sdl_controller);
  controller->sdl.controller_id = SDL_JoystickGetDeviceInstanceID(controller_id);
  std::clog << "Registered " << SDL_GameControllerName(sdl_controller) << " as a controller with device id "
            << controller->sdl.controller_id << ".\n";

  return controller;
}

}
