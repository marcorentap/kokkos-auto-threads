#include <err.h>

#include <MPerf/Tracers/CPPChrono.hpp>
#include <MPerf/Tracers/LinuxPerf.hpp>
#include <fstream>
#include <iostream>
#include <memory>

#include "KokkosAutoThreads.hpp"

using json = nlohmann::json;
using HLMType = MPerf::HLMeasureType;

auto logName = KokkosAutoThreads::progLogName;
auto outputFile = std::ofstream(logName);
constexpr int perKernelMaxLogCount = 20;

// Measurements
using linuxTracerType = MPerf::Tracers::LinuxPerf::Tracer;
using linuxMeasureType = MPerf::Tracers::LinuxPerf::Measure;
using chronoTracerType = MPerf::Tracers::CPPChrono::Tracer;
using measureType = std::unique_ptr<MPerf::Measure>;
linuxTracerType linuxTracer;
chronoTracerType chronoTracer;

// List of measure (tick, tock) pair
std::vector<measureType> measures;
std::vector<json> kernelTicks;
std::vector<json> kernelTocks;
json libraryTick, libraryTock;
std::map<std::string, int> kernelMeasurementCounts;

// Keep track of current kernel
std::string curKernelName;
std::string curHookType;

template <class T>
inline T JsonDiff(json tick, json tock, std::string fieldName) {
  T tickValue = tick[fieldName];
  T tockValue = tock[fieldName];
  return tockValue - tickValue;
}

json GetCurrentMeasurements() {
  // Measure
  for (auto &measure : measures) {
    measure->DoMeasure();
  }

  // Combine measure jsons
  json j;
  for (auto &measure : measures) {
    auto measureJson = measure->GetJSON();
    j.update(measureJson);
  }
  return j;
}

void PushMeasurements(json &j, std::vector<json> &dest) {
  std::string name = j["kernel_name"];
  int count = kernelMeasurementCounts.at(name);

  if (count > perKernelMaxLogCount) return;
  dest.push_back(j);
}

void UpdateKernelCount(std::string kernelName) {
  auto count = 0;
  try {
    count = kernelMeasurementCounts.at(kernelName);
  } catch (std::out_of_range e) {
    // Do nothing
  }

  kernelMeasurementCounts[kernelName] = count + 1;
}

void ResetLinuxMeasures() {
  for (auto &measure : measures) {
    auto linuxMeasure = dynamic_cast<linuxMeasureType *>(measure.get());
    if (linuxMeasure) linuxMeasure->ResetCounters();
  }
}

inline void checkOpenFds(measureType &measure, int expected) {
  using linuxMeasureType = MPerf::Tracers::LinuxPerf::Measure;
  auto asLinuxMeasure = dynamic_cast<linuxMeasureType *>(measure.get());
  if (asLinuxMeasure->GetOpenFDCount() != expected) {
    errx(EXIT_FAILURE, "Cannot open perf events");
  }
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  // Create and register measures
  auto timeMeasure = chronoTracer.MakeMeasure(HLMType::Time);
  // auto hwCacheMeasure = linuxTracer.MakeMeasure({
  //     HLMType::HWCacheReferences,
  //     HLMType::HWCacheMisses,
  // });
  // auto pgFaultMeasure = linuxTracer.MakeMeasure({
  //     HLMType::SWPageFaults,
  //     HLMType::SWPageFaultsMaj,
  //     HLMType::SWPageFaultsMin,
  // });
  auto clockInstMeasure = linuxTracer.MakeMeasure({
      HLMType::HWInstructions,
      HLMType::HWCPUCycles,
  });
  auto l1dCacheMeasure = linuxTracer.MakeMeasure({
      HLMType::HWCacheL1DReadAccess,
      HLMType::HWCacheL1DWriteAccess,
  });

  // All perf event must be successful
  // checkOpenFds(hwCacheMeasure, 2);
  // checkOpenFds(pgFaultMeasure, 3);
  checkOpenFds(clockInstMeasure, 2);
  checkOpenFds(l1dCacheMeasure, 2);

  // Register measure
  measures.push_back(std::move(timeMeasure));
  // measures.push_back(std::move(hwCacheMeasure));
  // measures.push_back(std::move(pgFaultMeasure));
  measures.push_back(std::move(clockInstMeasure));
  measures.push_back(std::move(l1dCacheMeasure));

  auto j = GetCurrentMeasurements();
  j["hook_type"] = "library";
  libraryTick = j;
}

extern "C" void kokkosp_finalize_library() {
  json outputJson;
  auto j = GetCurrentMeasurements();
  libraryTock = j;

  // Calculate kernel deltas
  for (int i = 0; i < kernelTicks.size(); i++) {
    auto tick = kernelTicks.at(i);
    auto tock = kernelTocks.at(i);
    json delta;
    for (auto tickItem : tick.items()) {
      auto key = tickItem.key();
      if (key == "kernel_name" || key == "hook_type") continue;

      uint64_t diff = JsonDiff<uint64_t>(tick, tock, key);
      diff = JsonDiff<uint64_t>(tick, tock, key);
      delta[key] = diff;
    }
    delta.update({
        {"kernel_name", tick["kernel_name"]},
        {"hook_type", tick["hook_type"]},
    });
    outputJson.push_back(delta);
  }

  // Calculate library deltas
  json libDelta;
  for (auto tickItem : libraryTick.items()) {
    auto key = tickItem.key();
    if (key == "kernel_name" || key == "hook_type") continue;

    uint64_t diff = JsonDiff<uint64_t>(libraryTick, libraryTock, key);
    libDelta[key] = diff;
  }
  libDelta.update({
      {"kernel_name", ""},
      {"hook_type", "library"},
  });
  outputJson.push_back(libDelta);

  outputFile << outputJson << std::endl;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_for";
  j["kernel_name"] = name;
  curKernelName = name;
  curHookType = "parallel_for";
  UpdateKernelCount(name);
  PushMeasurements(j, kernelTicks);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = curHookType;
  j["kernel_name"] = curKernelName;
  PushMeasurements(j, kernelTocks);
  ResetLinuxMeasures();
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_scan";
  j["kernel_name"] = name;
  curKernelName = name;
  curHookType = "parallel_scan";
  UpdateKernelCount(name);
  PushMeasurements(j, kernelTicks);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = curHookType;
  j["kernel_name"] = curKernelName;
  PushMeasurements(j, kernelTocks);
  ResetLinuxMeasures();
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_reduce";
  j["kernel_name"] = name;
  curKernelName = name;
  curHookType = "parallel_reduce";
  UpdateKernelCount(name);
  PushMeasurements(j, kernelTicks);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = curHookType;
  j["kernel_name"] = curKernelName;
  PushMeasurements(j, kernelTocks);
  ResetLinuxMeasures();
}
