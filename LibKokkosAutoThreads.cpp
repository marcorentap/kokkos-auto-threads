#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "KokkosAutoThreads.hpp"

using cppclock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<cppclock, cppclock::duration>;
using display_unit = std::chrono::nanoseconds;
using json = nlohmann::json;
using std::chrono::duration_cast;

auto logName = KokkosAutoThreads::progLogName;
auto outputFile = std::ofstream(logName);
auto outputJson = json();

time_point kernelTick, kernelTock, libTick, libTock;
auto kernelNames = std::unordered_map<int, std::string>();
auto kernelCounts = std::unordered_map<std::string, size_t>();
auto kernelCount = 0;

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  libTick = cppclock::now();
}

extern "C" void kokkosp_finalize_library() {
  libTock = cppclock::now();
  auto libElapsed = libTock - libTick;
  auto execTime = duration_cast<display_unit>(libElapsed).count();

  outputJson.push_back({{"hook_type", "library"}, {"exec_time", execTime}});
  outputFile << outputJson << std::endl;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  kernelTick = cppclock::now();
  if (kernelCounts.find(name) == kernelCounts.end()) {
    kernelCounts[name] = 1;
  } else {
    kernelCounts[name]++;
  }

  if (kernelCounts[name] > 10) return;

  *kID = kernelCount++;
  kernelNames[*kID] = name;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  if (kernelCounts[kernelNames[kID]] > 10) return;
  kernelTock = cppclock::now();
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  outputJson.push_back({{"hook_type", "parallel_for"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", execTime}});
}
extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  if (kernelCounts.find(name) == kernelCounts.end()) {
    kernelCounts[name] = 1;
  } else {
    kernelCounts[name]++;
  }

  if (kernelCounts[name] > 10) return;

  kernelTick = cppclock::now();
  *kID = kernelCount++;
  kernelNames[*kID] = name;
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (kernelCounts[kernelNames[kID]] > 10) return;
  kernelTock = cppclock::now();
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  outputJson.push_back({{"hook_type", "parallel_scan"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", execTime}});
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  if (kernelCounts.find(name) == kernelCounts.end()) {
    kernelCounts[name] = 1;
  } else {
    kernelCounts[name]++;
  }

  if (kernelCounts[name] > 10) return;

  kernelTick = cppclock::now();
  *kID = kernelCount++;
  kernelNames[*kID] = name;
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kernelCounts[kernelNames[kID]] > 10) return;
  kernelTock = cppclock::now();
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  outputJson.push_back({{"hook_type", "parallel_reduce"},
                        {"kernel_id", kID},
                        {"kernel_name", kernelNames.at(kID)},
                        {"exec_time", execTime}});
}
