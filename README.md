# aLEAKator: HDL Mixed-Domain Simulation for Masked Hardware & Software Formal Verification

aLEAKator is a formal verification tool for verifying the security of masked software running on
hardware as well as masked hardware implementations. It establishes the needed verifications and
eventual optimisations for a given security property provided by the user.

Verification and symbolic expression generation are performed using [VerifMSI](https://github.com/quentin-meunier/verifmsi).
The C++ implementation is used and should be released to the public soon. For now, a precompiled version
is available [on github](https://github.com/noeamiot/verif_msi_pp-prerelease).

To use this repository, you MUST clone the project with the submodules:
`git clone git@github.com:noeamiot/aleakator --recurse-submodules`

If you are allowed to access the cortex_m3 and cortex_m4 cores, you can add them as submodules
with the commands at the root of the project:
```
git submodule add git-url:repo-cortex-m4 submodules/cortex_m4
git submodule add git-url:repo-cortex-m3 submodules/cortex_m3
```

You need a custom version of yosys, in particular the custom cxxrtl backend behind aLEAKator.

It is available [on github](https://github.com/noeamiot/yosys).

Documentation is WIP.

# Stability

Stability is always computed but can optionally not be considered. For this, the flag
`USE_STABILITY` in combination with a leak library that disables the partialStabilize and
regStabilize functions allows for verification with glitches but without stability. This mode is
less tested but should be complete for the leakage model (meaning the over-approximations and 
optimisations performed by the manager is still complete).

# How to Build

Set the path for the clang 17 toolchain, then:
```
mkdir build
cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

You can also build in Debug mode, for the debug symbols to be enabled. Please note that the Release
build type disables assertions, thus throughout testing is needed before running in this mode.

## Specific Build Options

When leaksets are not to be considered, one can disable the functions from the `leaks` namespace.
Note that they become no-op but are still called. This choice has been made to allow for not fully
recompiling built cxxrtl models to disable leaksets, only re-linking against the lss with the
disable flag does not change the interfaces.

To disable the leaksets, in the build folder, run:
```
cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_LEAKSETS=OFF ..
```

The default value is `ON`.

# To Debug Concretisations

Compile using `DEBUG_CONCRETIZATION` then execute as follows: 
`gdb -iex 'set debuginfod enabled off' --command=../../../gdb_commands --args ./run_cortex_m3 aes_herbst`
