set shell := ["bash", "-c"]
set quiet := true

src_dir := 'src'
out_dir := 'out'
bin_dir := 'bin'
lib_dir := 'lib'
include_dir := 'include'
h2o_include := lib_dir + '/include'
link_flags := '-ljansson -lh2o -lssl -lcrypto -lz -luv -lm -lbrotlidec -lbrotlienc -O2 -flto -std=c99 -fsanitize=address -g -static-libasan'
compile_flags := '-O2 -flto -std=c99 -fsanitize=address -g'

default:
    just --list

ensure_h2o:
  #!/usr/bin/env bash
  TEMP=$(mktemp -d)

  [[ -d {{ lib_dir }} ]] || mkdir -p {{ lib_dir }}
  [[ -d {{ h2o_include }} ]] || mkdir -p {{ h2o_include }}
  git clone https://github.com/h2o/h2o.git $TEMP

  [[ -f {{ h2o_include }}/h2o.h ]] || cp $TEMP/include/h2o.h {{ h2o_include }}
  [[ -f {{ h2o_include }}/picotls.h ]] || cp $TEMP/deps/picotls/include/picotls.h {{ h2o_include }}
  [[ -f {{ h2o_include }}/quicly.h ]] || cp $TEMP/deps/quicly/include/quicly.h {{ h2o_include }}

  [[ -d {{ h2o_include }}/h2o ]] || cp -r $TEMP/include/h2o {{ h2o_include }}
  [[ -d {{ h2o_include }}/picotls ]] || cp -r $TEMP/deps/picotls/include/picotls {{ h2o_include }}
  [[ -d {{ h2o_include }}/quicly ]] || cp -r $TEMP/deps/quicly/include/quicly {{ h2o_include }}

  if ! [[ -f {{ lib_dir }}/libh2o.a ]]; then
    mkdir h2o_build_temp
    pushd h2o_build_temp
    cmake $TEMP
    make libh2o
    cp libh2o.a ../{{ lib_dir }}
    cp libh2o.pc ../{{ lib_dir }}
    popd
  fi

  rm -rf h2o_build_temp
  rm -rf $TEMP


compile:
    [[ -d {{ out_dir }} ]] || mkdir -p {{ out_dir }}
    [[ -d {{ h2o_include }} ]] || just ensure_h2o
    find {{ src_dir }} -name "*.c" -exec sh -c 'gcc -c "$1" -I {{ include_dir }} -I {{ h2o_include }}  -o "{{ out_dir }}/$(basename "${1%.c}").o"' sh {} \;

link:
    [[ -d {{ bin_dir }} ]] || mkdir -p {{ bin_dir }}
    [[ -f {{ lib_dir }}/libh2o.a ]] || just ensure_h2o
    gcc {{ out_dir }}/* -L {{ lib_dir}} {{ link_flags }} -o {{ bin_dir }}/toast

build: compile link

bear:
    bear -- just compile
    sed -i 's|"/nix/store/[^"]*gcc[^"]*|\"gcc|g' compile_commands.json
