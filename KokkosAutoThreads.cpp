#include "KokkosAutoThreads.hpp"

#include <err.h>

#include <fstream>
#include <iostream>

#include "json.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    errx(EXIT_FAILURE, "Expected executable path at argv[1]");
  }

  auto exec = KokkosAutoThreads::Executor(argc, argv);
  auto analyzer = KokkosAutoThreads::Analyzer();

  auto result = exec.Exec(10);
  auto summary = analyzer.Summarize(result);
  std::ofstream(KokkosAutoThreads::summaryName) << summary.dump(2);
  return 0;
}
