/*!
  \file renderer.hpp
  This file handles the initialization and setup of the rendering window. It is also
  responsible for defining the interface of what can be rendererd and how an object
  should wrap itself to be rendered.
*/
#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <raptr/common/rect.hpp>

#include <vector>
#include <memory>

/*!
Deleters for SDL objects that are allocated via std::shared_ptr
*/
struct SDLDeleter {
  void operator()(SDL_Texture* p) const;
  void operator()(SDL_Surface* p) const;
  void operator()(TTF_Font* p) const;
};

namespace raptr {

class Config;
class Entity;

struct Camera {
  SDL_Point pos;
  int32_t left, right, top, bottom;
};

/*!
  A renderable surface should construct a Renderable struct that defines the
  texture to render, the source location within that texture, and destination
  of where it will be blitted on the screen. Additionally, if the texture should
  be flipped or rotated.
*/
struct Renderable {
  //! The SDL texture that was generated, for examples see the Sprite class
  std::shared_ptr<SDL_Texture> texture;

  //! The source rectangle within the texture to draw
  SDL_Rect src;

  //! Where on the screen the texture will be rendered
  SDL_Rect dst;

  //! The angle to rotate the texture
  float angle;

  //! Flip the texture along the X-axis, after the src has been cropped out
  bool flip_x;

  //! Flip the texture along the Y-axis, after the sr has been cropped out
  bool flip_y;

  //! Positioning
  bool absolute_positioning;

  /*!
    Convenience method to translate the flip booleans into SDL masks
    /returns SDL mask defining how the texture should be flipped
  */
  int flip_mask()
  {
    int out;
    if (flip_x && flip_y) {
      out = SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL;
    } else if (flip_x) {
      out = SDL_FLIP_HORIZONTAL;
    } else if (flip_y) {
      out = SDL_FLIP_VERTICAL;
    } else {
      out = SDL_FLIP_NONE;
    }
    return out;
  }
};

/*!
  The Renderer is the class that will setup a Window and environment to render.
  It defines conveniences for creating textures from SDL surfaces, ways to add
  a renderable object to the screen, and methods for interacting with the window
  itself, such as toggling fullscreen.
*/
class Renderer {
 public:
  Renderer() = default;
  Renderer(const Renderer&) = default;
  Renderer(Renderer&&) = default;
  ~Renderer();

 public:
  /*!
    Add an object to the renderer to be drawn on the screen.
    /see Renderable
    /param texture - An SDL_Texture to render (see Sprite for usage examples)
    /param src - The source rectangle within the texture to draw
    /param dst - Where on the screen the texture will be rendered
    /param angle - The angle to rotate the texture
    /param flip_x - Flip the texture along the X-axis, after the src has been cropped out
    /param flip_y - Flip the texture along the Y-axis, after the sr has been cropped out
  */
  void add(std::shared_ptr<SDL_Texture>& texture,
           SDL_Rect src, SDL_Rect dst,
           float angle,  bool flip_x, bool flip_y, bool absolute_positioning = false);

  /*!
    Follow an entity so that the camera is centered on it
    /param entity - The entity to follow
  */
  void camera_follow(std::shared_ptr<Entity>& entity);

  /*!
    A utility method to create an SDL_Texture from an SDL_Surface using the current
    initialized SDL_Renderer
    /param surface - A shared pointer to a created SDL_Surface (see Sprite for an example)
    /return A freshly allocated texture created from the surface
  */
  SDL_Texture* create_texture(std::shared_ptr<SDL_Surface>& surface) const;

  /*!
    Initialize the renderer from a Configuration file
    /param config_ - The configuration that will be used to pull rendering parameters
    /return Whether the renderer could be setup
  */
  bool init(std::shared_ptr<Config>& config_);

  /*!
    Clears the screen and renders all added objects to the screen.
  */
  void run_frame();

  /*!
    Toggles between a BORDERLESS fullscreen and Window mode
    /return Whether the window is in fullscreen or not
  */
  bool toggle_fullscreen();

 public:
  //! The configuration that was used to create this Renderer
  std::shared_ptr<Config> config;

  //! How many frames have been rendered
  uint64_t frame_count;

  //! Camera position
  Camera camera;

 private:
  /*!
    An internal object for representing the SDL state of the renderer
  */
  struct SDLInternal {
    SDL_Window* window;
    SDL_Renderer* renderer;
  } sdl;

  //! A list of Renderable objects that will be rendered on the next run_frame()
  std::vector<Renderable> will_render;
  std::shared_ptr<Entity> entity_followed;
};

} // namespace raptr
