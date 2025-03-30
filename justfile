build_dir := "build"
build_bin_dir := build_dir / "bin"
build_script := build_dir / "build.sh"

@default:
  just --list

@build-libh2o:
  {{build_script}} -ih2o

@get-h2o-headers:
  {{build_script}} -hh2o

@compile extra_args="":
  {{build_script}} -c {{extra_args}}

@link extra_args="":
  {{build_script}} -l {{extra_args}}

@build-all:
  just build-libh2o
  just build
  mv {{build_bin_dir}}/toast .

@build extra_args="":
  just compile {{extra_args}}
  just link {{extra_args}}

@run:
  just build
  {{build_bin_dir}}/toast

@generate-compilation-database:
  {{build_script}} -cd
