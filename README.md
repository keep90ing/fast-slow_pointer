# Fast/Slow Pointer Benchmark

## Overview
This project benchmarks middle-node traversal on singly linked lists and provides a utility to inspect node layout.

The repository documents only two executables:
- `bench`: performance measurement for traversal algorithms.
- `check`: structural inspection of the generated linked list.

## Build
```bash
make
```

## `bench`
### Purpose
`bench` builds a linked list, measures only the traversal phase with `perf_event_open`, and reports performance counters.

### Usage
```bash
./bin/bench <mode> <count> <algo> [seed]
```

### Arguments
- `mode`: `sequential|1` or `random|2`
- `count`: number of nodes (`> 0`)
- `algo`: `single|single_pointer|1` or `fastslow|fast_and_slow|2`
- `seed`: optional unsigned integer (useful for reproducible random-mode runs)

### Output
`bench` outputs:
- `task-clock(ns)`
- `cpu-cycles`
- `cache-miss-rate`
- `L1-dcache-load-miss-rate`
- `l1-dcache-prefetches` (or `N/A` when unsupported)

### Examples
```bash
./bin/bench sequential 1000000 single
./bin/bench random 1000000 fastslow 12345
```

## `check`
### Purpose
`check` prints the linked-list traversal order, including each node value, node address, and next pointer.

### Usage
```bash
./bin/check <mode> <count> [seed]
```

### Arguments
- `mode`: `sequential|1` or `random|2`
- `count`: number of nodes (`> 0`)
- `seed`: optional unsigned integer (used in random mode)

### Example
```bash
./bin/check random 8 42
```
