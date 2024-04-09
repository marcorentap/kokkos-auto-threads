#include <chrono>
#include <cstdint>
#include <iostream>

#include "json.hpp"

std::string outputFilename = "kokkosautothreads.json";
nlohmann::json outputJson;
using cppclock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<cppclock, cppclock::duration>;
using time_precision = std::chrono::nanoseconds;
using std::chrono::duration_cast;
time_point tick, tock;

std::unordered_map<int, std::string> kernelNames;
uint64_t kernelCount = 0;

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  kernelNames.insert({*kID, name});
  tick = cppclock::now();
  *kID = kernelCount++;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  tock = cppclock::now();
  std::cout << "Kernel " << kID << " took "
            << duration_cast<time_precision>(tock - tick).count() << " nanoseconds " << std::endl;
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
