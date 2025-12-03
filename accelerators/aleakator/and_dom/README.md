# and dom - first order secure

A verilog, DOM masked and implementation, with the registers preventing the glitches on `c0` and `c1`.

Simulation over two cycles (needed to propagate registers), secure in value/transition with/without
glitches.

Please note that there are no leaks in transition because the secret variables on input do not change.
