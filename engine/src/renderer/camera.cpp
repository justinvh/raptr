#include <raptr/game/entity.hpp>
#include <raptr/renderer/camera.hpp>
#include <raptr/renderer/renderer.hpp>

namespace raptr {

Camera::Camera(Point center, int32_t w, int32_t h)
{
    show_bounds = false;
    trap_size.x = 200;
    trap_size.y = 100;
    look.desired = center;
    look.current = center;

    bounds.current = Bounds(center.x - w / 2.0, center.x + w / 2.0,
        center.y - h / 2.0, center.y + h / 2.0);
    bounds.desired = bounds.current;
    this->set_min_size(w, h);
    this->update_constraints();
    lerp_time_us = 500 * 1000;
    x_px_us = 600 / 1e6;
    y_px_us = 300 / 1e6;
}

void Camera::set_min_size(int32_t w, int32_t h)
{
    min_size = Rect(0, 0, w, h);
}

void Camera::look_at(Point point)
{
    look.desired = point;
}

void Camera::think(Renderer* renderer, uint64_t delta_us)
{
    auto& sdl = renderer->sdl;
    auto& b = bounds.current;
    int32_t w = b.max[0] - b.min[0];
    int32_t h = b.max[1] - b.min[1];

    //SDL_RenderSetLogicalSize(sdl.renderer, w, h);

    clips.clear();

    Bounds new_bounds = b;
    float speed_x = 1.0;
    float speed_y = 1.0;
    if (tracking.empty()) {
        new_bounds = b;
    } else {
        trap_state = { false, false, false, false };
        new_bounds = Bounds(1e5, -1e5, 1e5, -1e5);
        int32_t hw = renderer->window_size.w / 2;
        int32_t hh = renderer->window_size.h / 2;
        Point avg = { 0, 0 };
        bool wait = false;
        for (auto& entity : tracking) {
            auto b = entity->bbox();
            auto p = entity->position_abs();
            auto& v = entity->velocity_rel();
            avg.x += p.x + b.w / 2.0;
            avg.y += p.y + b.h / 2.0;
            if (std::fabs(v.y) > 1e-3) {
                wait = true;
            }

            if (p.x <= trap.left.x) {
                speed_x = 3.0;
                trap_state.left = true;
            }

            if (p.x >= trap.right.x) {
                speed_x = 3.0;
                trap_state.right = true;
            }

            if (p.y <= trap.bottom.y) {
                speed_y = 3.0;
                trap_state.bottom = true;
                wait = false;
            }

            if (p.y >= trap.top.y) {
                speed_y = 2.0;
                trap_state.top = false;
            }
        }

        float n = tracking.size();
        Point d = { avg.x / n, avg.y / n };

        if (wait) {
            d.y = look.current.y;
        }

        look.desired = d;

        if (!(look.desired == look.current)) {
            if (look.current.x < look.desired.x) {
                look.current.x += speed_x * x_px_us * delta_us;
                if (look.current.x > look.desired.x) {
                    look.current.x = look.desired.x;
                }
            } else if (look.current.x > look.desired.x) {
                look.current.x -= speed_x * x_px_us * delta_us;
                if (look.current.x < look.desired.x) {
                    look.current.x = look.desired.x;
                }
            }

            if (look.current.y < look.desired.y) {
                look.current.y += speed_y * y_px_us * delta_us;
                if (look.current.y > look.desired.y) {
                    look.current.y = look.desired.y;
                }
            } else if (look.current.y > look.desired.y) {
                look.current.y -= speed_y * y_px_us * delta_us;
                if (look.current.y < look.desired.y) {
                    look.current.y = look.desired.y;
                }
            }
        }

        auto p = look.current;
        double left = p.x - hw;
        double right = p.x + hw;
        double top = p.y + hh;
        double bottom = p.y - hh;

        if (left < renderer->camera_basic.min_x) {
            left = renderer->camera_basic.min_x;
        }

        if (right > renderer->camera_basic.max_x) {
            right = renderer->camera_basic.max_x;
        }

        if (bottom < renderer->camera_basic.min_y) {
            bottom = renderer->camera_basic.min_y;
        }

        if (top > renderer->camera_basic.max_y) {
            top = renderer->camera_basic.max_y;
        }

        if ((right - left) < renderer->window_size.w) {
            if ((left - renderer->camera_basic.min_x) < 1e-3) {
                right = left + renderer->window_size.w;
            } else {
                left = right - renderer->window_size.w;
            }
        }

        if ((top - bottom) < renderer->window_size.h) {
            if ((bottom - renderer->camera_basic.min_y) < 1e-3) {
                top = bottom + renderer->window_size.h;
            } else {
                bottom = top - renderer->window_size.h;
            }
        }

        bounds.current = Bounds(left, right, bottom, top);
    }

    this->update_constraints();

    CameraClip cam;
    cam.clip.x = b.min[0];
    cam.clip.y = renderer->window_size.h - b.max[1];
    cam.clip.w = b.max[0] - b.min[0];
    cam.clip.h = b.max[1] - b.min[1];
    cam.left_offset = 0;
    cam.viewport.x = 0;
    cam.viewport.y = 0;
    cam.viewport.w = w;
    cam.viewport.h = h;

    clips.push_back(cam);
}

void Camera::render(Renderer* renderer, const CameraClip& c)
{
    if (!show_bounds) {
        return;
    }

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color red = { 255, 0, 0, 255 };

    renderer->add_rect(trap.left, trap_state.left ? red : white, false, false);
    renderer->add_rect(trap.right, trap_state.right ? red : white, false, false);
    renderer->add_rect(trap.top, trap_state.top ? red : white, false, false);
    renderer->add_rect(trap.bottom, trap_state.bottom ? red : white, false, false);

    SDL_Rect center_rect;
    center_rect.x = look.current.x - 4;
    center_rect.y = look.current.y - 4;
    center_rect.w = 8;
    center_rect.h = 8;
    renderer->add_rect(center_rect, white, false, false);

    center_rect.x = look.desired.x - 12;
    center_rect.y = look.desired.y - 12;
    center_rect.w = 24;
    center_rect.h = 24;
    renderer->add_rect(center_rect, red, false, false);
}

void Camera::track(std::shared_ptr<Entity> entity)
{
    tracking.push_back(entity);
}

void Camera::update_constraints()
{
    auto& b = bounds.current;
    const double inf_size = 10000.0;
    double w = b.max[0] - b.min[0];
    double h = b.max[1] - b.min[1];
    trap.left = Rect(b.min[0] + trap_size.x, b.min[1], 1, h);
    trap.right = Rect(b.max[0] - trap_size.x, b.min[1], 1, h);
    trap.bottom = Rect(b.min[0], b.min[1] + trap_size.y, w, 1);
    trap.top = Rect(b.min[0], b.max[1] - trap_size.y, w, 1);
}

}