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

kokkosautothreads will execute the programs in 10 runs, with each run executing with
--kokkos-num-threads set from 1 to nproc.

It outputs three files:

- `kokkosautothreads.json` is the full log
- `kokkosautothreads.summary.json` shows best number of threads and execution time for each thread
- `kokkosautothreads.db` is full log in database form

## Log database

kokkosautothreads exports the execution log as an sqlite3 table:
```
┌─────┬─────────────────────┬─────────────┬─────────┬────────────┬────┐
│ cid │        name         │    type     │ notnull │ dflt_value │ pk │
├─────┼─────────────────────┼─────────────┼─────────┼────────────┼────┤
│ 0   │ run_id              │ INT         │ 0       │            │ 0  │
│ 1   │ num_threads         │ INT         │ 0       │            │ 0  │
│ 2   │ hook_type           │ varchar(64) │ 0       │            │ 0  │
│ 3   │ kernel_id           │ INT         │ 0       │            │ 0  │
│ 4   │ kernel_name         │ varchar(64) │ 0       │            │ 0  │
│ 5   │ exec_time           │ INT         │ 0       │            │ 0  │
│ 6   │ hw_cache_misses     │ INT         │ 0       │            │ 0  │
│ 7   │ hw_cache_references │ INT         │ 0       │            │ 0  │
│ 8   │ sw_page_faults      │ INT         │ 0       │            │ 0  │
│ 9   │ sw_page_faults_min  │ INT         │ 0       │            │ 0  │
│ 10  │ sw_page_faults_maj  │ INT         │ 0       │            │ 0  │
└─────┴─────────────────────┴─────────────┴─────────┴────────────┴────┘
```

For example, to view the average execution time for each kernel and number of threads in ascending order,

```
sqlite3 kokkosautothreads.db

sqlite> .mode box
sqlite> select kernel_name, num_threads, avg(exec_time) from results where hook_type is not 'library' group by kernel_name, num_threads order by kernel_name, avg(exec_time);
```

will output something like

```
┌──────────────────────────────────────────────────┬─────────────┬────────────────┐
│                   kernel_name                    │ num_threads │ avg(exec_time) │
├──────────────────────────────────────────────────┼─────────────┼────────────────┤
│ Kokkos::View::initialization [A View] via memset │ 2           │ 13558.8        │
│ Kokkos::View::initialization [A View] via memset │ 10          │ 13979.8        │
│ Kokkos::View::initialization [A View] via memset │ 3           │ 14004.7        │
│ Kokkos::View::initialization [A View] via memset │ 5           │ 14252.1        │
│ Kokkos::View::initialization [A View] via memset │ 7           │ 14402.9        │
│ Kokkos::View::initialization [A View] via memset │ 9           │ 14859.8        │
│ Kokkos::View::initialization [A View] via memset │ 4           │ 15058.4        │
│ Kokkos::View::initialization [A View] via memset │ 8           │ 15352.3        │
│ Kokkos::View::initialization [A View] via memset │ 11          │ 15515.8        │
│ Kokkos::View::initialization [A View] via memset │ 12          │ 15836.9        │
│ Kokkos::View::initialization [A View] via memset │ 6           │ 16168.7        │
│ Kokkos::View::initialization [A View] via memset │ 1           │ 19494.9        │
│ Z4mainEUlRKiE0_                                  │ 2           │ 31937.8        │
│ Z4mainEUlRKiE0_                                  │ 3           │ 36048.2        │
│ Z4mainEUlRKiE0_                                  │ 4           │ 49801.5        │
│ Z4mainEUlRKiE0_                                  │ 5           │ 59003.1        │
│ Z4mainEUlRKiE0_                                  │ 6           │ 103386.6       │
│ Z4mainEUlRKiE0_                                  │ 7           │ 132863.9       │
│ Z4mainEUlRKiE0_                                  │ 8           │ 160474.9       │
│ Z4mainEUlRKiE0_                                  │ 9           │ 266454.2       │
│ Z4mainEUlRKiE0_                                  │ 10          │ 299206.4       │
│ Z4mainEUlRKiE0_                                  │ 1           │ 385793.7       │
│ Z4mainEUlRKiE0_                                  │ 11          │ 428798.6       │
│ Z4mainEUlRKiE0_                                  │ 12          │ 752137.8       │
│ Z4mainEUlRKiE_                                   │ 2           │ 8752.12        │
│ Z4mainEUlRKiE_                                   │ 3           │ 9396.05        │
│ Z4mainEUlRKiE_                                   │ 4           │ 9670.57        │
│ Z4mainEUlRKiE_                                   │ 6           │ 9852.78        │
│ Z4mainEUlRKiE_                                   │ 5           │ 10338.57       │
│ Z4mainEUlRKiE_                                   │ 7           │ 10700.67       │
│ Z4mainEUlRKiE_                                   │ 8           │ 10886.52       │
│ Z4mainEUlRKiE_                                   │ 9           │ 11845.76       │
│ Z4mainEUlRKiE_                                   │ 1           │ 12246.39       │
│ Z4mainEUlRKiE_                                   │ 10          │ 14650.35       │
│ Z4mainEUlRKiE_                                   │ 11          │ 16165.8        │
│ Z4mainEUlRKiE_                                   │ 12          │ 151581.32      │
└──────────────────────────────────────────────────┴─────────────┴────────────────┘
```
