# and isw - second order order

A verilog, ISW masked and implementation.

Single cycle simulation, secure leaking in value/transition without glitches at second order.
This implementation does not prevent glitches so each cycle leaks in value/transition with glitches.

Please note that there are no leaks in transition because the secret variables on input do not change.
