#include "shell.hpp"

void Terminal::print_prompt() {
  print("$>");
}

std::error_code Terminal::read_line(int fd, std::string& line) {
  //PARSE_LINE VACIO Y READ_LINE VACÍOS????

  // Inciar pending_input como un 'vector' vacío
  std::string pending_input{line};
  // creamos el buffer
  std::vector<uint8_t> buffer(16ul * 1024 * 1024);
  while (true) {
    line.clear();
    // Buscar en pending_input el primer salto de línea
    size_t posicion_endl{pending_input.find('\n')};
    // Si se ha encontrado el salto de línea:
    if (posicion_endl != std::string::npos) { // npos es una constante de string que dice si lo ha encontrado o no
      //Sustituir el contenido de line con el contenido de pending_input desde el principio hasta incluir el salto de línea
      line = pending_input.substr(0, posicion_endl + 1);
      execute_commands(parse_line(line)); // LLAMAMOS A PARSE_LINE
      // eliminar del vector pending_input el contenido copiado en line
      pending_input = pending_input.substr(posicion_endl + 1, pending_input.size() - (posicion_endl + 1)); // pending_input.size() - (posicion_endl + 1) es la longitud de la cadena con la que nos quedamos
      return std::error_code();
    }
    ssize_t bytes{read(fd, buffer.data(), buffer.size())};
    // si bytes es igual a -1 es que no ha leido nada y por tanto da error
    if (bytes == -1) {
      return std::error_code(errno, std::system_category());
    }
    // si el buffer está vacío
    if (bytes == 0) {
      if (!pending_input.empty()) {
        // añadir a line el contenido de pendin_input + el salto de línea
        line = pending_input + '\n';
        // vaciar pending_input
        pending_input.clear();
      }
      return std::error_code();
    } else {
      // Añadir el contenido del buffer al final de pending_input
      for (int i = 0; i < buffer.size(); ++i) {
        pending_input += buffer[i];
      }
    }
  }
  return std::error_code();
}

std::vector<shell::command> Terminal::parse_line(const std::string& line) {
  std::vector<std::vector<std::string>> linea_de_comandos;
  std::istringstream iss(line);
  bool es_comando_puro{true}; // aux que nos sirve para saber si el string comando es un comando o un argumento, inicializado a true por que la primera palabra siempre sera un comando
  int posicion{0}; // aux que nos ayuda para poder acceder a los comandos y agregar los argumentos siempre que sea necesario
  while(!iss.eof()) {
    std::string comando, aux{""};
    iss >> comando; // Usar o almacenar la palabra word
    std::vector<std::string> comando_y_argumentos; // vector<string> -> comando con sus argumentos
    char caracter_especial{comando[comando.size() - 1]};

    if (es_comando_puro) {
      comando_y_argumentos.push_back(comando); // metemos el comando
      linea_de_comandos.push_back(comando_y_argumentos); // metemos el comando en el vector de la linea de comandos
      es_comando_puro = false; // ya que después de un comando siempre viene su argumento, marcamos que la siguiente palabra sera un argumento
    } else if (comando[0] == '#') { // Si la palabra empieza por '#', significa que el resto de la línea es un comentario, se sale retornando el vector de comandos leídos hasta el momento
      return linea_de_comandos; 
    } else { // es un argumento del comando
        if (caracter_especial == ';' || caracter_especial == '&' || caracter_especial == '|' ) {
          es_comando_puro = true; // despues de un caracter especial siempre vendra un comando, asi que lo marcamos para que lo siguiente que entre en el vector sea un comando
          comando.erase(comando.size() - 1); // le borramos el caracter especial a el comando
          linea_de_comandos[posicion].push_back(comando);
          posicion++; // lo que va a venir despues es un nuevo comando, asi que para poder acceder a el aumentamos la posicion
        } else { // puede ser que nos ingresen varios argumentos para un mismo comando separados por espacio, lo metemos en el vector de comandos
          es_comando_puro = true; // despues de un caracter especial siempre vendra un comando, asi que lo marcamos para que lo siguiente que entre en el vector sea un comando
          linea_de_comandos[posicion].push_back(comando);
        }
    }
  }
  return linea_de_comandos;
}

// Función para comandos internos
int Terminal::foo_command(const std::vector<std::string>& args) {
  /*
  Comandos internos:
  cd_command() cambia el directorio de trabajo.
  echo_command() muestra un mensaje por la salida estándar.
  cp_command() copia un fichero.
  mv_command() mueve un fichero.
  */
  if (args[0] == "cd") {
    cd_command(args);
  } else if (args[0] == "echo") {
    echo_command(args);
  } else if (args[0] == "cp") {
    cp_command(args);
  } else {
    mv_command(args);
  } 
  return 1;
}

// Función para comandos externos
int Terminal::execute_program(const std::vector<std::string>& args, bool has_wait=true) {
  return 0;
}

// Implementación de comandos iternos:
void Terminal::echo_command(const std::vector<std::string>& args) {
  for (int i{1}; i < args.size(); ++i) {
    std::cout << args[i] << " ";
  }
  std::cout << std::endl;
}

std::error_code Terminal::cp_command(const std::vector<std::string>& args) {
  if (args.size() == 4) {
    copy_file(args[2], args[3], true);
  } else {
    copy_file(args[1], args[2], false);
  }
  std::error_code();
}

std::error_code Terminal::mv_command(const std::vector<std::string>& args) {
  move_file(args[1], args[2]);
  std::error_code();
}

std::error_code Terminal::cd_command(const std::vector<std::string>& args) {
  if (args.size() > 2) {
    std::cerr << "Error: cd: Demasiados argumentos" << std::endl;
    return std::error_code(errno, std::system_category());
  } else {
    int error{chdir(args[1].c_str())};
    if (error == -1) {
      std::cerr << "Error: cd: " << args[1] <<": No es un directorio" << std::endl;
      return std::error_code(errno, std::system_category());
    } else {
      chdir(args[1].c_str());
    }
  }
  return std::error_code();
}

shell::command_result Terminal::execute_commands(const std::vector<shell::command>& commands) {
  for(auto command : commands) {
    if (command.at(0) == "exit") {
      return shell::command_result::quit(0);
    }
    if (es_comando_interno(command)) {
      foo_command(command);
    } else {
      execute_program(command);
    }
  }
}

// Método para empezar a ejecutar el funcionamiento de la shell
void Terminal::start(int fd, std::string& line) {
  fd = STDIN_FILENO; // descriptor de archivo para la salida estándar
  while (true) { // bucle principal
    print_prompt();
    getline(std::cin, line);
    line.push_back('\n'); // metemos el salto de línea para diferenciar si el usuario escribe un comando vacío
    read_line(fd, line);
  }
}
