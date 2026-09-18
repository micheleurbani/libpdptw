# libpdptw

Data structures and metaheuristics for the **Pickup and Delivery Problem with
Time Windows (PDPTW)**, extracted from [vrp](https://github.com/micheleurbani/vrp)
so they can be reused outside that project's solver CLI.

Two static libraries, no dependencies beyond libm:

- **`vrp_core`** (C99) — the `PDPTW` instance, `Route`, and `Solution` data
  structures, the Modified Solomon insertion heuristic, and the PD operators
  (`pd_shift`, `pd_exchange`, `pd_rearrange`, `insert_job`, `remove_job`, …).
- **`vrp_algorithms`** (C++17, links `vrp_core`) — two metaheuristics, each a
  small public class (`run(Solution*)`, `get_name()`) with every tunable as a
  constructor default:
  - `tabu_search` — tabu-embedded simulated annealing, following Li & Lim
    (2003), *A Metaheuristic for the Pickup and Delivery Problem with Time
    Windows* (`docs/li-lim-2003-pdptw.md`).
  - `alns` — Adaptive Large Neighborhood Search, following Ropke & Pisinger
    (2006), *An Adaptive Large Neighborhood Search Heuristic for the Pickup
    and Delivery Problem with Time Windows* (`docs/ropke2006.md`).

## Build & test

```bash
cmake -B build
cmake --build build
ctest --test-dir build   # run from a directory one level below the repo root,
                          # so the tests' relative "../data/pdp_100/..." paths resolve
```

`LIBPDPTW_BUILD_TESTS` is `ON` when this is the top-level build and `OFF` when
pulled in via `add_subdirectory`/`FetchContent`, so a consumer doesn't build
this repo's own Unity test suite as part of its own.

## Using it in another project

Via `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(libpdptw
    GIT_REPOSITORY git@github.com:micheleurbani/libpdptw.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(libpdptw)

target_link_libraries(your_target PRIVATE vrp_algorithms)  # pulls in vrp_core too
```

Or after `cmake --install`, via `find_package`:

```cmake
find_package(libpdptw REQUIRED)
target_link_libraries(your_target PRIVATE libpdptw::vrp_algorithms)
```

### API sketch

```cpp
extern "C" {
#include "problem.h"
#include "solution.h"
}
#include "algorithms/tabu.hpp"   // or algorithms/alns.hpp

PDPTW* problem = read_pdptw_from_file("instance.txt");   // Li & Lim text format,
                                                           // or build a PDPTW*
                                                           // by hand for another
                                                           // instance format
Solution* initial = create_solution(problem);
insertion_heuristic(initial, /*mu=*/1.0, /*alpha=*/0.5);

tabu_search solver;                 // every tunable optional, paper defaults
Solution* best = solver.run(initial);

save_solution_to(best, "out/");     // Li & Lim .sol template

free_solution(initial);
free_solution(best);
free_problem(problem);
```

Memory is manual throughout: `create_*`/`free_*` come in pairs
(`create_problem`/`free_problem`, `create_route`/`free_route`,
`create_solution`/`copy_solution`/`free_solution`). `run()` does not take
ownership of or free its `initial` argument; its return value is owned by the
caller.

### Data model

- **`PDPTW`** (`problem.h`) — immutable instance: distance/time matrices
  `D`/`T`, demands `q`, time windows `e`/`l`, service times `s`, and `jobs`
  pairing pickup↔delivery nodes.
- **`Route`** (`route.h`) — a vehicle path plus *cached cumulative metrics*
  (`D`, `T`, `b` service-begin, `Q` load, `wait`, `slack`). These caches are
  **not** kept current by the structural ops (`pd_shift`, `insert_job`, …) —
  call `update_route` to recompute them and `check_route` to validate
  feasibility before reading any cost off a mutated route.
- **`Solution`** (`solution.h`) — array of `routes`, list of `ujobs`
  (unassigned), and aggregate costs `TC` (travel), `SD` (schedule duration),
  `WT` (waiting time).

### Cost functions

- `get_solution_cost(S, alpha, beta, gamma, lambda)` — priority-weighted sum
  `NV ≫ Dist ≫ SD ≫ WT`, driving local-search descent and incumbent
  comparison in both metaheuristics.
- `tabu_search` additionally uses an SA-only cost (`Dist + phi·WT`,
  `phi = 0.01·Dist`) for its Metropolis acceptance step; see `tabu.cpp`.

If your problem has a different objective priority (e.g. no time windows, or
a different lexicographic order), adapt the weight anchoring rather than just
retuning the constants — see the constructor doc comments in
`include/algorithms/tabu.hpp` and `include/algorithms/alns.hpp`.

### Instance file format

`read_pdptw_from_file` reads the Li & Lim text format: first line
`M  Q  flag` (vehicles, capacity, flag), tab-separated; each subsequent line
`id  x  y  demand  earliest  latest  service  pickup_pair  delivery_pair`.
For a pickup node `delivery_pair` is its paired delivery (`pickup_pair = 0`);
for a delivery node the reverse. Node `0` is the depot.

If your instances aren't in this format, skip `read_pdptw_from_file` and
populate a `PDPTW*`/`Route*`/`Solution*` directly — the data structures and
both metaheuristics only care that `D`/`T`/`e`/`l`/`q`/`jobs` are populated
correctly, not how they got that way.

## Provenance

Split out of [vrp](https://github.com/micheleurbani/vrp) (commit `cc708e7`)
as a standalone library so the PDPTW data structures and metaheuristics can
be reused outside that repo's solver CLI, benchmarking, and tuning tooling.
This repo's own git history starts at that split; see the `vrp` repo for the
prior development history of these files.
