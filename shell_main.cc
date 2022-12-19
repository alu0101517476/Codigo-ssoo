#include "shell.hpp"

int main() {
  std::string line;
  Terminal prueba;
  prueba.start(STDIN_FILENO, line);
  return 0;
}
