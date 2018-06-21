#include <iostream>
#include <string>

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
  Uint32 timestamp = 0;
  std::string eventType;
  int index;
  std::string otherData;

  switch (e.type) {
    case SDL_JOYBUTTONUP:
      timestamp = e.jbutton.timestamp;
      eventType = "RELEASE";
      index = e.jbutton.button;
      otherData = "";
      break;
    case SDL_JOYBUTTONDOWN:
      timestamp = e.jbutton.timestamp;
      eventType = "PRESS";
      index = e.jbutton.button;
      otherData = "";
      break;
    case SDL_JOYHATMOTION:
      timestamp = e.jhat.timestamp;
      eventType = "POV";
      index = e.jhat.hat;
      break;
    case SDL_JOYAXISMOTION:
      timestamp = e.jaxis.timestamp;
      eventType = "STICK";
      index = e.jaxis.value;

      if (e.jaxis.axis == 0) {
        std::clog << timestamp << "\t" << eventType << "\t" << index << "\t" << otherData << "\n";
        for (auto func : right_joy_callbacks) {
          func(e.jaxis.value);
        }
      }

      break;
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
