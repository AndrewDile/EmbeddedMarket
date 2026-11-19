FROM debian:bookworm-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ cmake ninja-build ca-certificates xz-utils
WORKDIR /src
COPY . .
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DEM_BUILD_TESTS=OFF -DEM_BUILD_SERVICE=ON \
    && cmake --build /build --target em_engine_service

# Runs the unit tests during the image build: docker build --target test engine
FROM build AS test
RUN apt-get install -y --no-install-recommends libgtest-dev \
    && cmake -S . -B /build-tests -G Ninja -DEM_BUILD_TESTS=ON \
    && cmake --build /build-tests \
    && ctest --test-dir /build-tests --output-on-failure

FROM debian:bookworm-slim AS service
RUN apt-get update \
    && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /build/service/em_engine_service /usr/local/bin/em_engine_service
USER nobody
EXPOSE 8081
HEALTHCHECK --interval=5s --timeout=3s --retries=10 CMD curl -fsS http://localhost:8081/health || exit 1
CMD ["em_engine_service"]
