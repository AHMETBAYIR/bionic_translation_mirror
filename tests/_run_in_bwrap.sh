#!/bin/sh

case "$(uname -m)" in
	aarch64) ARCH="arm64" ;;
	armv*) ARCH="arm" ;;
	i386) ARCH="x86" ;;
	i686) ARCH="x86" ;;
	x86) ARCH="x86" ;;
	i86pc) ARCH="x86_64" ;;
	x86_64) ARCH="x86_64" ;;
	*) echo "error: can't deternmine arch from uname -m" && 0 ;;
esac

if [ $# -lt 2 ]; then
	echo "usage: _run_in_bwrap.sh <path-to-test_runner> <path-to-CtsBionicTestCases> [<gtest-options>]"
	exit
fi

# define BUILDDIR_PATH if there are non-system dependencies
if ! [ -z ${BUILDDIR_PATH} ]; then
	BUILDDIR_PATH=$(realpath ${BUILDDIR_PATH})
	BIND_BUILDDIR_PATH="--bind ${BUILDDIR_PATH} ${BUILDDIR_PATH}"
fi

# allow using an env to run in gdb
if ! [ -z ${GDB} ]; then
	GDB_PREFIX="gdb --args "
fi

TEST_RUNNER_PATH=$(realpath $1)
shift
TEST_CASES_PATH=$(realpath $1)
shift

# by binding the CtsBionicTestCases executable next to our runner, we sidestep issues with
# dirname(readlink(/proc/self/exe)) returning TEST_RUNNER_DIR no matter where CtsBionicTestCases
# is located
TEST_RUNNER_DIR=$(dirname ${TEST_RUNNER_PATH})

# for 64bit architectures, check if the runner is ELFCLASS32 (5th byte is 0x01)
# and if so, assume 32bit multilib is being used
case ${ARCH} in
	x86_64) test $(od -An -t x1 -j 4 -N 1 ${TEST_RUNNER_PATH}) -eq 1 && ARCH="x86" ;;
	arm64) test $(od -An -t x1 -j 4 -N 1 ${TEST_RUNNER_PATH}) -eq 1 && ARCH="arm" ;;
esac

LIBRARY_PATHS="${TEST_RUNNER_DIR}/":"${TEST_RUNNER_DIR}/bionic-loader-test-libs"

LD_LIBRARY_PATH=${LIBRARY_PATHS} BIONIC_LD_LIBRARY_PATH=${LIBRARY_PATHS} LINKER_DIE_AT_RUNTIME= \
 bwrap --bind /bin /bin \
       --dev-bind /dev /dev \
       --bind /etc /etc \
       --bind /home /home \
       --bind /lib /lib \
       --bind-try /lib64 /lib64 \
       --bind /proc /proc \
       --bind /run /run \
       --bind /sys /sys \
       --bind /tmp /tmp \
       --bind /usr /usr \
       --bind /var /var \
       ${BIND_BUILDDIR_PATH} \
       --bind "${TEST_CASES_PATH}/${ARCH}/CtsBionicTestCases" "${TEST_RUNNER_DIR}/CtsBionicTestCases" \
       --bind "${TEST_CASES_PATH}/${ARCH}/bionic-loader-test-libs" "${TEST_RUNNER_DIR}/bionic-loader-test-libs" \
       --bind "${TEST_RUNNER_PATH}" "${TEST_RUNNER_PATH}" \
       --tmpfs /data/local/tmp \
       --unshare-pid --unshare-user \
        ${GDB_PREFIX}"${TEST_RUNNER_PATH}" "${TEST_RUNNER_DIR}/CtsBionicTestCases" $@
