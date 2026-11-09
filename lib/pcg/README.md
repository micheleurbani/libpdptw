# pcg-c-basic (vendored)

Unmodified copy of `pcg_basic.c`, `pcg_basic.h` and `LICENSE.txt` (Apache 2.0)
from <https://github.com/imneme/pcg-c-basic>, pinned to commit
`bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3` (2014-10-18; upstream has no
release tags). libpdptw uses it only through `include/rng.h`, which calls the
`_r` functions with an explicit state; the global-state functions are unused.
