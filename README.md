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
sqlite3 database. For example, to view the average kernel execution times

```
sqlite3 kokkosautothreads.db

sqlite> .mode column
sqlite> select kernel_name, avg(exec_time) from results group by kernel_name;
```

will give an output like

```
kernel_name                                                          avg(exec_time)
-------------------------------------------------------------------  ----------------
10Dotproduct                                                         108933.652138158
12LeveledSweep                                                       103917.454396056
13fillColorsInd                                                      141139.584016393
13fillColorsMap                                                      187259.381001021
18RestrictionFunctor                                                 105490.488505747
19ProlongationFunctor                                                103185.157764996
7mapScan                                                             1104251.99795082
8AlphaOne                                                            106581.641975309
Kokkos::View::initialization [CrsMatrix: GlobalIndexMap] via memset  123041.626283368
Kokkos::View::initialization [CrsMatrix: LocalIndexMap] via memset   61037.0737704918
Kokkos::View::initialization [CrsMatrix: RowMap] via memset          7111.15195071869
Kokkos::View::initialization [CrsMatrix: RowMap_mirror] via memset   7145.73565573771
Kokkos::View::initialization [CrsMatrix: Values] via memset          129807.973360656
Kokkos::View::initialization [Matrix Diagonal] via memset            4280.65637860082
Kokkos::View::initialization [Vector: Values] via memset             6072.57095709571
Kokkos::View::initialization [b_lev_ind] via memset                  4377.07581967213
Kokkos::View::initialization [b_lev_map] via memset                  2852.26899383984
Kokkos::View::initialization [b_row_level] via memset                4645.38603696099
Kokkos::View::initialization [f2cOperator] via memset                4385.15432098765
Kokkos::View::initialization [f_lev_ind] via memset                  4626.5387755102
Kokkos::View::initialization [f_lev_map] via memset                  3263.55918367347
Kokkos::View::initialization [f_row_level] via memset                5072.07755102041
Kokkos::View::initialization [z] via memset                          6727.3396381579
KokkosSparse::spmv<NoTranspose,Static>                               168447.176229508
```
