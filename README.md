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

kokkosautothreads exports a table of `run_id`, `num_threads`, `hook_type`, `kernel_id`, `kernel_name`, `exec_time` as an
sqlite3 database. For example, to view the average execution time for each kernel and number of threads in ascending order,

```
sqlite3 kokkosautothreads.db

sqlite> .mode column
sqlite> select kernel_name, num_threads, avg(exec_time) from results group by kernel_name, num_threads order by kernel_name, avg(exec_time);
```

will output something like

```
kernel_name                                       num_threads  avg(exec_time)
------------------------------------------------  -----------  --------------
Kokkos::View::initialization [A View] via memset  3            8467.3
Kokkos::View::initialization [A View] via memset  8            8568.8
Kokkos::View::initialization [A View] via memset  7            8587.4
Kokkos::View::initialization [A View] via memset  4            8615.4
Kokkos::View::initialization [A View] via memset  6            8789.3
Kokkos::View::initialization [A View] via memset  2            9058.8
Kokkos::View::initialization [A View] via memset  12           9396.3
Kokkos::View::initialization [A View] via memset  16           9401.4
Kokkos::View::initialization [A View] via memset  15           9409.6
Kokkos::View::initialization [A View] via memset  13           9436.6
Kokkos::View::initialization [A View] via memset  11           9448.4
Kokkos::View::initialization [A View] via memset  14           9526.3
Kokkos::View::initialization [A View] via memset  10           9670.8
Kokkos::View::initialization [A View] via memset  5            9718.3
Kokkos::View::initialization [A View] via memset  9            10772.6
Kokkos::View::initialization [A View] via memset  1            11186.3
Print Values                                      3            75546.7
Print Values                                      5            91282.4
Print Values                                      6            92921.2
Print Values                                      4            93094.5
Print Values                                      7            93497.3
Print Values                                      9            112818.4
Print Values                                      8            116681.4
Print Values                                      10           133743.1
Print Values                                      12           140133.5
Print Values                                      11           147486.0
Print Values                                      13           159228.3
Print Values                                      14           187126.3
Print Values                                      1            311922.2
Print Values                                      2            372291.3
Print Values                                      15           579788.0
Print Values                                      16           2527508.4
Set Values                                        1            6095.48
Set Values                                        6            9440.46
Set Values                                        2            9788.49
Set Values                                        8            11388.18
Set Values                                        10           11425.96
Set Values                                        9            12384.86
Set Values                                        3            12558.08
Set Values                                        4            12895.6
Set Values                                        7            13083.7
Set Values                                        14           13218.94
Set Values                                        11           13219.22
Set Values                                        5            13640.99
Set Values                                        13           14195.39
Set Values                                        12           14475.56
Set Values                                        15           14626.58
Set Values                                        16           172966.12
```
