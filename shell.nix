{
  pkgs ? import <nixpkgs> {},
  lib ? pkgs.lib,
}:
with pkgs;
  mkShell {
    buildInputs = [
      cmake
      jansson
      zlib
      libuv
      openssl
      pkg-config
      brotli
      wslay
      libcap
    ];

    packages = [
      file
      bear
      gcc
      clang-tools
      valgrind
      just
    ];

    shellHook = ''
      mkdir -p ./tmp
      export TMPDIR=$(realpath ./tmp)
    '';
  }
