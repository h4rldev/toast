#!/usr/bin/env bash

# toast: build.sh
#
# written with this inspirational message in mind: Why tf would we use make
# edited from cav, FLSC-Portfolio, and originally from exMeteo.
# Relicensed under the AGPL-3.0 License

# toast
# Copyright (C) 2025  enstore.cloud
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
# details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

shopt -s nullglob # remove words if not found
# set -x            # show cmds
set -e # fail globally

__NAME__="toast: build.sh"
__AUTHOR__="h4rl"
__DESCRIPTION__="Compiles and links the toast into an executable"
__LICENSE__="AGPL-3.0"
__VERSION__="0.1.0"

BUILD="$(pwd)/build"
OUT="${BUILD}/out"
BIN="${BUILD}/bin"

# name projs by their directories
# and specify type :3

PROJ="toast"

SRC="$(pwd)/src"
LIB="$(pwd)/lib"
DIR="${SRC}/${PROJ}"

LIB_INCLUDE_DIR="${LIB}/include"
INCLUDE_DIR="$(pwd)/include"

CC="gcc"
CFLAGS="-O2 -Wall -Wextra -pedantic"

LINKER_PATHS="-L${LIB}"
LINKER_FLAGS="-ljansson -lh2o -lssl -lcrypto -lz -luv -lm -lbrotlidec -lbrotlienc"

for arg in "$@"; do
	case ${arg} in
	"--debug" | "-d")
		CFLAGS="-g ${CFLAGS}"
		echo -e "! Debug mode enabled"
		;;
	esac
done

handle_failure() {
	local MESSAGE="${1}"

	if [[ -z ${MESSAGE} ]]; then
		echo "No message provided, are you using this right?"
		exit 1
	fi

	echo "! ${MESSAGE}"
	exit 1
}

print_help() {
	cat <<EOF
${__NAME__} v${__VERSION__}
Licensed under: ${__LICENSE__}

USAGE: ${0} {FLAGS} {EXECUTABLE_NAME}
-c | --compile
Compiles toast into an executable.

-l | --link
Links all the files in ${OUT}/ to an executable with {EXECUTABLE_NAME}

-hh2o | --get-h2o-headers
Gets the h2o headers and puts them in ${LIB}/include

-ih2o | --install-h2o
Installs the h2o library and headers to ${LIB}

-cd | --compilation-database
Generates a compilation database for the project using bear

-d | --delete | --clean
Cleans all files in ${BIN}/ & ${OUT}/

-vg | --delete-cores | --delete-vgcores | --clean-vgcores | --clean-cores
Cleans all vgcore files generated by valgrind in the repo.

Made with <3 by ${__AUTHOR__}
EOF
}

get_h2o_headers() {
	if ! [[ -d ./h2o ]]; then
		git clone https://github.com/h2o/h2o.git
	fi

	mkdir -p "${LIB}/include"

	pushd h2o >/dev/null || handle_failure "Failed to pushd"

	cp -r include/* "${LIB}/include"
	pushd deps/ >/dev/null || handle_failure "Failed to pushd"
	pushd picotls >/dev/null || handle_failure "Failed to pushd"
	cp -r include/* "${LIB}/include"
	popd >/dev/null || handle_failure "Failed to popd"
	pushd quicly >/dev/null || handle_failure "Failed to pushd"
	cp -r include/* "${LIB}/include"

	popd >/dev/null || handle_failure "Failed to popd"
	popd >/dev/null || handle_failure "Failed to popd"
	popd >/dev/null || handle_failure "Failed to popd"
}

install_h2o() {
	if [[ -f ${LIB}/libh2o.a ]]; then
		return
	else
		mkdir -p "${LIB}"
	fi

	get_h2o_headers

	pushd h2o >/dev/null || handle_failure "Failed to pushd"
	mkdir -p build
	pushd build >/dev/null || handle_failure "Failed to pushd"

	cmake ..
	make libh2o
	cp ./libh2o.a "${LIB}/"
	cp ./libh2o.pc "${LIB}/"

	popd >/dev/null || handle_failure "Failed to popd"
	popd >/dev/null || handle_failure "Failed to popd"
	rm -rf h2o
}

# iterates and compiles each file on selected target, then compiles main.c, moving them to out

compile() {
	local -a C_FILES

	local -a TRIMMED_C_FILENAMES

	local TRIMMED_C_FILE
	local TRIMMED_C_FILENAME

	if [[ ! -d ${OUT} ]]; then
		mkdir "${OUT}"
	fi

	mapfile -t C_FILES < <(find "${DIR}" -type f -name "*.c")

	for ((i = 0; i < ${#C_FILES[@]}; i++)); do
		TRIMMED_C_FILE="${C_FILES[${i}]%.*}"
		TRIMMED_C_FILENAME="${TRIMMED_C_FILE##*/}"
		TRIMMED_C_FILENAMES+=("${TRIMMED_C_FILENAME}")
		echo -e "> Compiling: ${TRIMMED_C_FILENAMES[${i}]}.c.."
		${CC} ${CFLAGS} -I"${LIB_INCLUDE_DIR}" -I"${INCLUDE_DIR}" -c "${C_FILES[${i}]}" -o "${OUT}/${TRIMMED_C_FILENAME}.o"
	done

	#	if [[ ${TYPE} != "lib" ]]; then
	echo -e "> Compiling: main.c.."
	${CC} ${CFLAGS} -I"${LIB_INCLUDE_DIR}" -I"${INCLUDE_DIR}" -c "${SRC}/main.c" -o "${OUT}/main.o"

	echo -e "✓ Compiled ${TRIMMED_C_FILENAMES[*]} & main successfully"

	unset C_FILES
	unset TRIMMED_C_FILE
	unset TRIMMED_C_FILENAME
	unset TRIMMED_C_FILENAMES
}

generate_compilation_database() {
	bear -- "${0}" -c
}

# links all object files in out/ to an executable bin or a library in lib

link() {
	local -a OBJECTS
	local NAME
	local -a TRIMMED_FILES

	if [[ ! -d ${BIN} ]]; then
		mkdir "${BIN}"
	fi

	if [[ ! -d ${LIB} ]]; then
		install_h2o
	fi

	mapfile -t OBJECTS < <(find "${OUT}" -type f -name "*.o")

	TRIMMED_FILES="${OBJECTS[*]##*/}"
	pushd "${OUT}" >/dev/null || handle_failure "Failed to pushd" #|| echo "Failed to pushd" && exit 1

	if [[ -n ${1} ]]; then
		NAME="${1}"
	else
		NAME="${PROJ}"
	fi

	echo -e "> Linking: ${TRIMMED_FILES[*]}.."
	${CC} ${TRIMMED_FILES[*]} ${LINKER_PATHS} ${LINKER_FLAGS} -o "${BIN}/${NAME}"
	echo -e "✓ Linked ${TRIMMED_FILES} to ${NAME} successfully"

	popd >/dev/null || handle_failure "Failed to popd" # || echo "Failed to popd" && exit 1
}

# removes dangling object files that shouldn't be there, used to be required, not that much as of lately though.

clean_dangling() {
	local DIR1 # C-DIR 1
	local DIR2 # C-DIR 2 / OUT-DIR
	local DIR3 # OUT-DIR

	local OUTPUTDIR

	local LINE

	DIR1=${1}
	DIR2=${2}
	DIR3=${3}

	# Extract .c and .o filenames without paths and store them in temporary files

	if [[ -z ${DIR3} ]]; then
		find "$DIR1" -name "*.c" -exec basename {} \; >"c_files.txt"
		find "$DIR2" -name "*.o" -exec basename {} \; >>"o_files.txt"
		OUTPUTDIR="${DIR2}"
	else
		find "$DIR1" -name "*.c" -exec basename {} \; >"c_files.txt"
		find "$DIR2" -name "*.c" -exec basename {} \; >>"c_files.txt"
		find "$DIR3" -name "*.o" -exec basename {} \; >"o_files.txt"
		OUTPUTDIR="${DIR3}"
	fi

	echo "main.c" >>"c_files.txt"

	# Compare the lists and find .o files in dir2 that do not have a corresponding .c file in dir1
	grep -Fxv -f <(sed 's/\.c$/.o/' "c_files.txt") "o_files.txt" >"extra_o_files.txt" || true

	# Remove extra .o files from dir2
	while read -r LINE; do
		rm -f "${OUTPUTDIR}/${LINE}"
	done <"extra_o_files.txt"

	# Cleanup
	rm c_files.txt o_files.txt extra_o_files.txt
}

# cleans both /out && /bin

clean() {
	local CLEAN
	local LOCALOUT
	local LOCALBIN
	local LOG

	LOCALOUT=${1}
	LOCALBIN=${2}
	CONFIRMATION=${3}
	LOG=${4:true}

	if ${LOG}; then
		echo -e "! Cleaning ${LOCALOUT} & ${LOCALBIN}."
	fi
	if ! [[ ${CONFIRMATION} =~ [yY] ]]; then
		echo -ne "! You sure you want to proceed? [y/N]: "
		read -r CLEAN
	else
		CLEAN="y"
	fi
	if [[ ${CLEAN} =~ [Yy] ]]; then
		rm -fr "${LOCALOUT:?}/*"
		rm -fr "${LOCALBIN:?}/*"
		if ${LOG}; then
			echo -e "✓ Cleaned ${LOCALOUT} & ${LOCALBIN} successfully."
		fi
	else
		echo -e "✓ Cancelled."
	fi
}

clear_vgcores() {
	local -a FILES
	local FILE

	FILES=$(find "${PWD}" -type f -name "vgcore.*")

	if [[ -z ${FILES} ]]; then
		echo -e "! No files found :("
		return
	fi

	for FILE in ${FILES}; do
		if [[ -f ${FILE} ]]; then
			rm "${FILE}"
			echo -e "✓ Deleted file \"${FILE}\""
		else
			echo -e "! File does not exist :("
		fi
	done
}

case $1 in
"-c" | "--compile")
	compile
	;;
"-l" | "--link")
	link "${2}"
	;;
"-hh2o" | "--get-h2o-headers")
	get_h2o_headers
	;;
"-ih2o" | "--install-h2o")
	install_h2o
	;;
"-d" | "--delete" | "--clean")
	clean "${OUT}" "${BIN}" "n"
	;;
"-vg" | "--delete-cores" | "--delete-vgcores" | "--clean-vgcores" | "--clean-cores")
	clear_vgcores
	;;
"-cd" | "--compilation-database")
	generate_compilation_database
	;;
"--help" | "-h" | "-?" | *)
	print_help "${@}"
	;;
esac
