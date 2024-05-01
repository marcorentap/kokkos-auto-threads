#include <dlfcn.h>
#include <err.h>
#include <link.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
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
    char *LD_LIBRARY_PATH = getenv("LD_LIBRARY_PATH");
    if (LD_LIBRARY_PATH != NULL) {
      err(EXIT_FAILURE, "Cannot dlopen library %s with LD_LIBRARY_PATH=%s", libName.c_str(), LD_LIBRARY_PATH);
    } else {
      err(EXIT_FAILURE, "Cannot dlopen library %s", libName.c_str());
    }
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

  int pid = fork();

  if (pid < 0) {
    err(EXIT_FAILURE, "Cannot fork: ");
  }

  if (pid == 0) {
    cpu_set_t cpuSet;
    sched_param param;
    int policy = this->schedPolicy;

    // Make child highest priority
    if (sched_getparam(pid, &param) < 0) {
      err(EXIT_FAILURE, "Cannot get child scheduling parameters: ");
    }
    param.sched_priority = sched_get_priority_max(policy);
    if (sched_setparam(pid, &param) < 0) {
      err(EXIT_FAILURE, "Cannot set child scheduling parameters: ");
    }

    // Limit child to CPUs 0 to numThreads-1
    CPU_ZERO(&cpuSet);
    for (int i = 0; i < numThreads; i++) {
      CPU_SET(i, &cpuSet);
    }
    if (sched_setaffinity(pid, sizeof cpuSet, &cpuSet) < 0) {
      err(EXIT_FAILURE, "Cannot set child affinity: ");
    }

    // Set child scheduler
    if (sched_setscheduler(pid, policy, &param) < 0) {
      err(EXIT_FAILURE, "Cannot set child scheduler: ");
    }

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
  for (int run = 0; run < numRuns; run++) {
    auto runJson = ExecRun(maxThreads);
    rootJson.push_back({{"run_id", run}, {"run_log", runJson}});
    // Write to file on every run
    logFile.open(logName, fileMode);
    logFile << rootJson.dump(2);
    logFile.close();
  }

  return rootJson;
}
