# How to develop using toast

## Building

### Dependencies

- CMake
- libH2O
- Jansson

### libH2O's dependencies

- libuv
- zlib
- brotli
- OpenSSL
- picotls
- libcap
- wslay (probably not required for building but it's there if you want to use websockets)

### Building libH2O

```bash
just build-libh2o
```

### Building toast

```bash
just build
```

### Building all

```bash
just build-all
```

## Running

```bash
./toast or ./toast -h for help
```

## Generating a local compilation database for clangd

```bash
just generate-compilation-database
```

## Developing guide

### Adding a new endpoint

- Explore [api.c](../src/toast/api.c) and [api.h](../include/api.h) to see how you make endpoints, there should be examples there.
- Add your endpoint to [api.c](../src/toast/api.c) and [api.h](../include/api.h).
- Add your endpoint to main in [main.c](../src/main.c) after `#ifdef API_H_IMPLEMENTATION`.

### Adding a new configuration option

- Add your option to [config.c](../src/toast/config.c) and [config.h](../include/config.h).
- Be sure that it parses properly, writes properly and is used where it's needed.

### Write your own code

Feel free to clone this and use it as a good base for your backend, it has a restrictive license but you are still free to use it as a base for your own projects as long as it's license is respected.
It should have no issues with using database connections, websockets and so on, but this project by itself is not meant for websockets.
If you want to use websockets anyway, check out the websocket example in libh2o's [examples](https://github.com/h2o/h2o/tree/master/examples/libh2o).
