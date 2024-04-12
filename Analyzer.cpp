#include <sqlite3.h>

#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "KokkosAutoThreads.hpp"

using KokkosAutoThreads::Analyzer;
using nlohmann::json;

Analyzer::Analyzer(json data) { this->data = data; }

void Analyzer::ExportDB() {
  sqlite3 *db;
  // Hacky, reset database
  sqlite3_open(dbName.c_str(), &db);
  auto createTableQuery =
      "DROP TABLE IF EXISTS results;"
      "CREATE TABLE results ("
      "run_id int,"
      "num_threads int,"
      "hook_type varchar(64),"
      "kernel_id int,"
      "kernel_name varchar(64),"
      "exec_time int,"
      "hw_cache_misses int,"
      "hw_cache_references int,"
      "sw_page_faults int,"
      "sw_page_faults_min int,"
      "sw_page_faults_maj int"
      ");";

  sqlite3_exec(db, createTableQuery, NULL, NULL, NULL);

  for (auto &run : data) {
    std::string wholeQuery = "";
    for (auto &threadRun : run["run_log"]) {
      std::string runQuery = "INSERT INTO results VALUES ";
      for (auto &kernelLog : threadRun["run_log"]) {
        char kernelTuple[4096] = {0};

        // Kernel and application has these
        int runId = run["run_id"];
        int numThreads = threadRun["num_threads"];
        std::string hookType = kernelLog["hook_type"];
        int execTime = kernelLog["exec_time"];
        int hwCacheMisses = kernelLog["hw_cache_misses"];
        int hwCacheRefs = kernelLog["hw_cache_references"];
        int swPgFault = kernelLog["sw_page_faults"];
        int swPgFaultMin = kernelLog["sw_page_faults_min"];
        int swPgFaultMaj = kernelLog["sw_page_faults_maj"];

        if (kernelLog["hook_type"] == "library") {
          std::snprintf(kernelTuple, sizeof(kernelTuple),
                        "(%d, %d, '%s', %s, %s, %d, %d, %d, %d, %d, %d),",
                        runId, numThreads, hookType.c_str(), "NULL", "NULL",
                        execTime, hwCacheMisses, hwCacheRefs, swPgFault,
                        swPgFaultMin, swPgFaultMaj);
          runQuery.append(kernelTuple);
          continue;
        }

        // Only kernel has these
        int kernelId = kernelLog["kernel_id"];
        std::string kernelName = kernelLog["kernel_name"];

        std::snprintf(kernelTuple, sizeof(kernelTuple),
                      "(%d, %d, '%s', %d, '%s', %d, %d, %d, %d, %d, %d),",
                      runId, numThreads, hookType.c_str(), kernelId,
                      kernelName.c_str(), execTime, hwCacheMisses, hwCacheRefs,
                      swPgFault, swPgFaultMin, swPgFaultMaj);
        runQuery.append(kernelTuple);
      }
      // Replace trailing comma with semicolon
      runQuery[runQuery.length() - 1] = ';';
      wholeQuery.append(runQuery);
    }
    sqlite3_exec(db, wholeQuery.c_str(), NULL, NULL, NULL);
  }
}

json Analyzer::Summarize() {
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
