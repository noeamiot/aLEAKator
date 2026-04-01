FROM ubuntu AS aleakator_builder

RUN apt update && apt install -y unzip gawk git make python3 lld bison flex libffi-dev libfl-dev libreadline-dev pkg-config tcl-dev zlib1g-dev curl cmake libboost-program-options1.83-dev gnat && apt clean

RUN git clone --recurse-submodules https://github.com/noeamiot/yosys /src/yosys-aleakator

RUN curl -L --output /src/clang.tar.xz https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz
RUN mkdir -p /src/clang/ && tar xvf /src/clang.tar.xz --strip-components=1 -C /src/clang && rm /src/clang.tar.xz

# Install custom yosys
WORKDIR /src/yosys-aleakator
RUN make config-clang && make CC=/src/clang/bin/clang CXX=/src/clang/bin/clang++ -j6 && make install
RUN curl -L --output /src/sv2v.zip https://github.com/zachjs/sv2v/releases/download/v0.0.13/sv2v-Linux.zip && unzip /src/sv2v.zip -d /src/sv2v/ && cp /src/sv2v/sv2v-Linux/sv2v /bin/ && rm -r /src/sv2v /src/sv2v.zip

# Install ghdl from releases
RUN curl -L --output /src/ghdl.tar.gz "https://github.com/ghdl/ghdl/releases/download/v6.0.0/ghdl-llvm-6.0.0-ubuntu24.04-x86_64.tar.gz"
RUN mkdir -p /src/ghdl && tar xvf /src/ghdl.tar.gz --strip-components=1 -C /src/ghdl

# Install ghdl plugin
RUN git clone https://github.com/ghdl/ghdl-yosys-plugin /src/ghdl-yosys-plugin
WORKDIR /src/ghdl-yosys-plugin
# To ease reproduction, stay on a fixed commit
RUN git checkout 07a30ed39fb6a078f1bf7e9e88ce9ed712380ec2
RUN make GHDL=/src/ghdl/bin/ghdl && make install
ENV GHDL_PREFIX=/src/ghdl/lib/ghdl/

# Install verifmsi
RUN git clone https://github.com/quentin-meunier/verif_msi_pp /src/verif_msi_pp
WORKDIR /src/verif_msi_pp
RUN git checkout 0122ddb07d4c94b413ade96cf5d43795aaf38b45
RUN make -j

# Compile aleakator (copy local version instead of cloning it)
ADD . /src/aleakator
# Exploit build cache, very much appreciated to developpement build
RUN --mount=type=cache,target=/src/aleakator/build cd /src/aleakator/build && cmake -DCMAKE_BUILD_TYPE=Release -DCLANG_PATH=/src/clang/ -DVERIFMSI_PATH=/src/verif_msi_pp/ .. && make -j6 && cp -ra . /src/res_aleakator/
WORKDIR /src/res_aleakator/

# Second stage: minimal runtime image
FROM scratch
COPY --from=aleakator_builder /src/res_aleakator/ /

ENTRYPOINT ["echo", "Usage: start this container interactively with bash as entrypoint"]
