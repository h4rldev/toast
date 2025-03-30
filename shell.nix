{
  pkgs ? import <nixpkgs> {},
  lib ? pkgs.lib,
}:
with pkgs;
  mkShell {
    packages = [
      cmake
      jansson
      zlib
      libuv
      openssl
      pkg-config
      file
      bear
      gcc
      clang-tools
      wslay
      brotli
      libcap
      valgrind
    ];

    shellHook = ''
      mkdir -p ./tmp
      export TMPDIR=$(realpath ./tmp)
    '';
  }
