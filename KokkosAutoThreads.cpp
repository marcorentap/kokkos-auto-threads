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

  auto result = exec.Exec(10);
  auto analyzer = KokkosAutoThreads::Analyzer(result);
  auto summary = analyzer.Summarize();
  std::ofstream(KokkosAutoThreads::summaryName) << summary.dump(2);
  std::cout << "Exporting database..." << std::endl;
  analyzer.ExportDB();

  return 0;
}
