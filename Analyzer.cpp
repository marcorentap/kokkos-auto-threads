#include <sqlite3.h>

#include <MPerf/Core.hpp>
#include <cstdio>
#include <sstream>

#include "KokkosAutoThreads.hpp"

using KokkosAutoThreads::Analyzer;
using nlohmann::json;

Analyzer::Analyzer(json data) { this->data = data; }

std::string join(std::vector<std::string> const &strings, std::string delim) {
  std::stringstream ss;
  std::copy(strings.begin(), strings.end(),
            std::ostream_iterator<std::string>(ss, delim.c_str()));
  return ss.str();
}

void Analyzer::ExportDB() {
  sqlite3 *db;

  // List of labels
  std::vector<std::string> labelList;
  for (auto &type : Executor::HLMTypes) {
    auto label = MPerf::HLTypeLabels[type];
    labelList.push_back(label);
  }

  // Create query to create table
  std::string createTableQuery =
      "DROP TABLE IF EXISTS results;"
      "CREATE TABLE results ("
      "run_id int,"
      "num_threads int,"
      "hook_type varchar(64),"
      "kernel_name varchar(64),";
  for (int i = 0; i < labelList.size(); i++) {
    auto label = labelList[i];

    if (i > 0) createTableQuery.append(",");
    createTableQuery.append(label + " int");
  }
  createTableQuery.append(");");

  // Create table
  sqlite3_open(dbName.c_str(), &db);
  sqlite3_exec(db, createTableQuery.c_str(), NULL, NULL, NULL);

  for (auto &run : data) {
    std::string wholeQuery = "";
    for (auto &threadRun : run["run_log"]) {
      std::string runQuery = "INSERT INTO results VALUES ";
      for (auto &kernelLog : threadRun["run_log"]) {
        char kernelTuple[4096] = {0};
        std::vector<uint64_t> measureValues;

        // Kernel and application has these
        uint64_t runId = run["run_id"];
        uint64_t numThreads = threadRun["num_threads"];
        std::string hookType = kernelLog["hook_type"];
        std::string kernelName = kernelLog["kernel_name"];
        uint64_t execTime = kernelLog["time"];  // Follow MPerf's naming
        uint64_t hwCacheMisses = kernelLog["hw_cache_misses"];
        uint64_t hwCacheRefs = kernelLog["hw_cache_references"];
        uint64_t swPgFault = kernelLog["sw_page_faults"];
        uint64_t swPgFaultMin = kernelLog["sw_page_faults_min"];
        uint64_t swPgFaultMaj = kernelLog["sw_page_faults_maj"];

        // Get measurements
        for (auto &label : labelList) {
          measureValues.push_back(kernelLog[label]);
        }

        // Create record
        std::stringstream record;
        record << "(";
        record << runId << ", ";
        record << numThreads << ", ";
        record << "'" << hookType.c_str() << "', ";
        record << (kernelName == "" ? "NULL" : "'" + kernelName + "'");

        for (auto &value : measureValues) {
          record << ", " << value;
        }

        record << "),";
        runQuery.append(record.str());
      }
      // Replace trailing comma with semicolon
      runQuery[runQuery.length() - 1] = ';';
      wholeQuery.append(runQuery);
    }
    sqlite3_exec(db, wholeQuery.c_str(), NULL, NULL, NULL);
  }
}

json Analyzer::Summarize() { return json(); }
