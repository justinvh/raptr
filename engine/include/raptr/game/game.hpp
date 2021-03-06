/*!
  \file game.hpp
  This is the monolothic component of handling event dispatching and glueing the
  pieces all together.
*/
#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <crossguid/guid.hpp>
#include <sol/sol.hpp>

#include <raptr/common/filesystem.hpp>
#include <raptr/common/rect.hpp>
#include <raptr/common/rtree.hpp>
#include <raptr/network/snapshot.hpp>

namespace raptr {
class Config;
class Controller;
class Entity;
class Renderer;
class Sound;
class Map;

using IntersectEntityFilter = std::function<bool(const Entity*)>;
using IntersectCharacterFilter = std::function<bool(const Character*)>;

/*!
  The Game is a class that ties together the Renderer, Sound, Input, and Entities
  into one cohesive interaction. It can be thought of the main loop of the application
  with additional utilities and conveniences. 
*/
class Game : public std::enable_shared_from_this<Game>, public Serializable {
private:
    //! The class should be created using Game::create as it inherits from enable_shared_from_this
    Game(const fs::path& game_root_)
        : game_root(game_root_)
        , is_init(false)
    {
    }

public:
    ~Game();
    Game(const Game&) = delete;
    Game(Game&&) = default;
    Game& operator=(const Game&) = delete;
    Game& operator=(Game&&) = default;

    std::shared_ptr<Entity>& operator[](const std::string& key)
    {
        if (key.size() == 16) {
            std::array<unsigned char, 16> new_key;
            std::copy(key.begin(), key.end(), new_key.begin());
            const auto found = entity_lut.find(new_key);
            if (found != entity_lut.end()) {
                return found->second;
            }
        }
        return entity_short_lut[key];
    }

public:
    /*!
    This should be used when creating a Game. 
  */
    static std::shared_ptr<Game> create(const fs::path& game_root);

    template <class T>
    std::shared_ptr<T> get_entity(const std::string& key)
    {
        const auto entity = (*this)[key];
        return std::dynamic_pointer_cast<T>(entity);
    }

    /*!
    A headless server. One that does not render or use sound, etc.
  */
    static std::shared_ptr<Game> create_headless(const fs::path& game_root);

    template <class T>
    void add_event(T* event)
    {
        engine_events_buffers[engine_event_index].push_back(EngineEvent::create<T>(event));
    }

    void kill_character(const std::shared_ptr<Character>& character);
    void kill_entity(const std::shared_ptr<Entity>& entity);
    void kill_by_guid(const std::array<unsigned char, 16>& guid);

    void dispatch_event(const std::shared_ptr<EngineEvent>& event);

    void handle_load_map_event(const LoadMapEvent& event);

    void handle_character_spawn_event(const CharacterSpawnEvent& event);

    void handle_controller_event(const ControllerEvent& event);

    void handle_actor_spawn_event(const ActorSpawnEvent& event);

    void handle_trigger_spawn_event(const TriggerSpawnEvent& event);

    void setup_lua_context(sol::state& state);
    void lua_trigger_wrapper(
        sol::state& view,
        sol::table trigger_params,
        sol::protected_function lua_on_init,
        sol::protected_function lua_on_enter,
        sol::protected_function lua_on_exit);

public:
    bool interact_with_world(Entity* entity);

    bool intersect_world(Entity* entity, const Rect& bbox);

    bool intersect_anything(Entity* entity, const Rect& bbox);

    /*!
    Returns true if a given entity can teleport to a region defined by a bounding box
    \param entity - The entity that is trying to teleport
    \param bbox - The bounding box that the entity wants to teleport to
    \return The first entity that the object intersected or null if nothing
  */
    std::shared_ptr<Entity> intersect_entity(
        Entity* entity, const Rect& bbox,
        IntersectEntityFilter post_filter = [](const Entity*) -> bool { return true; });

    std::vector<std::shared_ptr<Entity>> intersect_entities(
        Entity* entity, const Rect& bbox,
        IntersectEntityFilter post_filter = [](const Entity*) -> bool { return true; },
        size_t limit = 0);

    std::shared_ptr<Character> intersect_character(
        Entity* entity, const Rect& bbox,
        IntersectCharacterFilter post_filter = [](const Character*) -> bool { return true; });

    std::vector<std::shared_ptr<Character>> intersect_characters(
        Entity* entity, const Rect& bbox,
        IntersectCharacterFilter post_filter = [](const Character*) -> bool { return true; },
        size_t limit = 0);

    void set_gravity(double m_s2);

    void spawn_now(const std::shared_ptr<Entity>& entity);

    /*!
    Spawn an entity to the world
  */
    void spawn_actor(
        const std::string& path,
        ActorSpawnEvent::Callback callback = [](auto& a) {
        });

    /*!
    Spawn an entity to the world
  */
    void spawn_character(
        const std::string& path,
        CharacterSpawnEvent::Callback callback = [](auto& a) {
        });

    /*
  */
    void spawn_player(
        int32_t controller_id,
        CharacterSpawnEvent::Callback callback = [](auto& a) {
        });

    /*
   */
    void spawn_trigger(
        const Rect& rect,
        TriggerSpawnEvent::Callback callback = [](auto& a) {
        });

    void load_map(
        const std::string& map_name,
        LoadMapEvent::Callback callback = [](auto& a) {
        });

    bool remove_entity_by_key(const std::string key);
    bool remove_entity(std::shared_ptr<Entity> entity);

    /*!
    Run the game and manage maintaining a healthy FPS
    \return Whether or not the game successfully ran
  */
    bool run();

    /*!
  */
    bool gather_engine_events();

    /*!
  */
    bool process_engine_events();

    /*!
    Top-level function to call all other init functions
    \return Whether all init functions passed or not
  */
    bool init();

    /*!
    Init stuff for a demo
  */
    bool init_demo();

    /*!
    Initialize and find controllers the game can use
    \return Whether a controller was found
  */
    bool init_controllers();

    /*!
    Initialize the filesystem. This allows us to quickly access 
    files relative to the game directory.
    \return Whether the filesystem could be constructed
  */
    bool init_filesystem();

    /*!
  */
    bool init_lua();

    /*!
    Initialize the renderer and setup the screen for game-time
    \return Whether the renderer could be initialized
  */
    bool init_renderer();

    /*!
    Initialize the sound engine on a separate thread
    \return Whether the sound engine could be initialized
  */
    bool init_sound();

    /*
  */
    bool poll_events();

    /*!
    Serialize and deserialize methods for the game
  */
    void serialize(std::vector<NetField>& list) override;
    bool deserialize(const std::vector<NetField>& fields) override;

    void show_collision_frames();

    void hide_collision_frames();

public:
    //! Global configuration loaded from a etc/raptr.ini
    std::shared_ptr<Config> config;

    //! Instance of the renderer that is being used by the game
    std::shared_ptr<Renderer> renderer;

    //! Instance of the sound engine being used
    std::shared_ptr<Sound> sound;

    //! The filesystem object for accessing files to the game
    FileInfo game_path;

    //! A mapping of controller device ID to controller instances
    std::vector<std::shared_ptr<Controller>> controllers_active;
    std::map<int32_t, std::shared_ptr<Controller>> controllers;
    std::map<int32_t, std::vector<std::shared_ptr<Character>>> controller_to_character;

    //! A list of all loaded entities in the game
    std::vector<std::shared_ptr<Entity>> entities;
    std::vector<std::shared_ptr<Character>> characters;

    //! A mapping of short-cut IDs
    std::map<std::string, std::shared_ptr<Entity>> entity_short_lut;

    //! A mapping of entity GUIDs to entities
    std::map<std::array<unsigned char, 16>, std::shared_ptr<Entity>> entity_lut;

    //! A map to find what the last known location of an entity was
    std::map<std::shared_ptr<Entity>, Point> last_known_entity_pos;
    std::map<std::shared_ptr<Entity>, Bounds> last_known_entity_bounds;

    //! The r-tree is used as a secondary test for bounding box interactions
    RTree<Entity*, double, 2> rtree;

    //! The root of the game folders to extract
    fs::path game_root;

    //! The gravity of the world
    double gravity_ps2;

    int64_t input_received_us;

    //! The number of ms since the last frame
    int64_t frame_delta_us;

    int64_t frame_last_time;

    int32_t engine_event_index = 0;
    std::vector<std::shared_ptr<EngineEvent>> engine_events_buffers[2];

    std::shared_ptr<Map> map;

public:
    //! If set, then all initialization has happened successfully
    bool is_init;
    bool is_headless;
    bool default_show_collision_frames;
    bool shutdown;

    bool use_threaded_renderer;
    std::thread renderer_thread;

    sol::state lua;
};
} // namespace raptr
