#include <dlfcn.h>
#include <err.h>
#include <limits.h>
#include <link.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <Kokkos_Core.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "json.hpp"

std::string execPath;
std::array<char *, 64> execArgv;
std::array<char *, 64> execEnvp;
char execLibArg[512];
int execArgc;
nlohmann::json rootJson;
std::ofstream outputFile;
std::ifstream localOutput;
nlohmann::json localJson;
std::string localOutputFilename = "kokkosautothreads.local.json";
std::string outputFilename = "kokkosautothreads.json";
std::string libName = "libkokkosautothreads.so";

std::string GetLibFullPath() {
  std::string fullpath;
  struct link_map *linkMap;
  void *handle;
  int ret;

  handle = dlopen(libName.c_str(), RTLD_LAZY);
  if (handle == NULL) {
    errx(EXIT_FAILURE, "Cannot dlopen library %s", libName.c_str());
  }

  ret = dlinfo(handle, RTLD_DI_LINKMAP, &linkMap);
  if (ret < 0) {
    errx(EXIT_FAILURE, "Cannot dlinfo library %s", libName.c_str());
  }
  return linkMap->l_name;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    errx(EXIT_FAILURE, "Expected executable path at argv[1]");
  }

  execPath = argv[1];
  execArgc = argc - 1;
  memcpy(&execArgv, &argv[1], sizeof(char *) * execArgc);
  std::snprintf(execLibArg, sizeof(execLibArg), "--kokkos-tools-libs=%s",
                GetLibFullPath().c_str());
  execArgv[execArgc++] = (char *)&execLibArg;

  for (int nThreads = 0; nThreads < 16; nThreads++) {
    nlohmann::json perThreadJson;
    for (int run = 0; run < 10; run++) {
      if (fork() == 0) {
        int ret = execvpe(execPath.c_str(), execArgv.data(), environ);
        if (ret < 0) err(EXIT_FAILURE, "Cannot execute %s", execPath.c_str());
      } else {
        int wstatus;
        wait(&wstatus);
        if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_FAILURE)
          exit(EXIT_FAILURE);
        localOutput.open(localOutputFilename);
        localJson = nlohmann::json::parse(localOutput);
        localOutput.close();
        perThreadJson.push_back({{"run_id", run}, {"run_log", localJson}});
      }
    }
    rootJson.push_back({{"num_threads", nThreads}, {"run_log", perThreadJson}});
  }

  outputFile.open(outputFilename);
  outputFile << rootJson << std::endl;

  return 0;
}
