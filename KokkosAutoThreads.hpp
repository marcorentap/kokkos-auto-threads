#ifndef KOKKOSAUTOTHREADS_HPP
#define KOKKOSAUTOTHREADS_HPP
#include <array>
#include <fstream>
#include <string>

#include "json.hpp"

namespace KokkosAutoThreads {

static std::string progLogName = "kokkosautothreads.tmp.json";
static std::string logName = "kokkosautothreads.json";
static std::string libName = "libkokkosautothreads.so";

class Executor {
  using json = nlohmann::json;

 private:
  static constexpr int EXEC_ARG_LEN = 64;

  enum execArgsType {
    TOOLS_LIBS,
    NUM_THREADS,
  };

  int nproc, argc;
  char *argv[512] = {nullptr};
  std::fstream logFile;
  std::string execPath, libPath;
  // --kokkos-tools-libs=, --kokkos-num-threads=
  std::array<char[64], 2> execArgs;

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

 public:
  json Summarize(json data);
};

}  // namespace KokkosAutoThreads

#endif
