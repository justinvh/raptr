/*!
  \file character.hpp
  This module is responsible for the spawning of controllable entities in the world
*/
#pragma once

#include <memory>
#include <cstdint>
#include <raptr/game/entity.hpp>
#include <raptr/common/filesystem.hpp>
#include <raptr/input/controller.hpp>

namespace raptr
{
class Game;
class Sprite;
class Renderer;
class Filesystem;

/*! 
  A character is a subclass of an entity that is intended to be controlled.
  It has various movement options, a more complicated think function to handle interactions
  with its environment, and most importantly can have a controller attached to it.
 */
class Character : public Entity
{
public:
  Character();
  ~Character() = default;
  Character(const Character&) = default;
  Character(Character&&) = default;
  Character& operator=(const Character&) = default;
  Character& operator=(Character&&) = default;

public:
  typedef CharacterSpawnEvent SpawnEvent;

  void serialize(std::vector<NetField>& list) override;

  bool deserialize(const std::vector<NetField>& fields) override;

  /*! 
    Attaches and registers bindings against a controller. This provides a mechanism for
    propagating events, such as button presses or controller movement, back to this class and 
    dispatching to the appropriate functions for handling the events.
    \param controller - The instance to a Controller object for registering callbacks against
  */
  virtual void attach_controller(std::shared_ptr<Controller>& controller);

  /*! 
    Returns the bounding box for this character
    \return An rectangle containing the character
  */
  std::vector<Rect> bbox() const override;

  /*! 
    Triggers the "crouch" animation of the character. This will influence
    how the character moves. The crouch factor is multiplied against either the walk
    or run speed when this is called.
  */
  virtual void crouch();

  /*!
    Generates a Character object from a TOML configuration file
    /param path - The path to the TOML file
    /return An instance of the character if found
  */
  static std::shared_ptr<Character> from_toml(const FileInfo& path);

  /*! Stop all movement and return back to an idle position and animation */
  virtual void stop();

  /*!
    This method will determine how the character interacts with the game.
    This include computing position, velocity, and acceleration updates; how the game elements
    interact with the character; rendering the sprite;
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  void think(std::shared_ptr<Game>& game) override;

  /*!
    Trigger the "Run" animation and set the movement speed to run speed 
    \param scale - A 0.0-1.0 multiplier that will be used to calculate the final run speed
  */
  virtual void run(float scale);

  /*!
  */
  void render(Renderer* renderer) override;

  /*!  
    Trigger the "Walk" animation and set the movement speed to walk speed
    \param scale - A 0.0-1.0 multiplier that will be used to calculate the final run speed
  */
  virtual void walk(float scale);

private:
  /*! 
    When a controller is attached to this character, any button presses will be dispatched to this
    function which will handle how best to deal with a button press
    \param state - What the controller was doing exactly when a button was pressed down
  */
  virtual bool on_button_down(const ControllerState& state);

  /*!
    When a controller is attached to this character, any button presses will be dispatched to this
    function which will handle how best to deal with a button press
    \param state - What the controller was doing exactly when a button was pressed down
  */
  virtual bool on_button_up(const ControllerState& state);

  /*!
    When a controller is attached to this character, any right joystick motion will be dispatched to this
    function which will handle how best to deal with the motion angle and amplitude.
    \param state - What the controller was doing exactly when a button was pressed down
  */
  virtual bool on_left_joy(const ControllerState& state);

public:

  //! The controller that is bound to this character
  std::shared_ptr<Controller> controller;

  //! Whether input has been made
  bool moving;

  //! Turn off/on character flashlight
  bool flashlight;
  std::shared_ptr<Sprite> flashlight_sprite;

  //! If the think() determines the character is falling down, then this will be set
  bool falling;

  //! If true, then a multiplier is put against the acceleration downwards
  bool fast_fall;

  //! Fast fall multiplier against gravity
  double fast_fall_scale;

  //! How long the character has been jumping
  int64_t jump_time_us;

  int64_t dash_length_usec;

  int64_t dash_time_usec;

  double dash_speed_ps;

  //! Current jump count
  uint32_t jump_count;

  //! If the player is in a double jump
  uint32_t jumps_allowed;

  //! Perfect jump scale
  double jump_perfect_scale;

  //! The walk speed in pixels/sec
  double walk_speed_ps;

  //! The run speed in pixels/sec
  double run_speed_ps;

  double mass_kg;

  //! The initial velocity (v(0)) for computing how high the character will jump
  double jump_vel_ps;

  int32_t bunny_hop_count;

  ControllerState last_controller_state, dash_controller_state;

  Point vel_exp;
};
} // namespace raptr
