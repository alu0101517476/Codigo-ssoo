#ifndef TOOLS_H
#define TOOLS_H
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <array>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "scope.h"

std::string basename(const std::string& path); 

std::error_code read(int fd, std::vector<uint8_t>& buffer); 

std::error_code write(int fd, const std::vector<uint8_t>& buffer); 

// Opción -a
std::error_code copy_file(const std::string& src_path,
                         const std::string& dst_path,
                         bool preserve_all);
                         
// Opción -m
std::error_code move_file(const std::string& src_path,
                         const std::string& dst_path);

                         namespace shell {
using command = std::vector<std::string>;
}

namespace shell {
  using command = std::vector<std::string>;
}

namespace shell {
  struct command_result {
    int return_value;
    bool is_quit_requested;
    command_result(int return_value, bool request_quit = false) : return_value{return_value}, is_quit_requested{request_quit} {}
    static command_result quit(int return_value = 0) {
      return command_result{return_value, true};
    }
  };
}  // namespace shell

std::error_code print(const std::string& str); 

bool es_comando_interno(const std::vector<std::string>& args);

#endif