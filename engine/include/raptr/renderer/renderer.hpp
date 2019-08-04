/*!
  \file renderer.hpp
  This file handles the initialization and setup of the rendering window. It is also
  responsible for defining the interface of what can be rendererd and how an object
  should wrap itself to be rendered.
*/
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <SDL_opengl.h>

#include <raptr/common/filesystem.hpp>
#include <raptr/renderer/camera.hpp>
#include <sol/sol.hpp>

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/*!
Deleters for SDL objects that are allocated via std::shared_ptr
*/
struct SDLDeleter {
    void operator()(SDL_Texture* p) const;
    void operator()(SDL_Surface* p) const;
};

namespace raptr {
class Config;
class Entity;
class Parallax;
class Renderer;
class Text;
class Map;

struct CameraBasic {
    SDL_Point pos;
    int32_t left, right, top, bottom;
    int32_t min_x, max_x;
    int32_t min_y, max_y;
};

extern int32_t GAME_WIDTH;
extern int32_t GAME_HEIGHT;

class RenderInterface {
public:
    virtual ~RenderInterface() = default;
    virtual void render(Renderer* renderer) = 0;
};

class Renderable {
public:
    virtual ~Renderable() = default;
    virtual bool render(Renderer* renderer, const CameraClip& camera) = 0;
    bool absolute_positioning;
};

class RenderableRect : public Renderable {
public:
    SDL_Rect rect;
    SDL_Color color;

public:
    bool render(Renderer* renderer, const CameraClip& camera) override;
};

/*!
  A renderable surface should construct a Renderable struct that defines the
  texture to render, the source location within that texture, and destination
  of where it will be blitted on the screen. Additionally, if the texture should
  be flipped or rotated.
*/
class RenderableTexture : public Renderable {
public:
    //! The SDL texture that was generated, for examples see the Sprite class
    SDL_Texture* texture;

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

    /*!
    Convenience method to translate the flip booleans into SDL masks
    /returns SDL mask defining how the texture should be flipped
  */
    int flip_mask() const
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

    bool render(Renderer* renderer, const CameraClip& camera) override;
};

class MemoryPool {
public:
    static constexpr size_t size = 1 * 1024 * 1024;

    MemoryPool()
        : ptr(mem)
        , off(0)
    {
    }

    void* allocate(int mem_size)
    {
        assert((ptr + mem_size) <= (mem + sizeof mem) && "Pool exhausted!");
        void* mem = ptr;
        ptr += mem_size;
        off += mem_size;
        return mem;
    }

    char mem[size];
    char* ptr;
    size_t off;
};

/*!
  The Renderer is the class that will setup a Window and environment to render.
  It defines conveniences for creating textures from SDL surfaces, ways to add_texture
  a renderable object to the screen, and methods for interacting with the window
  itself, such as toggling fullscreen.
*/
class Renderer {
public:
    Renderer(bool is_headless_)
        : is_headless(is_headless_)
    {
    }

    Renderer(const Renderer&) = delete;
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
    void add_texture(std::shared_ptr<SDL_Texture> texture,
        SDL_Rect src, SDL_Rect dst,
        float angle, bool flip_x, bool flip_y,
        bool absolute_positioning = false,
        bool render_in_foreground = false);

    template <class T>
    void add_observable(std::shared_ptr<T> object)
    {
        observing.push_back(std::dynamic_pointer_cast<RenderInterface>(object));
    }

    void add_rect(SDL_Rect dst, SDL_Color color,
        bool absolute_positioning = false,
        bool render_in_foreground = false);

    void add_rect(Rect dst, SDL_Color color,
        bool absolute_positioning = false,
        bool render_in_foreground = false);

    /*!
    Add a background to be rendered, these are special as the rendering call is dependent on the viewport
    /param background- The background that will be rendered (first)
  */
    void add_background(std::shared_ptr<Parallax> background);

    /*!
    Add a foreground to be rendered, these are special as the rendering call is dependent on the viewport
    /param foreground- The foreground that will be rendered (first)
  */
    void add_foreground(std::shared_ptr<Parallax> foreground);

    std::shared_ptr<Text> add_text(const SDL_Point& position, const std::string& text,
        uint32_t size = 16, SDL_Color color = { 255, 255, 255, 255 });

    /*!
    Follow an entity so that the camera is centered on it
    /param entity - The entity to follow
  */
    void camera_follow(std::vector<std::shared_ptr<Entity>> entities);

    void camera_follow(std::shared_ptr<Entity> entity);

    /*!
    A utility method to create an SDL_Texture from an SDL_Surface using the current
    initialized SDL_Renderer
    /param surface - A shared pointer to a created SDL_Surface (see Sprite for an example)
    /return A freshly allocated texture created from the surface
  */
    SDL_Texture* create_texture(std::shared_ptr<SDL_Surface>& surface);

    /*!
    Initialize the renderer from a Configuration file
    /param config_ - The configuration that will be used to pull rendering parameters
    /return Whether the renderer could be setup
  */
    bool init(std::shared_ptr<Config>& config_);

    /*!
    Clears the screen and renders all added objects to the screen.
  */
    void run_frame(bool force_render = false);

    void scale(const float ratio);

    void scale_to_height(const int32_t height);

    void scale_to_width(const int32_t width);

    static void setup_lua_context(sol::state& state);

    /*!
    Toggles between a BORDERLESS fullscreen and Window mode
    /return Whether the window is in fullscreen or not
  */
    bool toggle_fullscreen();

public:
    bool is_headless;

    //! The configuration that was used to create this Renderer
    std::shared_ptr<Config> config;
    std::shared_ptr<Text> fps_text, num_obj_rendered_text, mempool_text;

    //! How many frames have been rendered
    uint64_t total_frames_rendered;
    uint64_t fps;
    uint64_t last_render_time_us;

    //! Camera position
    CameraBasic camera_basic;
    Camera camera;

    /*!
    An internal object for representing the SDL state of the renderer
  */
    struct SDLInternal {
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_GLContext gl;
    } sdl;

    SDL_Point zero_offset;
    SDL_Rect window_size;
    SDL_Rect logical_size;
    SDL_Rect desired_size;
    double current_ratio;
    double desired_ratio;
    double ratio_per_second;

    //! A list of Renderable objects that will be rendered on the next run_frame()
    std::vector<std::shared_ptr<RenderInterface>> observing;
    std::vector<Renderable*> will_render_middle;
    std::vector<Renderable*> will_render_foreground;
    std::vector<std::shared_ptr<Entity>> entities_followed;
    std::vector<std::shared_ptr<Parallax>> backgrounds;
    std::vector<std::shared_ptr<Parallax>> foregrounds;

    int64_t render_err_us;

    std::mutex mutex;

    bool show_fps;
    FileInfo game_root;
    std::mutex add_object_mutex;
    int64_t frame_counter_time_start;
    int32_t frame_counter;
    float frame_fps;

    MemoryPool texture_mem_pool;
};
} // namespace raptr
