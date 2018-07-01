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

  dialog->last_ticks = SDL_GetTicks();
  return dialog;
}

bool Dialog::load_font(const FileInfo& path)
{
  if (TTF_Init() < 0) {
    logger->error("TTF failed to initialize: {}", SDL_GetError());
    return false;
  }

  std::string full_path = path.file_path.string();

  font.reset(TTF_OpenFont(full_path.c_str(), 15), SDLDeleter());
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
      logger->error("{}: Missing {}", section_name, key);
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
  prompt->section = section_name;
  if (!prompt->speaker) {
    parse_error = true;
    logger->error("{}: Failed to load speaker: {} (tried {})", 
      section_name, speaker_id, sprite_path.file_path);
    return false;
  }

  std::string animation_name = dict["expression"]->as<std::string>();
  if (!prompt->speaker->set_animation(animation_name)) {
    parse_error = true;
    logger->error("{}: Failed to find animation for expression {} in {}", 
      section_name, animation_name, speaker_id);
    return false;
  }

  prompt->name = dict["name"]->as<std::string>();

  prompt->text = dict["text"]->as<std::string>();

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
  
  prompt->button = "";
  if (dict.find("button") != dict.end()) {
    std::string prefix;
    if (prompt->has.evil_requirement) {
      prefix = "[EVIL]";
    } else if (prompt->has.wholesome_requirement) {
      prefix = "[WHOLESOME]";
    } else {
      prefix = "[" + animation_name + "]";
    }
    prompt->button = prefix + " " + dict["button"]->as<std::string>();
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
  SDL_Color hover_color = {0, 255, 0, 255};
  SDL_Color bg_color = {0, 0, 0, 255};

  prompt->r_text.surface.reset(
    TTF_RenderText_Solid(font.get(), prompt->text.c_str(), text_color), SDLDeleter());
  if (!prompt->r_text.surface) {
    logger->error("{}: Failed to create TTF for text '{}'", section_name, prompt->text);
    parse_error = true;
    return false;
  }

  prompt->r_name.surface.reset(
    TTF_RenderText_Solid(font.get(), prompt->name.c_str(), text_color), SDLDeleter());
  if (!prompt->r_name.surface) {
    logger->error("{}: Failed to create TTF for text '{}'", section_name, prompt->name);
    parse_error = true;
    return false;
  }

  if (!prompt->button.empty()) {
    prompt->r_button.surface.reset(
      TTF_RenderText_Shaded(font.get(), prompt->button.c_str(), text_color, bg_color), SDLDeleter());
    if (!prompt->r_button.surface) {
      parse_error = true;
      logger->error("{}: Failed to create TTF for button '{}'", section_name, prompt->button);
      return false;
    }
    prompt->r_button_hover.surface.reset(
      TTF_RenderText_Solid(font.get(), prompt->button.c_str(), hover_color), SDLDeleter());
  }

  int32_t check = 0;
  while (++check) {
    auto next_section(section);
    next_section.push_back(check);
    DialogPrompt next_prompt;

    if (!found->find(std::to_string(check))) {
      if (prompt->choices.size() > 1) {
        for (auto& choice : prompt->choices) {
          if (choice.button.empty()) {
            parse_error = true;
            logger->error("{}: Missing a 'button' tag. It is part of a group of responses.", 
                          choice.section);
            return false;
          }
        }
      }
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
  controller->on_right_joy(std::bind(&Dialog::on_right_joy, this, _1));
}

bool Dialog::on_button_down(const ControllerState& state)
{
  if (state.button != Button::b) {
    return false;
  }

  if (!active_prompt->choices.empty()) {
    active_prompt = &active_prompt->choices[selected_choice];
    selected_choice = 0;
  }
}

bool Dialog::on_right_joy(const ControllerState& state)
{
  if (!active_prompt) {
    return false;
  }

  if ((SDL_GetTicks() - last_ticks) < 250) {
    return false;
  }
  
  if (state.y < -0.5) {
    ++selected_choice;
    if (selected_choice >= active_prompt->choices.size()) {
      selected_choice = 0;
    }
    last_ticks = SDL_GetTicks();
  } else if (state.y > 0.5) {
    --selected_choice;
    if (selected_choice < 0) {
      selected_choice = active_prompt->choices.size() - 1;
    }
    last_ticks = SDL_GetTicks();
  }

  return true;
}


bool DialogPrompt::Text::allocate(std::shared_ptr<Renderer>& renderer)
{
  if (texture) {
    return false;
  }

  texture.reset(renderer->create_texture(surface), SDLDeleter());
  if (!texture) {
    logger->error("Failed to allocate texture");
    return false;
  }

  SDL_QueryTexture(texture.get(), NULL, NULL, &bbox.w, &bbox.h);
  bbox.x = 0;
  bbox.y = 0;

  return true;  
}

bool Dialog::think(std::shared_ptr<Game>& game)
{
  if (!active_prompt) {
    selected_choice = 0;
    active_prompt = &prompts[0];
  }

  // Render dialog box
  auto& renderer = game->renderer; 
  dialog_box->render(renderer);

  // Render speaker
  auto& speaker = active_prompt->speaker;
  speaker->render(renderer);

  // Start rendering frame info
  auto& current_frame = speaker->current_animation->current_frame();

  // Text of the Dialog box
  {
    SDL_Rect dst;
    auto& text = active_prompt->r_text;
    text.allocate(renderer);
    auto& texture = text.texture;
    auto& bbox = text.bbox;
    dst.w = bbox.w;
    dst.h = bbox.h;
    dst.x = speaker->x + current_frame.w + 16;
    dst.y = 32;
    renderer->add(texture, bbox, dst, 0.0, false, false);
  }

  // Name of the Character
  {
    SDL_Rect dst;
    auto& text = active_prompt->r_name;
    text.allocate(renderer);
    auto& texture = text.texture;
    auto& bbox = text.bbox;
    dst.w = bbox.w;
    dst.h = bbox.h;
    dst.x = 32;
    dst.y = 8;
    renderer->add(texture, bbox, dst, 0.0, false, false);
  }

  // Available choices
  if (active_prompt->choices.size() > 1) {
    int32_t choice_x = 40;
    int32_t choice_y = 100;
    for (int32_t i = 0; i < active_prompt->choices.size(); ++i) {
      auto& choice = active_prompt->choices[i];
      SDL_Rect dst;
      DialogPrompt::Text& text = (i == selected_choice) ? choice.r_button_hover : choice.r_button;
      text.allocate(renderer);
      auto& texture = text.texture;
      auto& bbox = text.bbox;
      dst.w = bbox.w;
      dst.h = bbox.h;
      dst.x = choice_x;
      dst.y = choice_y;
      renderer->add(texture, bbox, dst, 0.0, false, false);
      choice_y += 24;
    }
  }

  return true;
}

} // namespace raptr
