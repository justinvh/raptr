/*!
  /file sprite.hpp
  This file provides a class that loads an Aseprite spritesheet to transform it
  into textures and animations to be rendered. It does not provide anything beyond
  the constructs for creating a texture.
*/
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <iostream>

#include <SDL.h>
#include <SDL_surface.h>

#include <raptr/common/filesystem.hpp>

namespace raptr {

class Renderer;

/*!
  A simple object for describing a frame in animation
*/
struct AnimationFrame {
  //! The name of the animation as described in the Aseprite JSON
  std::string name;

  //! The positional offsets of the texture spritesheet
  int32_t x, y, w, h;

  //! How long the frame should hold before continuing to the next
  uint32_t duration;
  
  //! The teeter pixel is the most down and left pixel occuped by the sprite
  int32_t teeter_px;
};

/*!
  These enumerations match what Asesprite can animate
*/
enum class AnimationDirection {
  forward,
  backward,
  ping_pong
};

/*!
  An animation is a collection of frames. This container class tracks the current
  frame, the animation direction, and what speed it should play the animation at
  (as a multiplier to the duration of the frames themselves)
*/
class Animation {
 public:
  //! The animation name as described by the "Tag" of the spritesheet
  std::string name;

  //! If true, then animation will stop on the last frame
  bool hold_last_frame;

  //! A state variable to track if the animation should go backwards
  bool ping_backwards;

  //! Current frame tracking for the frame and its frame numbers
  int32_t frame, from, to;

  //! A list of AnimationFrames that will be used to retrieve x,y,w,h cuts into the spritesheet
  std::vector<AnimationFrame> frames;

  //! How the animation will iterate through frames
  AnimationDirection direction;

  //! A speed multiplier against the duration of the frames (1.0 == same speed)
  float speed;

 public:
  /*!
    Retrieve the current AnimationFrame based on what "frame" is at
    /return The current AnimationFrame
  */
  AnimationFrame& current_frame();

  /*!
    AnimationFrames have a duration. If the time between this clock time and
    first time has exceeded the duration of the frame, then the frame will be advanced.
    /param clock_ms - The clock::ticks() milliseconds
    /return Whether the frame was advanced
  */
  bool next(int64_t clock_us);
};

//! A mapping of Animation names to the Animation themselves
using AnimationTable = std::map<std::string, Animation>;

/*!
  Using the Animation utilites above, a Sprite is simply the container of those.
  This class uses Aseprite's "Export Spritesheet" output (a PNG and JSON) to parse
  and create the animation that can then be controlled.
  /see StaticMesh
  /see Character
*/
class Sprite {
 public:
  /*!
    Create a new Sprite object from a Asesprite JSON file
    /param path - An absolute path to a Aseprite JSON file
    /return An instance of the Sprite if it can be constructed
  */
  static std::shared_ptr<Sprite> from_json(const FileInfo& path);

  /*!
    Render this sprite to a provided renderer
    /param renderer - The Renderer that this sprite should render to
  */
  void render(std::shared_ptr<Renderer>& renderer);

  /*!
    Change the current animation to a different one by name, such as "Idle" or "Walk"
    /param name - The name of the animation, such as "Idle" or "Walk"
    /param hold_last_frame - Whether the last frame of the animation should be held, instead of cycled
    /return Whether the animation could be set
  */
  bool set_animation(const std::string& name, bool hold_last_frame = false);

  /*!
    Check if a given animation exists
    /param name - The name of the animation to check
    /return Whether the animation exists
  */
  bool has_animation(const std::string& name);

 public:
  //! A mapping of animation names to the animation itself
  AnimationTable animations;

  //! The width and height of the Spritesheet
  int32_t width, height;

  //! The constructed SDL_Surface from the spritesheet
  std::shared_ptr<SDL_Surface> surface;

  //! The constructed SDL_Texture from the SDL_Surface of the spritesheet
  std::shared_ptr<SDL_Texture> texture;

  //! A convenince to track the current running animation
  Animation* current_animation;

  //! The rotation angle of the sprite
  float angle;

  //! Whether the sprite should  be flipped along an axis
  bool flip_x, flip_y;

  //! The x and y position of the sprite in the world
  double x, y;

  //! The width and height scale multipliers
  double scale;

  //! When the Sprite was last rendered
  int64_t last_frame_tick;

  //! Absolute positioning
  bool absolute_positioning;
};

} // namespace raptr
