#!/bin/bash

# This file is part of Xfce (https://gitlab.xfce.org).
#
# Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.



# This script tests all possible build configurations using a brute-force search.
# It runs 2**7=128 build configurations in parallel and consequently requires
# a lot of memory, a lot of storage, and benefits from a high number of CPU cores.
#
# NOTE: It is recommended to AVOID running this script on a machine with
#       less than 16 GB of memory, or with less than 8 logical CPUs,
#       or with less than 10 GB of free space in the filesystem.
#
# On a 16 thread CPU (8 physical cores, 16 logical CPUs) with 32 GB RAM
# and an SSD storage device, this script completes in about 6 minutes.
#
# Upon completion of this script:
#  - The directory where this script is located contains log files of all builds.
#    Each log file starts with a line describing the selected options.
#  - Temporary directories of successfully completed builds are deleted.
#  - Temporary directories of failed builds are left on the filesystem.
#
# External dependencies:
#  - C/C++ compilers: clang and/or gcc
#  - Python
#  - rsync



set -e
shopt -s nullglob

LOG_BASE="$(basename $0 .sh)".log

if [ $# == 0 ]; then
	echo
	echo "Usage: $(basename "$0") <command>"
	echo
	echo "Commands:"
	echo
	echo "  clean  Remove all temporary directories and log files"
	echo "  run    Compile all possible build configurations"
	echo
elif [ $# == 1 -a "$1" == run ]; then
	if ! which grep &> /dev/null; then echo "error: failed to locate grep"; exit 1; fi
	if ! which python &> /dev/null; then echo "error: failed to locate Python"; exit 1; fi
	if ! which rsync &> /dev/null; then echo "error: failed to locate rsync"; exit 1; fi

	cd "$(dirname "$(realpath "$0")")"

	declare -a OPTIONS=(hddtemp libsensors netcat notification procacpi sysfsacpi xnvctrl)
	NUM_COMBINATIONS=$[1 << ${#OPTIONS[@]}]

	NPROC=$(nproc)
	MIN_CPUS=8
	if [ $NPROC -lt $MIN_CPUS ]; then
		echo
		echo "error: not enough logical CPUs to run this script"
		echo
		echo "Available number of logical CPUs: $NPROC"
		echo "Recommended minimum number of logical CPUs: $MIN_CPUS"
		echo
		echo "If you still want to run this script,"
		echo "please change MIN_CPUS in $0."
		echo
		exit 1
	fi

	MEM_AVAIL=$(python -c "import psutil; print(psutil.virtual_memory().available)")
	MEM_MIN1=100  # Minimum memory required to run 1 combination (unit: MiB)
	MEM_REQ=$[$NUM_COMBINATIONS * ($MEM_MIN1 << 20)]
	if [ $MEM_AVAIL -lt $MEM_REQ ]; then
		MEM_TOTAL=$(python -c "import psutil; print(psutil.virtual_memory().total)")
		echo
		echo "error: not enough memory to run this script"
		echo
		echo "Currently available memory: $[$MEM_AVAIL >> 20] MiB"
		echo "Required available memory: $[$MEM_REQ >> 20] MiB"
		echo "Total installed memory: $[$MEM_TOTAL >> 20] MiB"
		echo
		exit 1
	fi

	STORAGE_AVAIL=$(python -c "import psutil; print(psutil.disk_usage('.').free)")
	STORAGE_MIN1=20  # Minimum storage space required to run 1 combination (unit: MiB)
	STORAGE_REQ=$[$NUM_COMBINATIONS * ($STORAGE_MIN1 << 20)]
	if [ $STORAGE_AVAIL -lt $STORAGE_REQ ]; then
		echo
		echo "error: not enough space to run this script"
		echo
		echo "Currently available space: $[$STORAGE_AVAIL >> 20] MiB"
		echo "Required available space: $[$STORAGE_REQ >> 20] MiB"
		echo
		exit 1
	fi

	# Brute-force search through all build combinations/permutations
	exec 3>&1
	declare -i I=0
	for CC in clang gcc; do
		if ! which $CC &> /dev/null; then
			echo "warning: failed to locate compiler $CC"
			continue
		fi

		COMMON_FLAGS="-g0 -O0 -Wall -Wpointer-arith -Wsign-compare"
		CFLAGS=""
		if [ $CC == clang ]; then
			CXX=clang++
		else
			CXX=g++
			CFLAGS="${CFLAGS} -Wold-style-declaration"
		fi
		CFLAGS="${COMMON_FLAGS} ${CFLAGS} -Wdeclaration-after-statement -Wold-style-definition"
		CXXFLAGS="${COMMON_FLAGS}"

		# Run multiple combinations in parallel
		for((J = 0; J < $[ 1 << ${#OPTIONS[@]} ]; J++)); do
			if [ $[ $J & (1 << 0) ] == 0 ]; then A0=--disable-${OPTIONS[0]}; else A0=--enable-${OPTIONS[0]}; fi
			if [ $[ $J & (1 << 1) ] == 0 ]; then A1=--disable-${OPTIONS[1]}; else A1=--enable-${OPTIONS[1]}; fi
			if [ $[ $J & (1 << 2) ] == 0 ]; then A2=--disable-${OPTIONS[2]}; else A2=--enable-${OPTIONS[2]}; fi
			if [ $[ $J & (1 << 3) ] == 0 ]; then A3=--disable-${OPTIONS[3]}; else A3=--enable-${OPTIONS[3]}; fi
			if [ $[ $J & (1 << 4) ] == 0 ]; then A4=--disable-${OPTIONS[4]}; else A4=--enable-${OPTIONS[4]}; fi
			if [ $[ $J & (1 << 5) ] == 0 ]; then A5=--disable-${OPTIONS[5]}; else A5=--enable-${OPTIONS[5]}; fi
			if [ $[ $J & (1 << 6) ] == 0 ]; then A6=--disable-${OPTIONS[6]}; else A6=--enable-${OPTIONS[6]}; fi
			rm -f "${LOG_BASE}${I}"
			nice -n19 $0 __build__ "$CC" "$CXX" "$CFLAGS" "$CXXFLAGS" $A0 $A1 $A2 $A3 $A4 $A5 $A6 &> "${LOG_BASE}${I}" &
			I+=1
		done
		wait
	done

	# Print errors found in logs
	if [ -f "${LOG_BASE}0" ]; then
		for LOG_FILE in $(ls ${LOG_BASE}* | sort -n); do
			grep --color=auto "error" "$LOG_FILE" || true
		done
	else
		echo "error: no combination of build options was run"
		exit 1
	fi
elif [ $# == 1 -a "$1" == clean ]; then
	for TDIR in tmpdir.*; do
		rm -rf "$TDIR"
	done
	rm -rf ${LOG_BASE}*
elif [ $# -gt 1 -a "$1" == __build__ ]; then
	shift
	export CC="$1"; shift
	export CXX="$1"; shift
	export CFLAGS="$1"; shift
	export CXXFLAGS="$1"; shift

	COMBINATION="[$CC $CXX $@]"
	echo "[$(date --rfc-3339=seconds)]  Building $COMBINATION ..." >&3
	echo "$COMBINATION"
	echo
	TDIR="$(mktemp --directory tmpdir.XXXXXXXXXX)"
	rsync --archive --exclude "tmpdir.*" --exclude "*.log*" --quiet . "$TDIR"
	cd "$TDIR"
	export MAKEFLAGS=-j1
	if [ -f Makefile ]; then
		make -j1 distclean
	fi
	./configure "$@"
	make -j1
	cd ..
	rm -rf "$TDIR"
	echo "[$(date --rfc-3339=seconds)]  Done $COMBINATION" >&3
else
	echo "error: invalid arguments"
	exit 1
fi
