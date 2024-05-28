#ifndef KOKKOSAUTOTHREADS_HPP
#define KOKKOSAUTOTHREADS_HPP
#include <sched.h>
#include <sqlite3.h>

#include <MPerf/Core.hpp>
#include <array>
#include <fstream>
#include <string>

#include "json.hpp"

namespace KokkosAutoThreads {

static std::string progLogName = "kokkosautothreads.tmp.json";
static std::string logName = "kokkosautothreads.json";
static std::string libName = "libkokkosautothreads.so";
static std::string summaryName = "kokkosautothreads.summary.json";
static std::string dbName = "kokkosautothreads.db";

class Executor {
  using json = nlohmann::json;
  using HLMType = MPerf::HLMeasureType;

private:
  static constexpr int EXEC_ARG_LEN = 1024;
  int schedPolicy = SCHED_FIFO;

  enum execArgsType {
    TOOLS_LIBS,
    NUM_THREADS,
  };

  int nproc, argc;
  char *argv[512] = {nullptr};
  std::fstream logFile;
  std::string execPath, libPath;
  // --kokkos-tools-libs=, --kokkos-num-threads=
  std::array<char[EXEC_ARG_LEN], 2> execArgs;

  std::string GetFullLibPath();
  json ExecProgram(int numThreads);
  json ExecRun(int maxTheads);

public:
  constexpr static HLMType HLMTypes[] = {
      HLMType::Time,
      HLMType::HWInstructions,
      HLMType::HWCPUCycles,
      HLMType::HWCacheL1DReadAccess,
      HLMType::HWCacheL1DReadMiss,
      HLMType::HWCacheL1DWriteAccess,
      // HLMType::HWCacheReferences,
      // HLMType::HWCacheMisses,
      // HLMType::SWPageFaults,
      // HLMType::SWPageFaultsMaj,
      // HLMType::SWPageFaultsMin,
      HLMType::HWCacheL1DWriteMiss,
      HLMType::HWCacheLLReadAccess,
      HLMType::HWCacheLLReadMiss,
      HLMType::HWCycleStallsTotal,
  };
  Executor(int argc, char *argv[]);
  json Exec(int numRuns);
  json Exec(int numRuns, int maxThreads);
};

class Analyzer {
  using json = nlohmann::json;

private:
  json data;

public:
  Analyzer(json data);
  json Summarize();
  void ExportDB();
};

} // namespace KokkosAutoThreads

#endif
