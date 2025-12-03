# CV32E40P RISC-V Core

The core is available on [github](https://github.com/openhwgroup/cv32e40p).
The version used as submodule in this repository is a fork, containing few
configuration modifications, available (on github as well)[https://github.com/noeamiot/cv32e40p/].

Configuration modifications are mainly small updates to permit the core usage with cxxrtl and
addition of the simple benchmark from the ibex core.

## Synthesis steps

The core SystemVerilog source files are first read with `prepare.sh` to be converted to Verilog
and stored in the build repository.

The files are then read by yosys using the synthesis script `synth.ys` to provide the cxxrtl model
and topology file of the circuit. The cxxrtl model is patched for convenience after this step and
then compiled.

## Programs compilation

The programs present in the `programs` directory are compiled automatically and are available
when runing `./cv32e40p {program}`. They are first crossed-compiled using `clang`, their sections are
extracted and reformated as cxxrtl functions to inject at runtime their values in the simulated rom.

The symbol mapping is also made available in a similar way.
