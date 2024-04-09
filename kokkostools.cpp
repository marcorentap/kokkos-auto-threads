#include <cstdint>
#include <iostream>
extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  std::cout << "entered parallel for region " << name << " kID " << kID << std::endl;
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  std::cout << "ended parallel for region kID " << kID << std::endl;
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
