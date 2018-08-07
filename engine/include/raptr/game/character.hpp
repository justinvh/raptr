/*!
  \file character.hpp
  This module is responsible for the spawning of controllable entities in the world
*/
#pragma once

#include <memory>
#include <cstdint>
#include <raptr/game/entity.hpp>
#include <raptr/game/actor.hpp>
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
  Character(const Character&) = delete;
  Character(Character&&) = delete;
  Character& operator=(const Character&) = delete;
  Character& operator=(Character&&) = delete;

public:
  typedef CharacterSpawnEvent SpawnEvent;

  static void setup_lua_context(sol::state& state);

  void serialize(std::vector<NetField>& list) override;

  bool deserialize(const std::vector<NetField>& fields) override;

  /*! 
    Attaches and registers bindings against a controller. This provides a mechanism for
    propagating events, such as button presses or controller movement, back to this class and 
    dispatching to the appropriate functions for handling the events.
    \param controller - The instance to a Controller object for registering callbacks against
  */
  virtual void attach_controller(std::shared_ptr<Controller>& controller);

  virtual void detach_controller();

  /*! 
    Returns the bounding box for this character
    \return An rectangle containing the character
  */
  Rect bbox() const override;

  /*!
    Generates a Character object from a TOML configuration file
    /param path - The path to the TOML file
    /return An instance of the character if found
  */
  static std::shared_ptr<Character> from_toml(const FileInfo& path);

  /*!
    Crouch, reducing friction
   */
  virtual void crouch();

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
    Kill this character. It will not accept any more inputs.
   */
  virtual void kill();

  /*!
  */
  void render(Renderer* renderer) override;

  /*!  
    Trigger the "Walk" animation and set the movement speed to walk speed
    \param scale - A 0.0-1.0 multiplier that will be used to calculate the final run speed
  */
  virtual void walk(float scale);

  /*!
  */
  virtual void move_to(double x, double y, float scale);
  virtual void move_to_rel(double x, double y, float scale);

  /*!
  */
  virtual void walk_to(double x, double y);
  virtual void walk_to_rel(double x, double y);

  /*!
  */
  virtual void run_to(double x, double y);
  virtual void run_to_rel(double x, double y);

  /*!
  */
  virtual void jump();

  /*!
   */
  virtual void fall();

  /*
  */
  virtual void dash();

  /*
  */
  virtual void turn_around();


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

  void set_animation(const std::string& name);

public:

  //! The controller that is bound to this character
  std::shared_ptr<Controller> controller;

  //! Whether input has been made
  bool moving;

  //! Turn off/on character flashlight
  bool flashlight;
  std::shared_ptr<Sprite> flashlight_sprite;

  //! If true, then a tweening is occuring
  bool is_tweening;

  //! If the think() determines the character is falling down, then this will be set
  bool is_falling;

  bool is_crouched;

  bool activate_tile;

  //! If true, then a multiplier is put against the acceleration downwards
  bool fast_fall;

  //! Fast fall multiplier against gravity
  double fast_fall_scale;

  //! How long the character has been jumping
  int64_t jump_time_current_us;

  int64_t dash_length_usec;

  int64_t dash_time_usec;

  double dash_speed_ps;

  Point jump_point;

  double jump_height_px;
  uint32_t jump_time_ms;
  double rise_fall_vel_px;

  //! Current jump count
  uint32_t jump_count;

  //! If the player is in a double jump
  uint32_t jumps_allowed;

  //! The walk speed in pixels/sec
  double walk_speed_ps;

  //! The run speed in pixels/sec
  double run_speed_ps;

  double mass_kg;

  //! The initial velocity (v(0)) for computing how high the character will jump
  double jump_vel_ps;

  int32_t bunny_hop_count;
  int32_t on_left_joy_id;
  int32_t on_button_down_id;
  int32_t on_button_up_id;

  uint64_t think_frame;

  ControllerState last_controller_state, dash_controller_state;

  Point vel_exp;

  bool is_scripted;
  sol::state lua;
  FileInfo lua_script_fileinfo;
  std::string lua_script;
};
} // namespace raptr
