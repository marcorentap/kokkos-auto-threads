#include <err.h>

#include <MPerf/Tracers/CPPChrono.hpp>
#include <MPerf/Tracers/LinuxPerf.hpp>
#include <fstream>
#include <iostream>

#include "KokkosAutoThreads.hpp"

using json = nlohmann::json;
using HLMType = MPerf::HLMeasureType;

auto logName = KokkosAutoThreads::progLogName;
auto outputFile = std::ofstream(logName);

// Measurements
using linuxTracerType = MPerf::Tracers::LinuxPerf::Tracer;
using chronoTracerType = MPerf::Tracers::CPPChrono::Tracer;
using measureType = std::unique_ptr<MPerf::Measure>;
linuxTracerType linuxTracer;
chronoTracerType chronoTracer;

// List of measure (tick, tock) pair
std::vector<measureType> measures;
std::vector<json> kernelTicks, libraryTicks;
std::vector<json> kernelTocks, libraryTocks;

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

inline void checkOpenFds(measureType &measure, int expected) {
  using linuxMeasureType = MPerf::Tracers::LinuxPerf::Measure;
  auto asLinuxMeasure = dynamic_cast<linuxMeasureType *>(measure.get());
  if (asLinuxMeasure->GetOpenFDCount() != expected) {
    errx(EXIT_FAILURE,
         "Cannot open perf events, consider checking "
         "/proc/sys/kernel/perf_event_paranoid");
  }
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  // Create and register measures
  auto timeMeasure = chronoTracer.MakeMeasure(HLMType::Time);
  auto hwCacheMeasure = linuxTracer.MakeMeasure({
      HLMType::HWCacheReferences,
      HLMType::HWCacheMisses,
  });
  auto pgFaultMeasure = linuxTracer.MakeMeasure({
      HLMType::SWPageFaults,
      HLMType::SWPageFaultsMaj,
      HLMType::SWPageFaultsMin,
  });

  // All perf event must be successful
  checkOpenFds(hwCacheMeasure, 2);
  checkOpenFds(pgFaultMeasure, 3);

  // Register measure
  measures.push_back(std::move(timeMeasure));
  measures.push_back(std::move(hwCacheMeasure));
  measures.push_back(std::move(pgFaultMeasure));

  auto j = GetCurrentMeasurements();
  j["hook_type"] = "library";
  libraryTicks.push_back(j);
}

extern "C" void kokkosp_finalize_library() {
  json outputJson;
  auto j = GetCurrentMeasurements();
  libraryTocks.push_back(j);

  // Calculate kernel deltas
  for (int i = 0; i < kernelTicks.size(); i++) {
    auto tick = kernelTicks.at(i);
    auto tock = kernelTocks.at(i);
    json delta;
    for (auto tickItem : tick.items()) {
      auto key = tickItem.key();
      if (key == "kernel_name" || key == "hook_type") continue;

      auto diff = JsonDiff<uint64_t>(tick, tock, key);
      delta[key] = diff;
    }
    delta.update({
        {"kernel_name", tick["kernel_name"]},
        {"hook_type", tick["hook_type"]},
    });
    outputJson.push_back(delta);
  }

  // Calculate library deltas
  outputFile << outputJson << std::endl;
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_for";
  j["kernel_name"] = name;
  kernelTicks.push_back(j);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  auto cur = kernelTicks.back();
  j["hook_type"] = cur["hook_type"];
  j["kernel_name"] = cur["kernel_name"];
  kernelTocks.push_back(j);
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_scan";
  kernelTicks.push_back(j);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  auto cur = kernelTicks.back();
  j["hook_type"] = cur["hook_type"];
  j["kernel_name"] = cur["kernel_name"];
  kernelTocks.push_back(j);
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  auto j = GetCurrentMeasurements();
  j["hook_type"] = "parallel_reduce";
  kernelTicks.push_back(j);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  auto j = GetCurrentMeasurements();
  auto cur = kernelTicks.back();
  j["hook_type"] = cur["hook_type"];
  j["kernel_name"] = cur["kernel_name"];
  kernelTocks.push_back(j);
}
