/*!
  /file dialog.hpp
  Dialog provides the mechanism for interacting with character events.
*/
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <SDL_ttf.h>

#include <raptr/common/filesystem.hpp>
#include <raptr/input/controller.hpp>
#include <raptr/ui/font.hpp>

namespace toml {
class Value;
} // namespace toml

namespace raptr {

class Game;
class Controller;
class Sprite;
class Renderer;

struct DialogCharacter {
  std::string name;
  std::string font_name;
  int32_t font_size;
};

struct DialogPrompt {
  std::shared_ptr<Sprite> speaker;
  std::string name;
  std::string button;
  std::string text;
  std::string trigger;
  std::string key;
  std::string value;
  std::string section;
  int32_t evil_requirement;
  int32_t wholesome_requirement;
  std::vector<DialogPrompt> choices;

  std::shared_ptr<Text> r_name, r_text, r_button, r_button_hover;
  
  struct Has {
    bool trigger;
    bool key;
    bool value;
    bool evil_requirement;
    bool wholesome_requirement;
  } has;
};

/*!
  A Dialog is an overlay that is presented over the screen and allows the
  player to interact with the options presented
*/
class Dialog {
 public:
  /*!
    Attaches a controller to the dialog so the player can interact
    /param controller - An instance of a controller to bind
  */
  void attach_controller(std::shared_ptr<Controller>& controller_);

  /*!
    Parse the Dialog script and store errors
    /return Whether the document was parsed correctly
  */
  static std::shared_ptr<Dialog> from_toml(const FileInfo& path);

  /*!
    Start the dialog
  */
  bool start();

  /*!
    Start processing the dialog
  */
  bool think(std::shared_ptr<Game>& game);

 private:
  /*!
    Method to unwind a dialog file
    /param root - The current toml section to parse
    /param key - The key to find
  */
  bool parse_toml(const toml::Value* root, DialogPrompt* prompt, const std::vector<int32_t>& section);

  /*!
    When a controller is attached to this character, any button presses will be dispatched to this
    function which will handle how best to deal with a button press
    \param state - What the controller was doing exactly when a button was pressed down
  */
  bool on_button_down(const ControllerState& state);

  /*!
    This is a simple joystick interaction to control the Dialog options.
  */
  bool on_right_joy(const ControllerState& state);

 private:
  int64_t last_ticks;
  bool parse_error;
  FileInfo toml_path;
  std::shared_ptr<Controller> controller;
  std::vector<DialogPrompt> prompts;
  std::shared_ptr<Sprite> dialog_box;
  DialogPrompt* active_prompt;
  std::shared_ptr<TTF_Font> font;
  int32_t selected_choice;
};

} // namespace raptr
