# Transit Peek

Peek transit arrivals — a small KDE Plasma applet showing upcoming transit arrivals using GTFS Realtime.

## Features
- Shows upcoming arrival predictions for configured stops
- Lightweight C++/Qt (QML) Plasma applet
- Uses GTFS-realtime protobuf feed parsing

## Requirements
- CMake >= 3.22
- A C++20 toolchain (g++/clang with C++20 support)
- Qt 6 (Quick, Core, Qml, Sql) >= 6.10
- KDE Frameworks 6 (I18n, Config, KIO, Archive, KCMUtils) >= 6.22
- KDE Plasma 6.6 (target)
- Network access for live GTFS-realtime feeds

Notes: The build is configured to fetch and build a bundled Protobuf toolchain automatically. Building the bundled protobuf is required before the applet can be linked.

## Building
1. Create a build directory and run CMake:

```
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

2. Build the bundled Protobuf target first (CMake will error at configure-time if the bundled protobuf isn't installed):

```
cmake --build . --target transit_peek_protobuf -- -j
```

3. Build the applet:

```
cmake --build . -- -j
```

## Install
To install for the current user (adjust `--prefix` as desired):

```
cmake --install build --prefix ~/.local
```

After installation the applet should be available in Plasma's Add Widgets dialog as "Transit Peek".

## Development
- Sources live in `src/` and QML UI is under `src/qml/`.
- The GTFS realtime schema used is `src/gtfs-realtime.proto` (Apache-2.0).
- The project bundles `protobuf` at configure/build time; other third-party code (CSV parser) is fetched via CMake's FetchContent.

If you want to iterate quickly on QML only, you can copy the `qml/` files into a local Plasma development directory or run a local plasmoid runner (if available in your distro).

## Files of interest
- `src/` — C++ sources and QML files
- `src/gtfs-realtime.proto` — GTFS realtime schema
- `src/metadata.json` — Plasma applet metadata (name, author, license)

## Licensing
The applet's metadata currently states "All rights reserved" (see `src/metadata.json`). The bundled GTFS proto file is licensed under the Apache License 2.0. The build may fetch other third-party components; review their licenses in the build output and the `_deps/` directory.

## Contributing
Issues and pull requests are welcome. For build problems, include CMake output and platform details.

---
Author: Nagata Aptana (see `src/metadata.json`)