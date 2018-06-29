#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <functional>

#include <SDL_image.h>

#include <raptr/game/game.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/game/dialog.hpp>
#include <raptr/common/logging.hpp>

macro_enable_logger();

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)


namespace raptr {

std::shared_ptr<Dialog> Dialog::from_toml(const FileInfo& toml_path)
{
  auto toml_relative = toml_path.file_relative;
  auto ifs = toml_path.open();
  if (!ifs) {
    return nullptr;
  }

  toml::ParseResult pr = toml::parse(*ifs);

  if (!pr.valid()) {
    logger->error("Failed to parse {} with reason {}", toml_relative, pr.errorReason);
    return nullptr;
  }

  const toml::Value& v = pr.value;

  std::shared_ptr<Dialog> dialog(new Dialog());
  dialog->toml_path = toml_path;
  dialog->parse_error = false;
  dialog->active_prompt = nullptr;
  dialog->dialog_box = Sprite::from_json(toml_path.from_root("textures/dialog-simple.json"));
  dialog->dialog_box->set_animation("Idle");
  dialog->dialog_box->x = 0;
  dialog->dialog_box->y = 0;

  if (!dialog->load_font(toml_path.from_root("fonts/munro_small.ttf"))) {
    logger->error("Fonts failed to load!");
    return nullptr;
  }

  int32_t check = 0;
  while (++check) {
    if (!v.find(std::to_string(check))) {
      break;
    }

    DialogPrompt prompt;
    if (!dialog->parse_toml(&v, &prompt, {check})) {
      break;
    }
    dialog->prompts.push_back(prompt);
  }

  if (dialog->prompts.empty()) {
    logger->error("No prompts could be found in {}", toml_relative);
    return nullptr;
  }

  if (dialog->parse_error) {
    logger->error("{} failed to parse!", toml_relative);
    return nullptr;
  }

  return dialog;
}

bool Dialog::load_font(const FileInfo& path)
{
  if (TTF_Init() < 0) {
    logger->error("TTF failed to initialize: {}", SDL_GetError());
    return false;
  }

  std::string full_path = path.file_path.string();

  font.reset(TTF_OpenFont(full_path.c_str(), 14), SDLDeleter());
  if (!font) {
    logger->error("TTF failed to load font {}: {}", full_path, SDL_GetError());
    return false;
  }

  return true;
}

bool Dialog::parse_toml(const toml::Value* v, DialogPrompt* prompt, const std::vector<int32_t>& section)
{
  // Build the section string
  std::stringstream ss;
  ss << section[0];
  for (size_t i = 1; i < section.size(); ++i) {
    ss << "." << section[i];
  }

  std::string section_name = ss.str();

  const toml::Value* found = v->find(section_name);
  if (!found) {
    logger->error("Expected {}, but it was not found!", section_name);
    parse_error = true;
    return false;
  }

  std::string required_keys [] = {
    "speaker",
    "expression",
    "name",
    "text"
  };

  std::string optional_keys [] = {
    "evil_requirement",
    "wholesome_requirement",
    "trigger",
    "key",
    "value",
    "button"
  };

  std::map<std::string, const toml::Value*> dict;

  for (const auto& key : required_keys) {
    const toml::Value* value = found->find(key);
    if (!value) {
      logger->error("{} is missing {}", section_name, key);
      parse_error = true;
      return false;
    }
    dict[key] = value;
  }

  for (const auto& key : optional_keys) {
    const toml::Value* value = found->find(key);
    if (!value) {
      continue;
    }
    dict[key] = value;
  }

  std::string speaker_id = dict["speaker"]->as<std::string>();
  FileInfo sprite_path = toml_path.from_root(fs::path("textures") / (speaker_id + ".json"));
  prompt->speaker = Sprite::from_json(sprite_path);
  prompt->speaker->scale = 1.5;
  prompt->speaker->x = 15;
  prompt->speaker->y = 25;
  prompt->speaker->flip_x = true;
  if (!prompt->speaker) {
    parse_error = true;
    logger->error("Failed to load speaker: {} (tried {})", speaker_id, sprite_path.file_path);
    return false;
  }

  std::string animation_name = dict["expression"]->as<std::string>();
  if (!prompt->speaker->set_animation(animation_name)) {
    parse_error = true;
    logger->error("Failed to find animation for expression {} in {}", animation_name, speaker_id);
    return false;
  }

  prompt->name = dict["name"]->as<std::string>();

  prompt->text = dict["text"]->as<std::string>();

  prompt->button = "Ok.";
  if (dict.find("button") != dict.end()) {
    prompt->button = dict["button"]->as<std::string>();
  }

  prompt->has.evil_requirement = false;
  if (dict.find("evil_requirement") != dict.end()) {
    prompt->evil_requirement = dict["evil_requirement"]->as<int32_t>();
    prompt->has.evil_requirement = true;
  }

  prompt->has.wholesome_requirement = false;
  if (dict.find("wholesome_requirement") != dict.end()) {
    prompt->wholesome_requirement = dict["wholesome_requirement"]->as<int32_t>();
    prompt->has.wholesome_requirement = true;
  }

  prompt->has.trigger = false;
  if (dict.find("trigger") != dict.end()) {
    prompt->trigger = dict["trigger"]->as<std::string>();
    prompt->has.trigger = true;
  }

  prompt->has.key = false;
  if (dict.find("key") != dict.end()) {
    prompt->key = dict["key"]->as<std::string>();
    prompt->has.key = true;
  }

  prompt->has.value = false;
  if (dict.find("value") != dict.end()) {
    prompt->value = dict["value"]->as<std::string>();
    prompt->has.value = true;
  }

  SDL_Color text_color = {255, 255, 255, 255};
  prompt->text_surface.reset(TTF_RenderText_Solid(font.get(), prompt->text.c_str(), text_color), SDLDeleter());
  prompt->name_surface.reset(TTF_RenderText_Solid(font.get(), prompt->name.c_str(), text_color), SDLDeleter());

  int32_t check = 0;
  while (++check) {
    auto next_section(section);
    next_section.push_back(check);
    DialogPrompt next_prompt;

    if (!found->find(std::to_string(check))) {
      break;
    }

    if (!this->parse_toml(v, &next_prompt, next_section)) {
      break;
    }

    prompt->choices.push_back(next_prompt);
  }

  return true;
}


void Dialog::attach_controller(std::shared_ptr<Controller>& controller_)
{
  using std::placeholders::_1;
  controller = controller_;

  controller = controller_;
  controller->on_button_down(std::bind(&Dialog::on_button_down, this, _1));
}

bool Dialog::on_button_down(const ControllerState& state)
{
  if (state.button != Button::b) {
    return false;
  }

  if (!active_prompt->choices.empty()) {
    active_prompt = &active_prompt->choices[0];
  }
}


bool Dialog::think(std::shared_ptr<Game>& game)
{
  if (!active_prompt) {
    active_prompt = &prompts[0];
  }

  auto& renderer = game->renderer;

  if (!active_prompt->text_texture) {
    active_prompt->text_texture.reset(renderer->create_texture(active_prompt->text_surface), SDLDeleter());
    SDL_QueryTexture(active_prompt->text_texture.get(), NULL, NULL, &active_prompt->text_bbox.w, &active_prompt->text_bbox.h);
    active_prompt->text_bbox.x = 0;
    active_prompt->text_bbox.y = 0;
  }

  if (!active_prompt->name_texture) {
    active_prompt->name_texture.reset(renderer->create_texture(active_prompt->name_surface), SDLDeleter());
    SDL_QueryTexture(active_prompt->name_texture.get(), NULL, NULL, &active_prompt->title_bbox.w, &active_prompt->title_bbox.h);
    active_prompt->title_bbox.x = 0;
    active_prompt->title_bbox.y = 0;
  }
  
  dialog_box->render(renderer);

  auto& speaker = active_prompt->speaker;
  speaker->render(renderer);

  auto& current_frame = speaker->current_animation->current_frame();

  SDL_Rect dst;
  dst.w = active_prompt->text_bbox.w;
  dst.h = active_prompt->text_bbox.h;
  dst.x = speaker->x + current_frame.w + 16;
  dst.y = 32;
  renderer->add(active_prompt->text_texture, active_prompt->text_bbox, dst, 0.0, false, false);

  dst.w = active_prompt->title_bbox.w;
  dst.h = active_prompt->title_bbox.h;
  dst.x = 32;
  dst.y = 8;
  renderer->add(active_prompt->name_texture, active_prompt->title_bbox, dst, 0.0, false, false);

  return true;
}

} // namespace raptr
