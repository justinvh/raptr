/*!
  /file console.hpp
  A simple utility for processing and dispatching commands. The console is lua based
 */
#pragma once

#include <iostream>
#include <mutex>
#include <sol/sol.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace raptr {

class Console {
public:
    Console();
    ~Console();

    template <class T>
    void add(const std::string& command, const T& func)
    {
        lua.set_function(command, func);
    }

    void push(const std::string& command);
    void process_commands();
    void think();
    void show_options(const std::string& options);

public:
    bool shutdown;
    sol::state lua;
    std::thread think_thread;
    std::mutex mutex;
    std::vector<std::string> commands;
    std::string last_command;
};

}
