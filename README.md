kokkosautothreads executes a Kokkos program multiple times and gathers
whole-application and per-kernel statistics.

# Prerequisites

- sqlite3
- Kokkos
- CMake >= v3.16

# Installation

In the project directory, run

```
  mkdir build
  cd build
  cmake ..
  make
  sudo make install
```

That should install the `kokkosautothreads` executable and
`libkokkosautothreads.so` library.

# Usage

To run a Kokkos program with kokkosautothreads, do

```
kokkosautothreads ./mykokkosapplication arg1, arg2, ...
```

kokkosautothreads will execute the programs in 10 runs, with each run executing
--kokkos-auto-threads from 1 to nproc.

It outputs three files:

- kokkosautothreads.json is the full log
- kokkosautothreads.summary.json shows best number of threads and execution time for each thread
- kokkosautothreads.db full log in database form

## Log database

kokkosautothreads exports a table of run_id, `num_threads`, `hook_type`, `kernel_id`, `kernel_name`, `exec_time` as an
sqlite3 database. For example, to view the average kernel execution times

```
sqlite3 kokkosautothreads.db

sqlite> .headers ON
sqlite> select *, avg(exec_time) from results group by kernel_name;
```

will give an output like

```
run_id|num_threads|hook_type|kernel_id|kernel_name|exec_time|avg(exec_time)
0|1|parallel_for|0|Kokkos::View::initialization [A View] via memset|14763|9998.0375
0|1|parallel_for|1|Z4mainE3$_0|16292|23187.529375
0|1|parallel_for|2|Z4mainE3$_1|74074|179949.58125
```
