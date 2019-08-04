#include <raptr/common/logging.hpp>
#include <raptr/game/console.hpp>

namespace {
auto logger = raptr::_get_logger(__FILE__);
};

namespace raptr {

Console::Console()
{
    shutdown = false;
    lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);
    think_thread = std::thread(&Console::think, this);
}

Console::~Console()
{
    shutdown = true;
    think_thread.join();
}

void Console::push(const std::string& command)
{
    std::scoped_lock<std::mutex> lck(mutex);
    commands.push_back(command);
}

void Console::process_commands()
{
    std::scoped_lock<std::mutex> lck(mutex);
    for (const auto& command : commands) {
        try {
            if (command == "!") {
                lua.safe_script(last_command);
            } else {
                lua.safe_script(command);
            }
        } catch (const std::exception& e) {
            logger->error(e.what());
        }
    }

    if (!commands.empty()) {
        std::cout << std::endl
                  << "> ";
    }

    commands.clear();
}

void Console::think()
{
    bool in_block = false;
    std::stringstream buffer;
    std::string line;
    std::cout << "> ";
    while (!shutdown) {

        std::getline(std::cin, line);
        if (line == "BEGIN") {
            in_block = true;
            continue;
        }

        if (line == "END") {
            in_block = false;
            this->push(buffer.str());
            buffer = std::stringstream();
            continue;
        }

        buffer << line;

        if (!in_block) {
            auto cmd = buffer.str();
            if (cmd != "!") {
                last_command = cmd;
            }
            this->push(cmd);
            buffer = std::stringstream();
        }
    }
}

}
