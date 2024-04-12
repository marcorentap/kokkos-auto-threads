#include "LinuxPerf.hpp"

#include <err.h>

#include <cstdio>
#include <stdexcept>

namespace MPerf {
namespace Tracers {
namespace LinuxPerf {

void Measure::PerfEventOpen(uint32_t type, uint64_t config) {
  auto typeString = std::to_string(type);
  auto configString = std::to_string(config);
  PerfEventOpen("unknown" + typeString + configString, type, config);
}

void Measure::PerfEventOpen(std::string label, uint32_t type, uint64_t config) {
  int fd;
  perf_event_attr attr{
      .type = type, .config = config, .read_format = PERF_FORMAT_GROUP};

  if (fds.size() == 0) {
    fd = perf_event_open(&attr, getpid(), -1, -1, 0);
    leader_fd = fd;
  } else {
    fd = perf_event_open(&attr, getpid(), -1, leader_fd, 0);
  }

  // Skip if event not available
  if (fd < 0) {
    std::cerr << "WARNING: " << __FUNCTION__ << " cannot open fd";
    std::cerr << " for type " << type;
    std::cerr << " config " << config << "\n";
    return;
  }

  labelToResultIndex[label] = fds.size();
  fds.push_back(fd);
}

void Measure::DoMeasure() {
  memset(&result, 0, sizeof(result));
  int ret = read(leader_fd, &result, sizeof(result));
  if (ret < 0) {
    exit(EXIT_FAILURE);
  }
}
json Measure::GetJSON() {
  json j;
  for (auto &item : labelToResultIndex) {
    auto label = item.first;
    auto index = item.second;
    j[label] = result.values[index];
  }
  return j;
}

uPtrBMeasure Tracer::MakeMeasure(HLMType hlType) {
  return MakeMeasure(std::vector<HLMeasureType>({hlType}));
};
uPtrBMeasure Tracer::MakeMeasure(std::vector<HLMeasureType> hlTypes) {
  uPtrLinuxMeasure ptr;
  ptr.reset(new Measure());
  for (auto &hlType : hlTypes) {
    auto label = hlTolabel[hlType];
    auto type = hlToType[hlType];
    auto config = hlToConfig[hlType];
    ptr->PerfEventOpen(label, type, config);
  }
  return ptr;
}

}  // namespace LinuxPerf
}  // namespace Tracers
}  // namespace MPerf
