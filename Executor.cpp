#include <dlfcn.h>
#include <err.h>
#include <link.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>

#include "KokkosAutoThreads.hpp"

using Exec = KokkosAutoThreads::Executor;
using json = nlohmann::json;

Exec::Executor(int argc, char *argv[]) {
  this->execPath = argv[1];
  this->argc = argc;
  this->nproc = sysconf(_SC_NPROCESSORS_ONLN);
  std::memcpy(this->argv, argv, argc * sizeof(char *));

  // Point an exec arg to --kokkos-num-threads in execArgs
  auto threadArg = execArgs[execArgsType::NUM_THREADS];
  this->argv[argc] = threadArg;

  // Point an exec arg to --kokkos-tools-libs in execArgs
  auto libArg = execArgs[execArgsType::TOOLS_LIBS];
  this->argv[argc - 1] = libArg;

  // Set --kokkos-tools-libs argument
  this->libPath = GetFullLibPath();
  snprintf(libArg, EXEC_ARG_LEN, "--kokkos-tools-libs=%s", libPath.c_str());
}

std::string Exec::GetFullLibPath() {
  struct link_map *linkMap;

  // Open lib, check it exists
  auto handle = dlopen(libName.c_str(), RTLD_LAZY);
  if (handle == NULL) {
    err(EXIT_FAILURE, "Cannot dlopen library %s", libName.c_str());
  }
  // Lib exists, get full path
  auto ret = dlinfo(handle, RTLD_DI_LINKMAP, &linkMap);
  if (ret < 0) {
    err(EXIT_FAILURE, "Cannot dlinfo library %s", libName.c_str());
  }

  return linkMap->l_name;
}

json Exec::ExecProgram(int numThreads) {
  auto execArgv = &argv[1];
  auto execPath = this->execPath.c_str();

  // Set num threads
  auto threadArg = execArgs[execArgsType::NUM_THREADS];
  snprintf(threadArg, EXEC_ARG_LEN, "--kokkos-num-threads=%d", numThreads);

  if (fork() == 0) {
    execv(execPath, execArgv);
  } else {
    wait(NULL);
  }

  auto progLogFile = std::ifstream(progLogName);
  return json::parse(progLogFile);
}

json Exec::ExecRun(int maxThreads) {
  if (maxThreads > nproc || maxThreads <= 0) {
    throw std::invalid_argument("Invalid maxThreads");
  }
  json runJson;
  for (int numThreads = 1; numThreads <= maxThreads; numThreads++) {
    auto threadJson = ExecProgram(numThreads);
    runJson.push_back({{"num_threads", numThreads}, {"run_log", threadJson}});
  }

  return runJson;
}

json Exec::Exec(int numRuns) { return Exec(numRuns, nproc); }

json Exec::Exec(int numRuns, int maxThreads) {
  json rootJson;
  auto fileMode = std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
  logFile.open(logName, fileMode);
  for (int run = 0; run < numRuns; run++) {
    auto runJson = ExecRun(maxThreads);
    rootJson.push_back({{"run_id", run}, {"run_log", runJson}});
    // Write to file on every run
    logFile << rootJson.dump(2);
  }

  return rootJson;
}
