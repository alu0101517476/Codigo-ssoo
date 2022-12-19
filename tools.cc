#include "tools.hpp"

std::string basename(const std::string& path) {
  std::array<char, PATH_MAX> buffer;
  path.copy(buffer.data(), buffer.size());
  return std::string(basename(buffer.data()));
}

std::error_code read(int fd, std::vector<uint8_t>& buffer) {
  ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
  if (bytes_read < 0) {
    return std::error_code(errno, std::system_category());
  }
  buffer.resize(bytes_read);
  return std::error_code(0, std::system_category());
}

std::error_code write(int fd, const std::vector<uint8_t>& buffer) {
  ssize_t bytes_read = write(fd, buffer.data(), buffer.size());
  if (bytes_read < 0) {
    return std::error_code(errno, std::system_category());
  }
  return std::error_code(0, std::system_category());
}

// Opción -a
std::error_code copy_file(const std::string& src_path,
                         const std::string& dst_path,
                         bool preserve_all = false) {
  struct stat src_stat, dst_stat;
  // Raise EACCES si no src_path no existe:
  stat(src_path.c_str(), &src_stat); 
  if (errno == ENOENT) { // stat modifica a errno internamente, si errno es igual a ENOENT, que es una variable que dice si existe el archivo no eciste, devolvemos el error
    return std::error_code(errno, std::system_category());
  }
  // Comprobamos si src_path no es un archivo regular (raise EACCES if src_path no es un archivo regular.)
  if (!S_ISREG(src_stat.st_mode)) { // st_mode es un , y S_ISREG() te dice si es un fichero regular o no  
    return std::error_code(errno, std::system_category());
  }

  // Si dst_path existe:
  stat(dst_path.c_str(), &dst_stat); 
  std::string aux_dst_path{dst_path}; // aux 
  if (errno != ENOENT) { // stat modifica a errno internamente, si errno es igual a ENOENT, que es una variable que dice si existe el archivo no eciste, devolvemos el error
    assert(src_stat.st_dev != dst_stat.st_dev || src_stat.st_ino != dst_stat.st_ino);
    // Si dst_path es un directorio:
    if (S_ISDIR(dst_stat.st_mode)) {
      // Guardar en dst_path una nueva ruta con el nombre de archivo de src_path en el directorio dst_path
      aux_dst_path += ('/' + basename(src_path)); 
    }
  }
  // Abrir src_path para lectura
  int fd_src{open(src_path.c_str(), O_RDONLY)}; //
  if (fd_src < 0) {
    return std::error_code(errno, std::system_category());
  }
  auto src_guard = scope::scope_exit([fd_src]{ close(fd_src); });
  // Abrir dst_path para escritura
  int fd_dst{open(aux_dst_path.c_str(), O_WRONLY | O_TRUNC | O_CREAT)}; //
  if (fd_dst < 0) {
    return std::error_code(errno, std::system_category());
  }
  auto dst_guard = scope::scope_exit([fd_dst]{ close(fd_dst); });
  // Mientras haya datos en src_path por leer:
  std::vector<uint8_t> buffer(16ul * 1024 * 1024);
  do {
    read(fd_src, buffer);
    write(fd_dst, buffer);
  } while (buffer.size() > 0);
    // Cerrar src_path
    close(fd_src);
    // Cerrar dst_path
    close(fd_dst);
  
  if (preserve_all) {
    // Copiar los atributos(permisos, propietario, grupo, fecha de ultima modificación, fecha de último acceso) de src_path a dst_path 
    mode_t file_permissions{src_stat.st_mode & ~S_IFMT};
    chmod(aux_dst_path.c_str(), file_permissions); // le pasamos los permisos con chmod a dst_path 
    chown(aux_dst_path.c_str(), src_stat.st_uid, src_stat.st_gid); // le pasamos el propietario y el grupo
    struct utimbuf utime_dst;
    utime_dst.actime = src_stat.st_atime;
    utime_dst.modtime = src_stat.st_mtime;
    utime(aux_dst_path.c_str(), &utime_dst);
  }
  return std::error_code();
}

// Opción -m
std::error_code move_file(const std::string& src_path,
                         const std::string& dst_path) {
  struct stat src_stat, dst_stat;
  // raise EACCES si src_path no existe
  stat(src_path.c_str(), &src_stat);
  std::string aux_dst_path{dst_path};
  // Si dst_path es un directorio
  if (S_ISDIR(dst_stat.st_mode)) {
    // Guardamos en dst_path una nueva ruta con el nombre de archivo de src_path en el directorio dst_path
    aux_dst_path += ('/' + basename(src_path)); 
  }

  // Si src_path y dst_path están en el mismo dispositivo
  if (src_stat.st_dev == dst_stat.st_dev || src_stat.st_ino == dst_stat.st_ino) {
    rename(src_path.c_str(), dst_path.c_str());
  } else {
    // Copiar src_path a dst_path preservando todos los atributos (preserve_all = true)
    copy_file(src_path, dst_path, true);
    // Borrar src_path
    unlink(src_path.c_str());
  }
  return std::error_code();
}

std::error_code print(const std::string& str) {
  int bytes_written = write(STDOUT_FILENO, str.c_str(), str.size());
  if (bytes_written == -1) {
    return std::error_code(errno, std::system_category());
  }
  return std::error_code(0, std::system_category());
}

bool es_comando_interno(const std::vector<std::string>& args) {
  /*
  Comandos internos:
  cd_command() cambia el directorio de trabajo.
  echo_command() muestra un mensaje por la salida estándar.
  cp_command() copia un fichero.
  mv_command() mueve un fichero.
  */
  if (args[0] == "cd" || args[0] == "echo" || args[0] == "cp" || args[0] == "mv") {
    return true;
  }
  return false;
}