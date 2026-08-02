FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    gcovr \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# Build & install GoogleTest from source (pinned version). Ubuntu's libgtest-dev
# package has inconsistently shipped a usable CMake config across releases, so a
# pinned source build keeps this reproducible regardless of base image version.
RUN git clone --branch v1.17.0 --depth 1 https://github.com/google/googletest.git /tmp/googletest \
    && cmake -S /tmp/googletest -B /tmp/googletest/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/googletest/build --target install -j"$(nproc)" \
    && rm -rf /tmp/googletest

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
    && cmake --build build -j"$(nproc)" --target all_tests dbc_compiler can_sandbox socket_can_tests

# Fails the image build (and therefore CI) if any test fails.
RUN build/bin/all_tests
