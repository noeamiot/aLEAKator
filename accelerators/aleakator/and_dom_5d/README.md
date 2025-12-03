# and dom - fifth order secure

A verilog, DOM masked and implementation, with the registers preventing the glitches on wires
`c0` to `c5`.

Simulation over two cycles (needed to propagate registers), secure in value/transition with/without
glitches at fifth order.

Please note that there are no leaks in transition because the secret variables on input do not change.
