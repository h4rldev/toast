# toast

A HTTP server for high performance websites

Hand crafted with some libraries like

- [libH2O](https://h2o.examp1e.net) for HTTP server functionality
- [jansson](https://github.com/akheron/jansson) for json parsing and dumping

## Features

- Configuration through JSON \[done\]
- CLI override of configuration \[done\]
- Logging to file \[done\]
- Togglable GZIP or ~~BROTLI~~ compression \[done\]
  (BROTLI is not supported by libh2o)
- HTTPS support \[done\]
- Easy endpoint creation \[done\]

## Building

### Dependencies

- [CMake](https://cmake.org) for building libH2O
- [libH2O](https://h2o.examp1e.net) for HTTP server functionality
- [Jansson](https://github.com/akheron/jansson) for JSON parsing and dumping

### libH2O's dependencies

- [libuv](https://github.com/libuv/libuv) for libh2o's event loop
- [zlib](https://zlib.net) for libh2o's compression
- [brotli](https://github.com/google/brotli) for libh2o's compression (despite not working)
- [OpenSSL](https://www.openssl.org) for libh2o's SSL
- [picotls](https://github.com/h2o/picotls) for libh2o's SSL
- [libcap](https://git.kernel.org/pub/scm/libs/libcap/libcap.git/) for libh2o's privileges
- [wslay](https://github.com/tatsuhiro-t/wslay) for libh2o's websockets (probably not required for building)

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

### Running

```bash
./toast or ./toast -h for help
```

## License

This project is licensed under the [AGPL-3.0](./LICENSE) license.

## Contributing

Contributions are welcome!
