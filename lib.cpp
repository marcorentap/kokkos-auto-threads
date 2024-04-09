#include <chrono>
#include <cstdint>
#include <fstream>

#include "json.hpp"
#include "kokkosautothreads.hpp"

std::ofstream outputFile;
nlohmann::json outputJson;
using cppclock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<cppclock, cppclock::duration>;
using time_precision = std::chrono::nanoseconds;
using std::chrono::duration_cast;
time_point tick, tock;

std::unordered_map<int, std::string> kernelNames;
uint64_t kernelCount = 0;

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  outputFile.open(localOutputFilename, std::ios_base::trunc);
  outputFile << "test";
}

extern "C" void kokkosp_finalize_library() {
  outputFile << outputJson;
  outputFile.flush();
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  kernelNames[kernelCount] = name;
  tick = cppclock::now();
  *kID = kernelCount++;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  tock = cppclock::now();
  auto timeDiff = duration_cast<time_precision>(tock - tick).count();
  outputJson.push_back({{"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"time", timeDiff}});
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {}

extern "C" void kokkosp_begin_fence(const char *name, const uint32_t devID,
                                    uint64_t *kID) {}

extern "C" void kokkosp_end_fence(const uint64_t kID) {}
