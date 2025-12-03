# Ibex RISC-V Core

The core is available on [github](https://github.com/lowRISC/ibex).
The version used as submodule in this repository is a fork, containing few
configuration modifications, available (on github as well)[https://github.com/noeamiot/ibex/].

Configuration modifications are mainly small updates to permit the core usage with cxxrtl.

## Synthesis steps

The core SystemVerilog source files are first read with `prepare.sh` to be converted to Verilog
and stored in the build repository.

> **_NOTE:_** There is an exception for the register-file, that is interpreted directly in SystemVerilog, to
prevent a bug in which it was represented as a shifter in yosys.

The files are then read by yosys using the synthesis script `synth.ys` to provide the cxxrtl model
and topology file of the circuit. The cxxrtl model is patched for convenience after this step and
then compiled.

## Programs compilation

The programs present in the `programs` directory are compiled automatically and are available
when runing `./ibex {program}`. They are first crossed-compiled using `clang`, their sections are
extracted and reformated as cxxrtl functions to inject at runtime their values in the simulated rom.

The symbol mapping is also made available in a similar way.
