# and dom - second order secure

A verilog, DOM masked and implementation, with the registers preventing the glitches on `c0`, `c1`
and `c2`.

Simulation over two cycles (needed to propagate registers), secure in value/transition with/without
glitches at second order.

Please note that there are no leaks in transition because the secret variables on input do not change.
