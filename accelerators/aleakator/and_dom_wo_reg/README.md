# and dom without registers

A verilog, DOM masked and implementation, without the registers preventing the glitches on `c0`
and `c1`.

Simulation drives 16 cycles, none leaking in value/transition without glitches, while iterating
over possible `a` and `b` values, which do not matter for verification anyways.
This implementation does not prevent glitches so each cycle leaks in value/transition with glitches.

Please note that there are no leaks in transition because the secret variables on input do not change.
