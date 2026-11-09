# libpdptw

Solution building blocks for the **Pickup and Delivery Problem with Time
Windows (PDPTW)**, extracted from [vrp](https://github.com/micheleurbani/vrp)
so they can be reused outside that project's solver CLI. Search algorithms
are not part of this library; they live in the projects that build on it.

One static C99 library, no dependencies beyond libm:

- **`vrp_core`** — the `PDPTW` instance, `Route`, and `Solution` data
  structures, the Modified Solomon insertion heuristic, and the PD operators
  (`pd_shift`, `pd_exchange`, `pd_rearrange`, `insert_job`, `remove_job`, …).
  Also:
  - `rng.h`: a seeded PCG32 generator (`rng_seed`, `rng_next`, `rng_below`).
    Its state is passed explicitly, so there is no global generator and no
    `rand()`. PCG is vendored unmodified in `lib/pcg` (see its README for the
    pinned commit).
  - `construction.h`: `random_construction`, which inserts jobs in a random
    order, each at its smallest-Δdistance position, and falls back to the
    Solomon heuristic when the fleet is too small.
  - `objectives.h`: `evaluate_objectives`, which returns the objective
    vector (number of vehicles, total distance, makespan, total waiting
    time).

`problem.h` and `solution.h` carry `extern "C"` guards for C++ callers;
`route.h` and `utils.h` do not, and `solution.h` includes `route.h` outside
its guard, so C++ code should wrap `route.h` in an `extern "C"` block before
including the others.

`docs/` holds notes on the two papers the building blocks come from: Li & Lim
(2003), *A Metaheuristic for the Pickup and Delivery Problem with Time
Windows* (`docs/li-lim-2003-pdptw.md`), and Ropke & Pisinger (2006), *An
Adaptive Large Neighborhood Search Heuristic for the Pickup and Delivery
Problem with Time Windows* (`docs/ropke2006.md`).

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
    GIT_TAG        v0.2.0
)
FetchContent_MakeAvailable(libpdptw)

target_link_libraries(your_target PRIVATE vrp_core)
```

Or after `cmake --install`, via `find_package`:

```cmake
find_package(libpdptw REQUIRED)
target_link_libraries(your_target PRIVATE libpdptw::vrp_core)
```

### API sketch

```c
#include "problem.h"
#include "solution.h"

PDPTW* problem = read_pdptw_from_file("instance.txt");   // Li & Lim text format,
                                                           // or build a PDPTW*
                                                           // by hand for another
                                                           // instance format
Solution* S = create_solution(problem);
insertion_heuristic(S, /*mu=*/1.0, /*alpha=*/0.5);

Solution* neighbour = copy_solution(S);   // mutate the copy with the PD
                                           // operators, then update_route and
                                           // check_route each touched route

save_solution_to(S, "out/");               // Li & Lim .sol template, named
                                           // after the instance
write_solution(neighbour, "out/1.sol");    // same format, to an exact path

free_solution(neighbour);
free_solution(S);
free_problem(problem);
```

Memory is manual throughout: `create_*`/`free_*` come in pairs
(`create_problem`/`free_problem`, `create_route`/`free_route`,
`create_solution`/`copy_solution`/`free_solution`). A copy shares the
problem and its job pairs with the original, so free the problem last.

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
  `NV ≫ Dist ≫ SD ≫ WT`. If your problem has a different objective priority,
  adapt the weight anchoring rather than just retuning the constants.

### Instance file format

`read_pdptw_from_file` reads the Li & Lim text format: first line
`M  Q  flag` (vehicles, capacity, flag), tab-separated; each subsequent line
`id  x  y  demand  earliest  latest  service  pickup_pair  delivery_pair`.
For a pickup node `delivery_pair` is its paired delivery (`pickup_pair = 0`);
for a delivery node the reverse. Node `0` is the depot.

If your instances aren't in this format, skip `read_pdptw_from_file` and
populate a `PDPTW*`/`Route*`/`Solution*` directly — the data structures and
operators only care that `D`/`T`/`e`/`l`/`q`/`jobs` are populated
correctly, not how they got that way.

## Provenance

Split out of [vrp](https://github.com/micheleurbani/vrp) (commit `cc708e7`)
as a standalone library so the PDPTW data structures can be reused outside
that repo's solver CLI, benchmarking, and tuning tooling. Version 0.1.0 also
shipped the `vrp_algorithms` library (tabu search and ALNS); 0.2.0 removed it.
This repo's own git history starts at that split; see the `vrp` repo for the
prior development history of these files.
