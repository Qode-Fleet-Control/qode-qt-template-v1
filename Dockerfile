# Built by .github/workflows/deploy.yml and pushed to Artifact Registry.
#
# Qt 6 QHttpServer on Debian trixie (Qt 6.8). qt6-websockets-dev is listed
# explicitly: Debian's Qt6HttpServerConfig.cmake requires Qt6WebSockets, but
# qt6-httpserver-dev does not depend on it. Multi-stage: the build stage has
# the compiler and Qt dev packages; the runtime stage carries only the shared
# Qt libraries the binary links against, and runs as a non-root user.
# The port is read from $PORT when the container starts, not at build time.
FROM debian:trixie AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build qt6-base-dev qt6-httpserver-dev qt6-websockets-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build \
 && install -D build/app /out/app

FROM debian:trixie-slim AS runtime
RUN apt-get update \
 && apt-get install -y --no-install-recommends libqt6httpserver6 \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -r -u 10001 app
WORKDIR /app
ARG BUILD_ID=""
ENV PORT=8080 BUILD_ID=$BUILD_ID
COPY --from=build /out/app /app/app
EXPOSE 8080
USER app
ENTRYPOINT ["/app/app"]
