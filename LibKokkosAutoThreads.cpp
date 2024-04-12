// #include <MPerf/Core.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "KokkosAutoThreads.hpp"
#include "mperf/LinuxPerf.hpp"

using cppclock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<cppclock, cppclock::duration>;
using display_unit = std::chrono::nanoseconds;
using json = nlohmann::json;
using HLMType = MPerf::HLMeasureType;
using std::chrono::duration_cast;

auto logName = KokkosAutoThreads::progLogName;
auto outputFile = std::ofstream(logName);
auto outputJson = json();

// Variables to keep track of kernel name, id, count
auto idMap = std::unordered_map<std::string, int>();
auto nameMap = std::unordered_map<int, std::string>();
auto kernelCounts = std::unordered_map<uint64_t, size_t>();

// Measurements
using tracerType = MPerf::Tracers::LinuxPerf::Tracer;
using measureType = std::unique_ptr<MPerf::Measure>;
tracerType linuxTracer;
measureType hwCacheMeasure;

time_point kernelTick, kernelTock, libTimeTick, libTimeTock;
json kernelCacheTick, kernelCacheTock, libCacheTick, libCacheTock;

inline void SetKernelId(const char *name, uint64_t *kID) {
  if (idMap.find(name) == idMap.end()) {
    idMap[name] = idMap.size();
  }
  *kID = idMap[name];
  nameMap[*kID] = name;
}

inline void IncrKernelCount(uint64_t kID) {
  if (kernelCounts.find(kID) == kernelCounts.end()) {
    kernelCounts[kID] = 1;
  } else {
    kernelCounts[kID]++;
  }
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  // Create and register measures
  // Do measurement every parallel for, reduce, scan
  hwCacheMeasure = linuxTracer.MakeMeasure(
      {HLMType::HWCacheReferences, HLMType::HWCacheMisses});

  libTimeTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  libCacheTick = hwCacheMeasure->GetJSON();
}

extern "C" void kokkosp_finalize_library() {
  libTimeTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  libCacheTock = hwCacheMeasure->GetJSON();

  auto libElapsed = libTimeTock - libTimeTick;
  auto execTime = duration_cast<display_unit>(libElapsed).count();
  auto cacheMissDiff = (int)libCacheTock["hw_cache_misses"] -
                       (int)libCacheTick["hw_cache_misses"];
  auto cacheRefsDiff = (int)libCacheTock["hw_cache_references"] -
                       (int)libCacheTick["hw_cache_references"];

  outputJson.push_back({
      {"hook_type", "library"},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
  });

  outputFile << outputJson << std::endl;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTick = hwCacheMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  kernelTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTock = hwCacheMeasure->GetJSON();

  if (kernelCounts[kID] > 10) return;
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff = (int)kernelCacheTock["hw_cache_misses"] -
                       (int)kernelCacheTick["hw_cache_misses"];
  auto cacheRefsDiff = (int)kernelCacheTock["hw_cache_references"] -
                       (int)kernelCacheTick["hw_cache_references"];

  outputJson.push_back({
      {"hook_type", "parallel_for"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
  });
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTick = hwCacheMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  kernelTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTock = hwCacheMeasure->GetJSON();

  if (kernelCounts[kID] > 10) return;
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff = (int)kernelCacheTock["hw_cache_misses"] -
                       (int)kernelCacheTick["hw_cache_misses"];
  auto cacheRefsDiff = (int)kernelCacheTock["hw_cache_references"] -
                       (int)kernelCacheTick["hw_cache_references"];

  outputJson.push_back({
      {"hook_type", "parallel_scan"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
  });
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTick = hwCacheMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  kernelTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  kernelCacheTock = hwCacheMeasure->GetJSON();

  if (kernelCounts[kID] > 10) return;
  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff = (int)kernelCacheTock["hw_cache_misses"] -
                       (int)kernelCacheTick["hw_cache_misses"];
  auto cacheRefsDiff = (int)kernelCacheTock["hw_cache_references"] -
                       (int)kernelCacheTick["hw_cache_references"];

  outputJson.push_back({
      {"hook_type", "parallel_reduce"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
  });
}
