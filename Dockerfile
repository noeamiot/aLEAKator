FROM ubuntu

RUN apt update && apt install -y unzip gawk git make python3 lld bison flex libffi-dev libfl-dev libreadline-dev pkg-config tcl-dev zlib1g-dev curl cmake libboost-program-options1.83-dev gnat && apt clean

RUN git clone --recurse-submodules https://github.com/noeamiot/yosys /src/yosys-aleakator

RUN curl -L --output /src/clang.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz
RUN mkdir -p /src/clang/ && tar xvf /src/clang.tar.xz --strip-components=1 -C /src/clang && rm /src/clang.tar.xz

# Install custom yosys
WORKDIR /src/yosys-aleakator
RUN make config-clang && make CC=/src/clang/bin/clang CXX=/src/clang/bin/clang++ -j6 && make install
RUN curl -L --output /src/sv2v.zip https://github.com/zachjs/sv2v/releases/download/v0.0.13/sv2v-Linux.zip && unzip /src/sv2v.zip -d /src/sv2v/ && cp /src/sv2v/sv2v-Linux/sv2v /bin/ && rm -r /src/sv2v /src/sv2v.zip

# Install ghdl from releases
RUN curl -L --output /src/ghdl.tar.gz "https://github.com/ghdl/ghdl/releases/download/v5.1.1/ghdl-llvm-5.1.1-ubuntu24.04-x86_64.tar.gz"
RUN mkdir -p /src/ghdl && tar xvf /src/ghdl.tar.gz --strip-components=1 -C /src/ghdl

# Install ghdl plugin
RUN git clone https://github.com/ghdl/ghdl-yosys-plugin /src/ghdl-yosys-plugin
WORKDIR /src/ghdl-yosys-plugin
RUN make GHDL=/src/ghdl/bin/ghdl && make install
ENV GHDL_PREFIX=/src/ghdl/lib/ghdl/

# Install prebuilt verifmsi
RUN git clone https://github.com/noeamiot/verif_msi_pp-prerelease /src/verif_msi_pp

# Compile aleakator (copy local version instead of cloning it)
ADD . /src/aleakator
#RUN git clone --recurse-submodules https://github.com/noeamiot/aLEAKator /src/aleakator
RUN rm -rf /src/aleakator/build && mkdir -p /src/aleakator/build
WORKDIR /src/aleakator/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCLANG_PATH=/src/clang/ -DVERIFMSI_PATH=/src/verif_msi_pp/ .. && make -j2

ENTRYPOINT echo "Usage: start this container interactively with bash as entrypoint"
