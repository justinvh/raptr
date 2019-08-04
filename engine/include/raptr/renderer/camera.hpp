#pragma once

#include <SDL_rect.h>
#include <memory>
#include <raptr/common/rect.hpp>
#include <vector>

namespace raptr {

class Entity;
class Renderer;

struct CameraTrap {
    Rect top, left, right, bottom;
};

struct CameraBounds {
    Bounds current;
    Bounds desired;
};

struct CameraPoint {
    Point current;
    Point desired;
};

struct CameraClip {
    SDL_Rect clip, viewport;
    int32_t left_offset;
    std::vector<std::shared_ptr<Entity>> contains;
};

struct CameraTrapState {
    bool left, right, top, bottom;
};

class Camera {
public:
    Camera() = default;
    Camera(Point center, int32_t w, int32_t h);
    void track(std::shared_ptr<Entity> entity);
    void set_min_size(int32_t w, int32_t h);
    void update_constraints();
    void think(Renderer* renderer, uint64_t delta_us);
    void look_at(Point point);
    void render(Renderer* renderer, const CameraClip& clip);

public:
    std::vector<std::shared_ptr<Entity>> tracking;
    Point trap_size;
    Rect min_size;
    std::vector<CameraClip> clips;
    int64_t transition_time;
    CameraBounds bounds;
    CameraTrap trap;
    CameraTrapState trap_state;
    CameraPoint look;
    int64_t lerp_time_us;
    double x_px_us;
    double y_px_us;
    bool show_bounds;
};

}