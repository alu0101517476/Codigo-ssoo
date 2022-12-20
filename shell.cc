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
  std::istringstream iss(line);         // creamos un istringstream para coger comando por comando de la cadena.
  std::vector<shell::command> linea_de_comandos; // creamos el vector de vectores "comandos" que contendrá todos los comandos.
  shell::command comando_actual;        // creamos un vector de strings para ver el comando actual.

  while (!iss.eof()) {
      std::string comando; // necesitamos una string para almacenar comando por comando de la cadena total de comandos
                        // pasados por parámetros.
      iss >> comando;      // recogemos el comando de "line".
      char caracter_especial = comando[comando.size() - 1]; // asignamos una variable como un caracter especial

      if (caracter_especial == ';' || caracter_especial == '&' || caracter_especial == '|') {
          comando.erase(comando.size() - 1);        // Eliminamos el carácter especial
          comando_actual.push_back(comando);     // Añadimos la palabra al vector de comandos actuales
          linea_de_comandos.push_back(comando_actual); // Añadimos el comando actual a la linea de comandos
          comando_actual.clear();             // Vaciamos el comando contenido en el comando actual.
      } else if (comando[0] == '#') {
          break;                          // Si la línea empieza con un '#', omitimos esa línea.
      } else {
          comando_actual.push_back(comando); // Si no hemos llegado al final del comando ni es una línea comentada,
                                          // entendemos que no ha llegado su fin. Añadimos al comando actual la extensión.
      }
  }

  if (!comando_actual.empty()) {
      linea_de_comandos.push_back(comando_actual); // En el caso de no tener un fin de comando (';', '|', '&'),
                                                  // añadimos a comandos lo que tenemos en el comando actual.
  }
  return linea_de_comandos;
}


// Función para comandos internos
int Terminal::menu_comandos(const std::vector<std::string>& args) {
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
int Terminal::echo_command(const std::vector<std::string>& args) {
  for (int i{1}; i < args.size(); ++i) {
    std::cout << args[i] << " ";
  }
  std::cout << std::endl;
  return 0;
}

int Terminal::cp_command(const std::vector<std::string>& args) {
  if (args.size() == 4) {
    copy_file(args[2], args[3], true);
  } else {
    copy_file(args[1], args[2], false);
  }
  return 0;
}

int Terminal::mv_command(const std::vector<std::string>& args) {
  move_file(args[1], args[2]);
  return 0;
}

int Terminal::cd_command(const std::vector<std::string>& args) {
  if (args.size() > 2) {
    std::cerr << "Error: cd: Demasiados argumentos" << std::endl;
    return 1;
  } else {
    int error{chdir(args[1].c_str())};
    if (error == -1) {
      std::cerr << "Error: cd: " << args[1] <<": No es un directorio" << std::endl;
      return 1;
    } else {
      chdir(args[1].c_str());
    }
  }
  return 0;
}

shell::command_result Terminal::execute_commands(const std::vector<shell::command>& commands) {
  for(auto command : commands) {
    if (command.at(0) == "exit") {
      exit(1);
    }
    if (es_comando_interno(command)) {
      menu_comandos(command);
    } else {
      execute_program(command);
    }
  }
  shell::command_result::quit(0);
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
