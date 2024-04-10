#include <err.h>
#include <iostream>

#include "KokkosAutoThreads.hpp"
#include "json.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    errx(EXIT_FAILURE, "Expected executable path at argv[1]");
  }

  auto exec = KokkosAutoThreads::Executor(argc, argv);
  auto j = exec.Exec(10);
  auto analyzer = KokkosAutoThreads::Analyzer();
  auto a = analyzer.Summarize(j);
  std::cout << a.dump(2) << std::endl;
  return 0;
}
