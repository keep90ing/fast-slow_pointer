# Fast/Slow Pointer Locality Lab

## Node Layout Modes

1. `contiguous` (`1`)
- 一次 `malloc(N * sizeof(node))`
- `next` 串成 `&pool[i + 1]`
- 最佳 spatial locality

2. `sequential` (`2`)
- 每個節點獨立 `malloc`
- 依建立順序串接
- locality 通常次於 contiguous

3. `random` (`3`)
- 建立完節點後隨機 permutation 串接
- 最接近 pointer-chasing 的 worst-case cache 行為

## Build

```bash
make
```

可用 targets：

- `make runner`：只建 `bin/runner`
- `make check`：只建 `bin/check`
- `make clean`：清除 `build/` 與 `bin/runner`、`bin/check`

## runner: create + algorithm benchmark loop

`runner` 會：

- 先建立一次指定型態與大小的 linked list
- linked list 的建立邏輯由 `create.c` 提供 API，不能再獨立執行
- 只在 traversal 期間啟用 `perf_event_open()` 計數器
- traversal 結束立刻停用計數器，最後才 `free` list

```bash
./bin/runner <mode> <count> <algo> <rounds> [seed]
```

參數：

- `mode`: `contiguous|1`、`sequential|2`、`random|3`
- `count`: 節點數量（`> 0`）
- `algo`: `single|single_pointer|1`、`fastslow|fast_and_slow|2`
- `rounds`: 執行輪數（`> 0`）
- `seed`: 可選，固定 random 模式用的 seed（不給就用目前時間）

輸出至少包含：

- `elapsed(sec)`
- `cache-references:u`
- `cache-misses:u`
- `cache-miss-rate`
- `L1-dcache-prefetches:u`
- `IPC`

範例：

```bash
./bin/runner contiguous 1000000 single 100
./bin/runner random 1000000 fastslow 100 12345
```

## check: 印出 linked list 串接位址

`check` 只需要 3 個參數：

```bash
./bin/check <mode> <count> [seed]
```

參數：

- `mode`: `contiguous|1`、`sequential|2`、`random|3`
- `count`: 節點數量（`> 0`）
- `seed`: 可選，只有在 `random` 模式下有意義；不給就用目前時間

`check` 會：

- 透過 `create` API 建立 linked list
- 依 `next` 串接順序印出各 node 的位址，格式為 `addressA ----> addressB`
- 最後呼叫 `destroy_list()` 釋放記憶體

範例：

```bash
./bin/check contiguous 8
./bin/check sequential 8
./bin/check random 8 42
```
