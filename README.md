# Fast/Slow Pointer Locality Lab

## Node Layout Modes

1. `sequential` (`1`)
- 每個節點獨立 `malloc`
- 依建立順序串接
- locality 較好，分配器常把節點放在相近區域

2. `random` (`2`)
- 建立完節點後隨機 permutation 串接
- 最接近 pointer-chasing 的 worst-case cache 行為

## Build

```bash
make
```

## runner: create + algorithm benchmark loop

`runner` 會：

- 先建立一次指定型態與大小的 linked list
- linked list 建立邏輯由 `create.c` 提供 API，只能透過 `runner`、`traverse`、`check` 使用
- 只在 traversal 期間啟用 `perf_event_open()` 計數器
- traversal 結束立刻停用計數器，最後才 `free` list

```bash
./bin/runner <mode> <count> <algo> <rounds> [seed]
```

參數：

- `mode`: `sequential|1`、`random|2`
- `count`: 節點數量（`> 0`）
- `algo`: `single|single_pointer|1`、`fastslow|fast_and_slow|2`
- `rounds`: 執行輪數（`> 0`）
- `seed`: 可選，固定 random 模式用的 seed（不給就用目前時間）

輸出至少包含：

- `elapsed(sec)`
- `cache-references:u`
- `cache-misses:u`
- `cache-miss-rate`
- `IPC`

範例：

```bash
./bin/runner sequential 1000000 single 100
./bin/runner random 1000000 fastslow 100 12345
```

## traverse: create + traversal only

`traverse` 的參數和 `runner` 相同，但不做任何 `perf_event_open()` 採樣。

```bash
./bin/traverse <mode> <count> <algo> <rounds> [seed]
```

它會：

- 建立 linked list
- 執行指定演算法的 traversal
- 輸出 middle node 與 `elapsed(sec)`
- 最後釋放 linked list

`mode` 使用 `sequential|1` 或 `random|2`。

## check: inspect node layout

`check` 用來檢視整條 linked list 的節點位址、值與 `next` 指標。

```bash
./bin/check <mode> <count> [seed]
```

參數：

- `mode`: `sequential|1`、`random|2`
- `count`: 節點數量（`> 0`）
- `seed`: 可選，只有 `random` 模式下有意義

輸出格式：

```text
node i = (address), val = ..., next = ...
```
