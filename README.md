# toast

A HTTP server for high performance websites

Hand crafted with some libraries like

- [libH2O](https://h2o.examp1e.net) for HTTP server functionality
- [jansson](https://github.com/akheron/jansson) for json parsing and dumping

## Features

- [x] Configuration through JSON
- [x] CLI override of configuration
- [x] Logging to file
- [x] Togglable GZIP or ~~BROTLI~~ compression
  (BROTLI is not supported by libh2o)
- [x] HTTPS support
- [x] Easy endpoint creation
- [ ]  Custom error pages (Maintaining this project will be paused 'til I can figure out how to do this)

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

(Done automatically when missing)

```bash
just ensure_h2o
```

### Building toast

```bash
just build
```

### Running

```bash
./bin/toast or ./bin/toast -h for help
```

## License

This project is licensed under the [BSD 3-Clause](./LICENSE) license.

## Contributing

Contributions are welcome!
