/*!
  \file controller.hpp
  This module provides a class for interacting with a Game Controller 
*/
#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include <raptr/common/filesystem.hpp>

#undef SDL_JOYSTICK_DINPUT
#define SDL_JOYSTICK_WINMM 1
#undef SDL_HAPTIC_DINPUT
#define SDL_HAPTIC_DISABLED 1

#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

namespace raptr
{
/*!
  These buttons are modelled after a Xbox Controller
*/
enum class Button
{
  x,
  y,
  a,
  b,
  bump_left,
  bump_right,
  joy_left,
  joy_right,
  dpad_left,
  dpad_right,
  dpad_down,
  dpad_up,
  not_set
};

/*!
  When a Joy- or Button- callback is registered against the Controller class,
  then any callback will be provided with this ControllerState object which
  will be completely filled out
*/
struct ControllerState
{
  int32_t joystick;
  float angle;
  float magnitude;
  float x;
  float y;
  Button button;
};

//! The controller callback is a simple function that a caller class must define
using ControllerCallback = std::function<bool(const ControllerState& controller)>;

//! A sortable pair for vectors
using ControllerCallbackSortable = std::pair<int32_t, ControllerCallback>;

inline bool operator<(const ControllerCallbackSortable& a, const ControllerCallbackSortable& b)
{
  return a.first < b.first;
}

/*!
  This is an abstraction of a Game Pad, like an Xbox Controller. It provides methods
  for registering callbacks on certain events, like Button or Joystick actions, as
  well as a way for an SDL Event loop to push events to it
*/
class Controller : public std::enable_shared_from_this<Controller>
{
public:
  /*!
    Add a callback when a button is pressed down
    /param callback - The function to call when a button down event occurs
  */
  void on_button_down(const ControllerCallback& callback, int32_t priority = 0);

  /*!
    Add a callback when a button is released
    /param callback - The function to call when a button up event occurs
  */
  void on_button_up(const ControllerCallback& callback, int32_t priority = 0);

  /*!
    Add a callback when a left joystick is moved
    /param callback - The function to call when a left joystick event occurs
  */
  void on_left_joy(const ControllerCallback& callback, int32_t priority = 0);

  /*!
    Add a callback when a right joystick is moved
    /param callback - The function to call when a right joystick event occurs
  */
  void on_right_joy(const ControllerCallback& callback, int32_t priority = 0);

  /*!
    Given a controller id as specified by SDL, attach and register this class
    to that device and return back a new instance
    /param controller_id - The ID as it would be specified to SDL_GameControllerOpen
    /return An instance of the Controller or an empty container if failed
  */
  static std::shared_ptr<Controller> open(const FileInfo& game_root, int32_t controller_id);

  /*!
    A convenience method that processes an SDL Event to dispatch controller-specific
    events to the functions registered to this class
    /param e - The SDL_Event that was generated by SDL
  */
  void process_event(const SDL_Event& e);

  /*!
    Returns true if the controller is an SDL gamepad
    /return Whether the controller is a gamepad
  */
  bool is_gamepad() const;

  /*!
    The controller id that this class is responsible for
    /return The controller id
  */
  int32_t id() const
  {
    return sdl.controller_id;
  }

public:
  ControllerState state;

  //! List of callbacks that are called when a button down event occurs
  std::vector<ControllerCallbackSortable> button_down_callbacks;

  //! List of callbacks that are called when a button up event occurs
  std::vector<ControllerCallbackSortable> button_up_callbacks;

  //! List of callbacks that are called when a left joystick event occurs
  std::vector<ControllerCallbackSortable> left_joy_callbacks;

  //! List of callbacks that are called when a right joystick event occurs
  std::vector<ControllerCallbackSortable> right_joy_callbacks;

private:
  /*!
    An internal class for managing the SDL states of the controller
  */
  struct SDLInternal
  {
    ~SDLInternal();
    int32_t controller_id;
    SDL_GameController* controller;
    SDL_Joystick* joystick;
  } sdl;
};
} // namespace raptr
