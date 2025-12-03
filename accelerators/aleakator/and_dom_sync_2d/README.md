# and dom - synchro and second order secure

A verilog, DOM masked and implementation, with the registers preventing the glitches on `c0` to `c2`.
Other registers to equilibrate branches are used (this is needed for SNI security).

Simulation over two cycles (needed to propagate registers), secure in value/transition with/without
glitches at second order.

Please note that there are no leaks in transition because the secret variables on input do not change.
