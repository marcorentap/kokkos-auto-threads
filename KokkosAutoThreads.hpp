#ifndef KOKKOSAUTOTHREADS_HPP
#define KOKKOSAUTOTHREADS_HPP
#include <sqlite3.h>

#include <array>
#include <fstream>
#include <string>
#include <sched.h>

#include "json.hpp"

namespace KokkosAutoThreads {

static std::string progLogName = "kokkosautothreads.tmp.json";
static std::string logName = "kokkosautothreads.json";
static std::string libName = "libkokkosautothreads.so";
static std::string summaryName = "kokkosautothreads.summary.json";
static std::string dbName = "kokkosautothreads.db";

class Executor {
  using json = nlohmann::json;

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

}  // namespace KokkosAutoThreads

#endif
