#pragma once

#include <memory>
#include <string>

#include <raptr/common/filesystem.hpp>

namespace raptr {
class Renderer;

class Text {
public:
    std::shared_ptr<SDL_Surface> surface;
    std::shared_ptr<SDL_Texture> texture;
    SDL_Rect bbox;
    bool allocate(Renderer& renderer);
    void render(Renderer* renderer, const SDL_Point& position);

public:
    static std::shared_ptr<Text> create(const FileInfo& game_root,
        const std::string& font,
        const std::string& text,
        int32_t size,
        const SDL_Color& fg,
        int32_t max_width = 400);
};
}
