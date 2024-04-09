#include <dlfcn.h>
#include <err.h>
#include <limits.h>
#include <unistd.h>
#include <link.h>
#include <err.h>

#include <Kokkos_Core.hpp>
#include <iostream>
#include <string>

std::string execPath;
std::array<char *, 64> execArgv;
std::array<char *, 64> execEnvp;
std::string libName = "libkokkosautothreads.so";

std::string GetLibFullPath() {
  std::string fullpath;
  struct link_map *linkMap;
  void *handle;
  int ret;
  
  handle = dlopen(libName.c_str(), RTLD_LAZY);
  if (handle == NULL) {
    err(EXIT_FAILURE, "Cannot dlopen library %s", libName.c_str());
  }

  ret = dlinfo(handle, RTLD_DI_LINKMAP, &linkMap);
  if (ret < 0) {
    err(EXIT_FAILURE, "Cannot dlinfo library %s", libName.c_str());
  }
  return linkMap->l_name;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    errx(EXIT_FAILURE, "Expected executable path at argv[1]");
  }

  memcpy(&execArgv, &argv[1], sizeof(char *) * (argc - 1));
  execPath = argv[1];

  std::cout << "full lib path is " << GetLibFullPath() << std::endl;

  execvpe(execPath.c_str(), execArgv.data(), environ);

  return 0;
}
