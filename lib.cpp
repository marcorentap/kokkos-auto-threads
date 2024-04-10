#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "json.hpp"

std::ofstream outputFile;
nlohmann::json outputJson;
using cppclock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<cppclock, cppclock::duration>;
using time_precision = std::chrono::nanoseconds;
using std::chrono::duration_cast;
time_point tick, tock;
time_point wholeTick, wholeTock;

std::unordered_map<int, std::string> kernelNames;
uint64_t kernelCount = 0;

std::string outputFilename = "kokkosautothreads.local.json";

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  wholeTick = cppclock::now();
  outputFile.open(outputFilename, std::ios_base::trunc);
}

extern "C" void kokkosp_finalize_library() {
  wholeTock = cppclock::now();
  auto timeDiff = duration_cast<time_precision>(tock - tick).count();
  outputJson.push_back({{"hook_type", "library"}, {"exec_time", timeDiff}});
  outputFile << outputJson << std::endl;
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
  outputJson.push_back({{"hook_type", "parallel_for"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", timeDiff}});
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  kernelNames[kernelCount] = name;
  tick = cppclock::now();
  *kID = kernelCount++;
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  tock = cppclock::now();
  auto timeDiff = duration_cast<time_precision>(tock - tick).count();
  outputJson.push_back({{"hook_type", "parallel_for"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", timeDiff}});
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  kernelNames[kernelCount] = name;
  tick = cppclock::now();
  *kID = kernelCount++;
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  tock = cppclock::now();
  auto timeDiff = duration_cast<time_precision>(tock - tick).count();
  outputJson.push_back({{"hook_type", "parallel_for"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", timeDiff}});
}
