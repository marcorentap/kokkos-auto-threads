#include <sqlite3.h>

#include <cstdio>

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
      "kernel_name varchar(64),"
      "exec_time int,"  // NOTE: This is different from MPerf's naming
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
        uint64_t runId = run["run_id"];
        uint64_t numThreads = threadRun["num_threads"];
        std::string hookType = kernelLog["hook_type"];
        uint64_t execTime = kernelLog["time"];  // Follow MPerf's naming
        uint64_t hwCacheMisses = kernelLog["hw_cache_misses"];
        uint64_t hwCacheRefs = kernelLog["hw_cache_references"];
        uint64_t swPgFault = kernelLog["sw_page_faults"];
        uint64_t swPgFaultMin = kernelLog["sw_page_faults_min"];
        uint64_t swPgFaultMaj = kernelLog["sw_page_faults_maj"];

        if (kernelLog["hook_type"] == "library") {
          std::snprintf(kernelTuple, sizeof(kernelTuple),
                        "(%lu, %lu, '%s', %s, %s, %lu, %lu, %lu, %lu, %lu, %lu),",
                        runId, numThreads, hookType.c_str(), "NULL", "NULL",
                        execTime, hwCacheMisses, hwCacheRefs, swPgFault,
                        swPgFaultMin, swPgFaultMaj);
          runQuery.append(kernelTuple);
          continue;
        }

        // Only kernel has these
        std::string kernelName = kernelLog["kernel_name"];

        std::snprintf(kernelTuple, sizeof(kernelTuple),
                      "(%lu, %lu, '%s', '%s', %lu, %lu, %lu, %lu, %lu, %lu),", runId,
                      numThreads, hookType.c_str(), kernelName.c_str(),
                      execTime, hwCacheMisses, hwCacheRefs, swPgFault,
                      swPgFaultMin, swPgFaultMaj);
        runQuery.append(kernelTuple);
      }
      // Replace trailing comma with semicolon
      runQuery[runQuery.length() - 1] = ';';
      wholeQuery.append(runQuery);
    }
    sqlite3_exec(db, wholeQuery.c_str(), NULL, NULL, NULL);
  }
}

json Analyzer::Summarize() { return json(); }
