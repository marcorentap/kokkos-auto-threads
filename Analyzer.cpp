#include <iostream>
#include <memory>
#include <unordered_map>

#include "KokkosAutoThreads.hpp"

using KokkosAutoThreads::Analyzer;
using nlohmann::json;

json Analyzer::Summarize(json data) {
  json summary;
  // kID -> (kernel name, best num_threads, best exec_time)
  using resultType = std::tuple<std::string, uint64_t, uint64_t>;
  std::unordered_map<int, resultType> kernelResults;
  resultType libResult = {"library", 0, 0};

  // Each data is multiple run
  for (auto &run : data) {
    auto threadLogs = run["run_log"];
    // Each run executes multiple numThreads
    for (auto &threadRun : threadLogs) {
      auto numThreads = threadRun["num_threads"];
      auto kernelLogs = threadRun["run_log"];
      // Each thread executes multiple kernel
      for (auto &kernelLog : kernelLogs) {
        // Library result
        if (kernelLog["hook_type"] == "library") {
          auto execTime = kernelLog["exec_time"];
          auto curExecTime = std::get<2>(libResult);
          if (curExecTime == 0 || execTime < curExecTime) {
            libResult = {"library", numThreads, execTime};
          }
          continue;
        }

        auto kernelId = kernelLog["kernel_id"];
        auto kernelName = kernelLog["kernel_name"];
        auto execTime = kernelLog["exec_time"];

        // No result yet, exec time is best time
        if (kernelResults.find(kernelId) == kernelResults.end()) {
          kernelResults[kernelId] = {kernelName, numThreads, execTime};
          continue;
        }

        // Store best thread count
        auto curResult = kernelResults[kernelId];
        auto curExecTime = std::get<2>(curResult);
        if (execTime < curExecTime) {
          kernelResults[kernelId] = {kernelName, numThreads, execTime};
        }
      }
    }
  }

  // Build json
  summary["lib_num_thread"] = std::get<1>(libResult);
  summary["lib_exec_time"] = std::get<2>(libResult);

  json kernelJson;
  for (auto &kernelResult : kernelResults) {
    auto id = kernelResult.first;
    auto result = kernelResult.second;
    auto name = std::get<0>(result);
    auto numThreads = std::get<1>(result);
    auto execTime = std::get<2>(result);
    kernelJson.push_back(
        {{"name", name}, {"num_thread", numThreads}, {"exec_time", execTime}});
  }
  summary["kernels"] = kernelJson;

  return summary;
}
