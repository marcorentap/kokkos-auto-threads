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
measureType hwCacheMeasure, pgFaultMeasure;

time_point kernelTick, kernelTock, libTimeTick, libTimeTock;
json kernelCacheTick, kernelCacheTock, libCacheTick, libCacheTock;
json kernelPgFaultTick, kernelPgFaultTock, libPgFaultTick, libPgFaultTock;

template <class T>
T JsonDiff(json tick, json tock, std::string fieldName) {
  T tickValue = tick[fieldName];
  T tockValue = tock[fieldName];
  return tockValue - tickValue;
}

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
  hwCacheMeasure = linuxTracer.MakeMeasure({
      HLMType::HWCacheReferences,
      HLMType::HWCacheMisses,
  });
  pgFaultMeasure = linuxTracer.MakeMeasure({
      HLMType::SWPageFaults,
      HLMType::SWPageFaultsMaj,
      HLMType::SWPageFaultsMin,
  });

  libTimeTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  libCacheTick = hwCacheMeasure->GetJSON();
  libPgFaultTick = pgFaultMeasure->GetJSON();
}

extern "C" void kokkosp_finalize_library() {
  libTimeTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  libCacheTock = hwCacheMeasure->GetJSON();
  libPgFaultTock = pgFaultMeasure->GetJSON();

  auto libElapsed = libTimeTock - libTimeTick;
  auto execTime = duration_cast<display_unit>(libElapsed).count();
  auto cacheMissDiff =
      JsonDiff<int>(libCacheTick, libCacheTock, "hw_cache_misses");
  auto cacheRefsDiff =
      JsonDiff<int>(libCacheTick, libCacheTock, "hw_cache_references");
  auto pgFaultDiff =
      JsonDiff<int>(libPgFaultTick, libPgFaultTock, "sw_page_faults");
  auto pgFaultMinDiff =
      JsonDiff<int>(libPgFaultTick, libPgFaultTock, "sw_page_faults_min");
  auto pgFaultMajDiff =
      JsonDiff<int>(libPgFaultTick, libPgFaultTock, "sw_page_faults_maj");

  outputJson.push_back({
      {"hook_type", "library"},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
      {"sw_page_faults", pgFaultDiff},
      {"sw_page_faults_min", pgFaultMinDiff},
      {"sw_page_faults_maj", pgFaultMajDiff},
  });

  outputFile << outputJson << std::endl;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTick = hwCacheMeasure->GetJSON();
  kernelPgFaultTick = pgFaultMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  if (kernelCounts[kID] > 10) return;
  kernelTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTock = hwCacheMeasure->GetJSON();
  kernelPgFaultTock = pgFaultMeasure->GetJSON();

  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_misses");
  auto cacheRefsDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_references");
  auto pgFaultDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults");
  auto pgFaultMinDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_min");
  auto pgFaultMajDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_maj");

  outputJson.push_back({
      {"hook_type", "parallel_for"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
      {"sw_page_faults", pgFaultDiff},
      {"sw_page_faults_min", pgFaultMinDiff},
      {"sw_page_faults_maj", pgFaultMajDiff},
  });
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTick = hwCacheMeasure->GetJSON();
  kernelPgFaultTick = pgFaultMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (kernelCounts[kID] > 10) return;
  kernelTock = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTock = hwCacheMeasure->GetJSON();
  kernelPgFaultTock = pgFaultMeasure->GetJSON();

  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_misses");
  auto cacheRefsDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_references");
  auto pgFaultDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults");
  auto pgFaultMinDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_min");
  auto pgFaultMajDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_maj");

  outputJson.push_back({
      {"hook_type", "parallel_scan"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
      {"sw_page_faults", pgFaultDiff},
      {"sw_page_faults_min", pgFaultMinDiff},
      {"sw_page_faults_maj", pgFaultMajDiff},
  });
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  kernelTick = cppclock::now();
  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTick = hwCacheMeasure->GetJSON();
  kernelPgFaultTick = pgFaultMeasure->GetJSON();

  SetKernelId(name, kID);
  IncrKernelCount(*kID);
  if (kernelCounts[*kID] > 10) return;
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kernelCounts[kID] > 10) return;
  kernelTock = cppclock::now();

  hwCacheMeasure->DoMeasure();
  pgFaultMeasure->DoMeasure();

  kernelCacheTock = hwCacheMeasure->GetJSON();
  kernelPgFaultTock = pgFaultMeasure->GetJSON();

  auto kernelElapsed = kernelTock - kernelTick;
  auto execTime = duration_cast<display_unit>(kernelElapsed).count();
  auto cacheMissDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_misses");
  auto cacheRefsDiff =
      JsonDiff<int>(kernelCacheTick, kernelCacheTock, "hw_cache_references");
  auto pgFaultDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults");
  auto pgFaultMinDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_min");
  auto pgFaultMajDiff =
      JsonDiff<int>(kernelPgFaultTick, kernelPgFaultTock, "sw_page_faults_maj");

  outputJson.push_back({
      {"hook_type", "parallel_reduce"},
      {"kernel_id", kID},
      {"kernel_name", nameMap.at(kID)},
      {"exec_time", execTime},
      {"hw_cache_misses", cacheMissDiff},
      {"hw_cache_references", cacheRefsDiff},
      {"sw_page_faults", pgFaultDiff},
      {"sw_page_faults_min", pgFaultMinDiff},
      {"sw_page_faults_maj", pgFaultMajDiff},
  });
}
