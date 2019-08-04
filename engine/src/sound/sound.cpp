#include <map>
#include <string>
#include <thread>

#include <SDL_mixer.h>

#include <raptr/common/logging.hpp>
#include <raptr/sound/sound.hpp>

namespace {
auto logger = raptr::_get_logger(__FILE__);
std::map<raptr::fs::path, Mix_Chunk*> sound_table;
}

namespace raptr {

void play_chunk(Mix_Chunk* chunk)
{
    Mix_PlayChannel(-1, chunk, 0);
}

void play_sound(const FileInfo& path)
{
    static int32_t current_channel = 0;
    Mix_Chunk* chunk = nullptr;
    const auto found = sound_table.find(path.file_path);
    if (found == sound_table.end()) {
        const auto file_path_str = path.file_path.string();
        const auto wav = file_path_str.c_str();
        const auto sound = Mix_LoadWAV(wav);
        if (!sound) {
            logger->error("Failed to play sound {}", path.file_relative);
            return;
        }
        sound_table[path.file_path] = sound;
        chunk = sound;
    } else {
        chunk = found->second;
    }

    Mix_Volume(current_channel, 10);
    Mix_PlayChannel(current_channel, chunk, 0);

    current_channel = (current_channel + 1) % 64;
}

} // raptr