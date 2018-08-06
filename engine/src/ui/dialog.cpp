#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <functional>

#include <raptr/game/game.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/renderer/sprite.hpp>
#include <raptr/renderer/renderer.hpp>
#include <raptr/common/logging.hpp>
#include <raptr/ui/dialog.hpp>

namespace
{
auto logger = raptr::_get_logger(__FILE__);
};

#pragma warning(disable : 4996)
#include <toml/toml.h>
#pragma warning(default : 4996)

namespace raptr
{
namespace
{
std::map<std::string, DialogCharacter> CHARACTER_DIALOG_CACHE;
bool dialog_cache_loaded = false;
}

bool load_dialog_cache(const FileInfo& game_root)
{
  if (dialog_cache_loaded) {
    return true;
  }

  auto toml_path = game_root.from_root("dialog/dialog.toml");
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
  const toml::Array& registry_values = v.find("dialog")->as<toml::Array>();
  for (const toml::Value& v : registry_values) {
    std::string name = v.get<std::string>("character");
    std::string font_name = v.get<std::string>("font");
    int32_t font_size = v.get<int32_t>("size");
    DialogCharacter dc = {name, font_name, font_size};
    CHARACTER_DIALOG_CACHE[name] = dc;
    logger->debug("Registered {} dialog defaults", name);
  }

  dialog_cache_loaded = true;
  return true;
}

std::shared_ptr<Dialog> Dialog::from_easy_params(
      const FileInfo& game_root,
      const std::string& speaker, const std::string& expression,
      const std::string& name, const std::string& text)
{
  std::stringstream is;
  is << "[1]\n"
     << "speaker = \"" << speaker << "\"\n"
     << "expression = \"" << expression << "\"\n"
     << "name = \"" << name << "\"\n"
     << "text = \"" << text << "\"\n";

  toml::ParseResult pr = toml::parse(is);
  return Dialog::from_toml(game_root, pr);
}

std::shared_ptr<Dialog> Dialog::from_toml(const FileInfo& toml_path)
{
  load_dialog_cache(toml_path);
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

  return Dialog::from_toml(toml_path, pr);
}

std::shared_ptr<Dialog> Dialog::from_toml(const FileInfo& toml_path, toml::ParseResult& pr)
{
  auto toml_relative = toml_path.file_relative;
  const toml::Value& v = pr.value;

  auto dialog = std::make_shared<Dialog>();
  dialog->toml_path = toml_path;
  dialog->parse_error = false;
  dialog->active_prompt = nullptr;
  dialog->dialog_box = Sprite::from_json(toml_path.from_root("textures/dialog-simple.json"));
  dialog->dialog_box->set_animation("Idle");
  dialog->dialog_box->x = 0;
  dialog->dialog_box->y = 0;
  dialog->dialog_box->absolute_positioning = true;

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

  dialog->last_ticks = clock::ticks();
  return dialog;
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
    "button",
    "font_name",
    "font_size"
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

  if (!prompt->speaker) {
    parse_error = true;
    logger->error("{}: Failed to load speaker: {} (tried {})",
                  section_name, speaker_id, sprite_path.file_path);
    return false;
  }

  prompt->speaker->scale = 1.5;
  prompt->speaker->x = 15;
  prompt->speaker->y = GAME_HEIGHT - 25;
  prompt->speaker->flip_x = true;
  prompt->section = section_name;
  prompt->speaker->absolute_positioning = true;

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

  auto char_map = CHARACTER_DIALOG_CACHE.find(speaker_id);
  bool has_defaults = char_map != CHARACTER_DIALOG_CACHE.end();
  DialogCharacter char_defaults;
  if (has_defaults) {
    char_defaults = char_map->second;
  } else {
    char_defaults.font_name = "default";
    char_defaults.font_size = 15;
  }

  int32_t font_size = char_defaults.font_size;
  if (dict.find("font_size") != dict.end()) {
    font_size = dict["font_size"]->as<int32_t>();
  }

  std::string font_name = char_defaults.font_name;
  if (dict.find("font_name") != dict.end()) {
    font_name = dict["font_name"]->as<std::string>();
  }

  SDL_Color text_color = {255, 255, 255, 255};
  SDL_Color hover_color = {0, 255, 0, 255};
  SDL_Color bg_color = {0, 0, 0, 255};
  int32_t max_width = 400;

  auto game_root = toml_path.from_root("");
  prompt->r_text = Text::create(game_root, font_name,
                                prompt->text, font_size, text_color,
                                max_width);

  if (!prompt->r_text) {
    logger->error("{}: Failed to create TTF for text '{}'", section_name, prompt->text);
    parse_error = true;
    return false;
  }

  prompt->r_name = Text::create(game_root, font_name,
                                prompt->name, font_size, text_color,
                                max_width);

  if (!prompt->r_name) {
    logger->error("{}: Failed to create TTF for text '{}'", section_name, prompt->name);
    parse_error = true;
    return false;
  }

  if (!prompt->button.empty()) {
    prompt->r_button = Text::create(game_root, "default", prompt->button, 15, text_color);
    if (!prompt->r_button) {
      parse_error = true;
      logger->error("{}: Failed to create TTF for button '{}'", section_name, prompt->button);
      return false;
    }
    prompt->r_button_hover = Text::create(game_root, "default", prompt->button, 15, hover_color);
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
  controller->on_button_down(std::bind(&Dialog::on_button_down, this, _1), -1);
  controller->on_right_joy(std::bind(&Dialog::on_right_joy, this, _1), -1);
  controller->on_left_joy(std::bind(&Dialog::on_right_joy, this, _1), -1);
}

bool Dialog::on_button_down(const ControllerState& state)
{
  if (!active_prompt) {
    return true;
  }

  if (state.button != Button::b) {
    return false;
  }

  if (!active_prompt->choices.empty()) {
    active_prompt = &active_prompt->choices[selected_choice];
    selected_choice = 0;
  } else {
    active_prompt = nullptr;
  }

  return false;
}

bool Dialog::on_right_joy(const ControllerState& state)
{
  if (!active_prompt) {
    return true;
  }

  if ((clock::ticks() - last_ticks) / 1e3 < 250) {
    return false;
  }

  if (state.y < -0.5) {
    ++selected_choice;
    if (selected_choice >= active_prompt->choices.size()) {
      selected_choice = 0;
    }
    last_ticks = clock::ticks();
  } else if (state.y > 0.5) {
    --selected_choice;
    if (selected_choice < 0) {
      selected_choice = static_cast<int32_t>(active_prompt->choices.size() - 1);
    }
    last_ticks = clock::ticks();
  }

  return false;
}

bool Dialog::start()
{
  if (prompts.empty()) {
    logger->error("There are no prompts to start");
    return false;
  }

  selected_choice = 0;
  active_prompt = &prompts[0];
  return true;
}

bool Dialog::render(Renderer* renderer) const
{
  if (!active_prompt) {
    return false;
  }

  // Render dialog box
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
    text->allocate(*renderer);
    auto& texture = text->texture;
    auto& bbox = text->bbox;
    dst.w = bbox.w;
    dst.h = bbox.h;
    dst.x = static_cast<int32_t>(speaker->x + current_frame.w + 16);
    dst.y = GAME_HEIGHT - 32;
    renderer->add_texture(texture, bbox, dst, 0.0, false, false, true);
  }

  // Name of the Character
  {
    SDL_Rect dst;
    auto& text = active_prompt->r_name;
    text->allocate(*renderer);
    auto& texture = text->texture;
    auto& bbox = text->bbox;
    dst.w = bbox.w;
    dst.h = bbox.h;
    dst.x = 32;
    dst.y = 8;
    renderer->add_texture(texture, bbox, dst, 0.0, false, false, true);
  }

  // Available choices
  if (active_prompt->choices.size() > 1) {
    int32_t choice_x = 40;
    int32_t choice_y = GAME_HEIGHT - 200;
    for (int32_t i = 0; i < active_prompt->choices.size(); ++i) {
      auto& choice = active_prompt->choices[i];
      SDL_Rect dst;
      auto& text = i == selected_choice ? choice.r_button_hover : choice.r_button;
      text->allocate(*renderer);
      auto& texture = text->texture;
      auto& bbox = text->bbox;
      dst.w = bbox.w;
      dst.h = bbox.h;
      dst.x = choice_x;
      dst.y = choice_y;
      renderer->add_texture(texture, bbox, dst, 0.0, false, false, true);
      choice_y += 24;
    }
  }

  return true;
}
} // namespace raptr
