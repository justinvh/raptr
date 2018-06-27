/*!
  \file character.hpp
  This module is responsible for the spawning of controllable entities in the world
*/
#pragma once

#include <cstdint>
#include <raptr/game/entity.hpp>

namespace raptr {

class Game;
class Sprite;
class Controller;
struct ControllerState;

/*! 
  A character is a subclass of an entity that is intended to be controlled.
  It has various movement options, a more complicated think function to handle interactions
  with its environment, and most importantly can have a controller attached to it.
 */
class Character : public Entity {
public:
  /*! 
    Attaches and registers bindings against a controller. This provides a mechanism for
    propagating events, such as button presses or controller movement, back to this class and 
    dispatching to the appropriate functions for handling the events.
    \param controller - The instance to a Controller object for registering callbacks against
  */
  virtual void attach_controller(std::shared_ptr<Controller> controller);

  /*! 
    Returns the bounding box for this character
    \return An rectangle containing the character
  */
  virtual Rect bbox() const;

  /*! 
    Triggers the "crouch" animation of the character. This will influence
    how the character moves. The crouch factor is multiplied against either the walk
    or run speed when this is called.
  */
  virtual void crouch();

  /*!
    Returns the unique id for this character in the world
    \return a 32-bit signed integer representing a unique entity id
  */
  virtual int32_t id() const;

  /*! 
    Returns true if this character intersects with another entity
    \param other - The entity that this character will attempt to intersect against
    \return Whether an intersection occured
  */
  virtual bool intersects(const Entity* other) const;

  /*! Stop all movement and return back to an idle position and animation */
  virtual void stop();

  /*!
    This method will determine how the character interacts with the game.
    This include computing position, velocity, and acceleration updates; how the game elements
    interact with the character; rendering the sprite;
    \param game - An instance of the game to retrieve event states, such as the current renderer
  */
  virtual void think(std::shared_ptr<Game> game);

  /*!
    Trigger the "Run" animation and set the movement speed to run speed 
    \param scale - A 0.0-1.0 multiplier that will be used to calculate the final run speed
  */
  virtual void run(float scale);

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
    When a controller is attached to this character, any right joystick motion will be dispatched to this
    function which will handle how best to deal with the motion angle and amplitude.
    \param state - What the controller was doing exactly when a button was pressed down
  */
  virtual bool on_right_joy(const ControllerState& state);

public:

  //! The sprite that is used to render this character
  std::shared_ptr<Sprite> sprite;

  //! The controller that is bound to this character
  std::shared_ptr<Controller> controller;

  //! The unique ID assigned
  int32_t _id;

  //! If the think() determines the character is falling down, then this will be set
  bool falling;

  //! The walk speed in pixels/sec
  uint32_t walk_speed;

  //! The run speed in pixels/sec
  uint32_t run_speed;

  //! The initial velocity (v(0)) for computing how high the character will jump
  uint32_t jump_vel;
};

}