# SchedLab

A small C++20 command-line CPU scheduling simulator using STL, CMake, and GoogleTest.

SchedLab simulates CPU scheduling policies in software. It does not modify
the host operating system's actual scheduler.

## Algorithms and features

- FCFS, non-preemptive SJF, SRTF, non-preemptive/preemptive Priority, Round Robin, and three-level MLFQ.
- Arrival times, idle periods, configurable context-switch overhead, deterministic ties.
- Execution timeline, completion order, per-process metrics, and comparison of all seven variants.
- Completion, turnaround, waiting, response, their relevant averages, throughput, CPU utilization, and context-switch count.

## Build and test

Requires CMake 3.20+, Git, and a modern C++20 compiler. The first test-enabled
configuration downloads GoogleTest v1.15.2; the executable does not use GoogleTest.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

For MinGW on Windows, add `-G "MinGW Makefiles"` to the configure command.
Run `build/schedlab` on Linux/macOS, `build/schedlab.exe` with MinGW, or
`build/Release/schedlab.exe` with Visual Studio. The commands below use the Linux/macOS path.
MinGW and MSVC builds link their compiler runtimes statically; normal OS libraries remain required.

For an executable-only build without downloading GoogleTest:

```sh
cmake -S . -B out -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build out --config Release --parallel
```

## Usage

```sh
./build/schedlab --algorithm fcfs --input examples/basic.txt
./build/schedlab --algorithm sjf --input examples/basic.txt
./build/schedlab --algorithm srtf --input examples/preemption.txt
./build/schedlab --algorithm priority --input examples/priority.txt
./build/schedlab --algorithm priority --preemptive --input examples/priority.txt
./build/schedlab --algorithm rr --quantum 2 --input examples/round_robin.txt
./build/schedlab --algorithm mlfq --input examples/mlfq.txt --context-switch 1
./build/schedlab --compare --input examples/basic.txt --quantum 2 --context-switch 1
./build/schedlab --help
```

Comparison runs the original workload independently through every variant.
`--quantum` applies to RR (default 2); MLFQ always uses its fixed queue rules.
Errors print to stderr and return exit code 1. No arguments displays help.

## Workload format

```text
# pid arrival burst priority
1 0 8 2
2 1 4 1
3 2 1 3
```

Each nonblank row contains exactly four integers. `#` begins a comment, including
inline comments. PIDs must be unique, nonnegative 32-bit integers. Arrival times
must be nonnegative and bursts positive. Times use signed 64-bit integers;
overflow is reported as an error. Priorities are signed 32-bit integers, with
smaller values meaning higher priority. All four columns are required for every algorithm.

## Scheduling rules

- Selection ties use arrival time, then PID. RR/MLFQ enqueue simultaneous arrivals in PID order.
- RR enqueues arrivals through the quantum boundary before requeuing the unfinished process.
- MLFQ: Q0 uses RR(2), Q1 uses RR(4), Q2 uses FCFS. New jobs enter Q0;
  consuming a full quantum demotes unfinished jobs. Q0 arrivals preempt lower queues.
  Interrupted jobs retain their unused quantum and return to the front of their queue.
- A switch is a direct change between different running processes. Initial dispatch,
  continuation of the same process, and dispatch after a genuine idle gap are free.
  Switches are counted even when their cost is zero. Arrivals during switch overhead
  participate in the final dispatch decision without another overhead charge.
- Time starts at zero. Throughput is completed jobs / elapsed time. Utilization is
  useful CPU execution / elapsed time × 100; idle time and overhead are excluded from useful work.
- Turnaround = completion − arrival; waiting = turnaround − burst;
  response = first execution − arrival. Adjacent slices of the same process are merged.

## Example output

For `--algorithm srtf --input examples/preemption.txt` (excerpt):

```text
Algorithm: SRTF

Timeline:
[0,1) P1
[1,2) P2
[2,3) P3
[3,6) P2
[6,13) P1
Completion order: P3 P2 P1

Process AT BT Priority CT TAT WT RT
P1 0 8 2 13 13 5 0
P2 1 4 1 6 5 1 0
P3 2 1 3 3 1 0 0
```

## Structure and limitations

`include/` contains four small headers; `src/` contains the scheduler, metrics,
CLI, and main implementations. `tests/` holds GoogleTest coverage;
`examples/` holds five workloads. Tests include exact timelines and hand-calculated
metrics, CLI validation, overflow checks, and 100 seeded workloads across all variants.

Designed for small CPU-only educational workloads. No I/O bursts, real processes,
threads, priority aging, or MLFQ boosts. Starvation is possible under priority-based
policies; RR with huge bursts and tiny quanta can take many simulation steps.
No scheduler is declared universally best. Locally verified with GCC/MinGW on Windows;
Linux, macOS, and MSVC execution have not been verified here.
