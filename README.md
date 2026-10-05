# Qt template

Provisioned from [`Qode-Fleet-Control/fleet-template-v1`](https://github.com/Qode-Fleet-Control/fleet-template-v1) — the fleet
lifecycle contract (`bin/`, `fleet.conf`, `compose.yaml`, deploy workflows) with a Qt 6 QHttpServer starter laid on top.

A minimal HTTP service on Qt 6's QHttpServer module (Qt 6.8 from Debian trixie), built with CMake. Routes: `GET /` (plain-text greeting), `GET /health` (`{"status":"ok"}`) and `GET /api/hello/<name>` (JSON).

## Origin

    hand-written (no official generator for a QHttpServer project) — CMakeLists.txt in the shape `qt_add_executable` + `qt_standard_project_setup()` that Qt's CMake docs teach; main.cpp follows Qt's "Simple HTTP Server" example (QHttpServer routes bound to a QTcpServer)

Qt Creator's project wizard is the only Qt project generator, and it is interactive; `qt-cmake-create` only writes a CMakeLists.txt for sources that already exist.

## Run it

### On the fleet

The fleet runs it as containers (the docker runtime): `bin/run` builds the image with
`docker compose build` and then starts it with `docker compose up` in the foreground, publishing `$PORT`.

It listens on `0.0.0.0:$PORT` (default `8080`), read from the environment when the container starts,
and serves at the root of its own hostname (`https://<hash>.<FLEET_APP_DOMAIN>/`). The health check hits `/health`.

### With docker

```sh
PORT=8080 bin/run                 # build + run through compose, Ctrl-C to stop
docker compose up --build             # the same, by hand
curl localhost:8080/health
curl localhost:8080/api/hello/fleet
```

### Without docker

```sh
# Debian/Ubuntu: sudo apt install build-essential cmake qt6-base-dev qt6-httpserver-dev qt6-websockets-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
PORT=8080 ./build/app
# or: FLEET_RUNTIME=process PORT=8080 bin/run
```

`fleet.conf` drives every script in `bin/`:

| step | docker runtime (fleet) | `FLEET_RUNTIME=process` |
|---|---|---|
| install | — | `(none)` |
| build | `docker compose build` | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j` |
| start | `docker compose up --remove-orphans` | `env PORT="$PORT" ./build/app` |

## Layout

- `CMakeLists.txt` — one executable target `app`, linked to `Qt6::Core` and `Qt6::HttpServer`.
- `src/main.cpp` — routes, then `QTcpServer::listen(QHostAddress::Any, $PORT)` + `QHttpServer::bind()`.
- `Dockerfile` — `debian:trixie` build stage, `debian:trixie-slim` runtime with only `libqt6httpserver6`, non-root user `app`.
- `compose.yaml` — service `app`, publishes `${PORT:-8080}:${PORT:-8080}`, fleet variables passed through by name.

## Deviations from stock, and why

- The listen port comes from `$PORT` at runtime (default 8080) instead of the example's fixed port, and the server binds `0.0.0.0` so the container is reachable.
- Added a `/health` route for the fleet's health check.
- The Dockerfile installs `qt6-websockets-dev` explicitly: Debian's `Qt6HttpServerConfig.cmake` requires Qt6WebSockets, but `qt6-httpserver-dev` does not depend on it, so `find_package(Qt6 COMPONENTS HttpServer)` fails without it.

## Verified

2026-10-05, Docker 29.8 on linux/amd64:

- `verify.sh <dir> 46501` (the migrate-docker-runtime skill's end-to-end check) → `run=200 restart=200 containers_after_stop=0`.
- `migrate.py audit <dir>` → `READY`.
- `docker compose up` with `PORT=46501`: `GET /` → 200 `Hello from the Qt template!`, `GET /health` → `{"status":"ok"}`.

The no-docker path (`FLEET_RUNTIME=process`) was not run on a host toolchain; it is the same CMake build the image runs.

See `docs/fleet-lifecycle.md` for the lifecycle contract.
