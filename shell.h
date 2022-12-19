#ifndef SHELL_H
#define SHELL_H
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
#include "tools.hpp"
#include "scope.h"

class Terminal {
 public:
  Terminal(){

  }
  void start(int fd, std::string& line); // método que empieza con el funcionamiento de la shell
  // shell::command_result execute_commands(const std::vector<shell::command>&
  // commands);

 private:
  void print_prompt();
  void echo_command();
  std::error_code read_line(int fd, std::string& line);
  std::vector<shell::command> parse_line(const std::string& line);
  shell::command_result execute_commands(const std::vector<shell::command>& commands);
  // Comandos internos
  void echo_command(const std::vector<std::string>& args);
  std::error_code cd_command(const std::vector<std::string>& args);
  std::error_code cp_command(const std::vector<std::string>& args);
  std::error_code mv_command(const std::vector<std::string>& args);
  // Métodos para ejecutar comandos internos y externos respectivamente
  int foo_command(const std::vector<std::string>& args);
  int execute_program(const std::vector<std::string>& args, bool has_wait);
};

#endif
