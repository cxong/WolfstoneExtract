#!/bin/bash
# shellcheck disable=SC2155

# This script is primarily for checking and handling releases. If you are
# looking to build ECWolf then you should build manually.

# This script takes a single argument which specifies which configuration to
# use. Running with no arguments will list out config names, but you can also
# look at the bottom of this script.

# Anything that the configs write out to /results (logs, build artifacts) will
# be copied back to the hosts results directory.

# Infrastructure ---------------------------------------------------------------

# Build our clean environment if we don't have one built already
check_environment() {
	declare -n Config=$1
	shift

	declare DockerTag="${Config[dockerimage]}:${Config[dockertag]}"

	if ! docker image inspect "$DockerTag" &> /dev/null; then
		declare Dockerfile=$(mktemp -p .)
		"${Config[dockerfile]}" > "${Dockerfile}"
		docker build -t "$DockerTag" -f "$Dockerfile" . || {
			rm "$Dockerfile"
			echo 'Failed to create build environment' >&2
			return 1
		}
		rm "$Dockerfile"
	fi
	return 0
}

# Recursively build docker environments
check_environment_prereq() {
	declare ConfigName=$1
	shift

	[[ $ConfigName ]] || return 0

	declare -n Config=$ConfigName
	if [[ ${Config[prereq]} ]]; then
		check_environment_prereq "${Config[prereq]}" || return
	fi

	check_environment "$ConfigName"
}

run_config() {
	declare ConfigName=$1
	shift

	declare -n Config=$ConfigName

	check_environment_prereq "$ConfigName" || return

	declare Container
	Container=$(docker create -i -v "$(pwd):/mnt:ro" "${Config[dockerimage]}:${Config[dockertag]}" bash -s --) || return
	{
		declare -fx
		echo "\"${Config[entrypoint]}\" \"\$@\""
	} | docker start -i "$Container"
	declare Ret=$?

	# Copy out any logs or build artifacts we might be interested in
	mkdir -p "results/${ConfigName}"
	docker cp "$Container:/results/." "results/${ConfigName}"

	docker rm "$Container" > /dev/null

	return "$Ret"
}

main() {
	declare SelectedConfig=$1
	shift

	declare ConfigName

	# List out configs
	if [[ -z $SelectedConfig ]]; then
		declare -A ConfigGroups=([all]=1)
		for ConfigName in "${ConfigList[@]}"; do
			declare -n Config=$ConfigName
			ConfigGroups[${Config[type]}]=1
		done

		echo 'Config list:'
		printf '%s\n' "${ConfigList[@]}" | sort

		echo
		echo 'Config groups:'
		printf '%s\n' "${!ConfigGroups[@]}" | sort
		return 0
	fi

	declare ConfigName
	if [[ -v "$SelectedConfig""[@]" ]]; then
		# Full name
		ConfigName=$SelectedConfig
	else
		# Short name
		ConfigName=${ConfigList[$SelectedConfig]}
	fi

	# Run by type
	if [[ -z $ConfigName ]]; then
		declare -a FailedCfgs=()
		for ConfigName in "${ConfigList[@]}"; do
			declare -n Config=$ConfigName
			if [[ $SelectedConfig == "${Config[type]}" || $SelectedConfig == 'all' ]]; then
				run_config "${ConfigName}" || FailedCfgs+=("$ConfigName")
			fi
		done

		if (( ${#FailedCfgs} > 0 )); then
			echo 'Failed configs:'
			printf '%s\n' "${FailedCfgs[@]}"
			return 1
		else
			echo 'All configs passed!'
			return 0
		fi
	fi

	# Run the specific config
	run_config "${ConfigName}"
}

# Minimum supported configuration ----------------------------------------------

dockerfile_ubuntu_minimum() {
	cat <<-'EOF'
		FROM ubuntu:18.04

		RUN apt-get update && \
		DEBIAN_FRONTEND=noninteractive apt-get install g++ git make ninja-build pax-utils lintian sudo libssl-dev -y && \
		rm -rf /var/lib/apt/lists/* && \
		useradd -rm ecwolf && \
		echo "ecwolf ALL=(ALL) NOPASSWD: /usr/bin/ninja install" >> /etc/sudoers && \
		mkdir /home/ecwolf/results && \
		chown ecwolf:ecwolf /home/ecwolf/results && \
		ln -s /home/ecwolf/results /results

		ADD https://ftp.gnu.org/gnu/gcc/gcc-10.2.0/gcc-10.2.0.tar.xz gcc.tar.xz
		RUN tar xf gcc.tar.xz && \
		( cd gcc-10.2.0 && contrib/download_prerequisites ) && \
		mkdir gcc-build && \
		cd gcc-build && \
		../gcc-10.2.0/configure --disable-multilib --enable-languages=c,c++,lto --prefix=/usr --program-suffix=-10 && \
		make -j "$(nproc)" && \
		make install-strip && \
		cd .. && rm -rf gcc-* gcc.tar.xz
		
		ADD https://cmake.org/files/v3.18/cmake-3.18.0.tar.gz cmake.tar.gz
		RUN tar xf cmake.tar.gz && \
		cd cmake-3.18.0 && \
		./configure --prefix=/usr --parallel="$(nproc)" && \
		make -j "$(nproc)" && \
		make install/strip && \
		cd .. && rm -rf cmake-* cmake.tar.gz

		USER ecwolf
	EOF
}

# Performs a build of WolfstoneExtract. Extra CMake args can be passed as args.
build_wolfstoneextract() {
	declare SrcDir=/mnt

	cd ~ || return

	# Check for previous invocation
	if [[ -d build ]]; then
		rm -rf build
	fi

	# Only matters on CMake 3.5+
	export CLICOLOR_FORCE=1

	mkdir build &&
	cd build &&
	CXX=g++-10 CC=gcc-10 cmake "$SrcDir" -GNinja \
		-DINSTALL_FLAT=ON \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" "$@" &&
	touch install_manifest.txt && # Prevent root from owning this file
	ninja || return
}
export -f build_wolfstoneextract

test_build_wolfstoneextract() {
	declare BuildCfg=$1
	shift

	build_wolfstoneextract "$@" 2>&1 | tee "/results/buildlog-$BuildCfg.log"
	(( PIPESTATUS[0] == 0 )) || return "${PIPESTATUS[0]}"

	cd ~/build &&
	sudo ninja install 2>&1 | tee "/results/installlog-$BuildCfg.log"
	(( PIPESTATUS[0] == 0 )) || return "${PIPESTATUS[0]}"

	lddtree /usr/bin/wolfstoneextract | tee "/results/lddtree-$BuildCfg.txt"
}
export -f test_build_wolfstoneextract

# shellcheck disable=SC2034
declare -A ConfigUbuntuMinimum=(
	[dockerfile]=dockerfile_ubuntu_minimum
	[dockerimage]='wolfstoneextract-ubuntu'
	[dockertag]=1
	[entrypoint]=test_build_wolfstoneextract
	[prereq]=''
	[type]=test
)

# Ubuntu packaging -------------------------------------------------------------

package_wolfstoneextract() {
	{
		build_wolfstoneextract &&

		ninja package &&
		cp wolfstoneextract-*.tar* /results/
	} 2>&1 | tee '/results/build.log'
	return "${PIPESTATUS[0]}"
}
export -f package_wolfstoneextract

# shellcheck disable=SC2034
declare -A ConfigUbuntuPackage=(
	[dockerfile]=dockerfile_ubuntu_minimum
	[dockerimage]='wolfstoneextract-ubuntu'
	[dockertag]=1
	[entrypoint]=package_wolfstoneextract
	[prereq]=ConfigUbuntuMinimum
	[type]=build
)

# Ubuntu MinGW-w64 -------------------------------------------------------------

dockerfile_mingw() {
	cat <<-'EOF'
		FROM ubuntu:20.04

		RUN apt-get update && \
		DEBIAN_FRONTEND=noninteractive apt-get install cmake git g++-mingw-w64-x86-64 ninja-build -y && \
		rm -rf /var/lib/apt/lists/* && \
		useradd -rm ecwolf && \
		echo "ecwolf ALL=(ALL) NOPASSWD: /usr/bin/ninja install" >> /etc/sudoers && \
		mkdir /home/ecwolf/results && \
		chown ecwolf:ecwolf /home/ecwolf/results && \
		ln -s /home/ecwolf/results /results

		USER ecwolf
	EOF
}

build_mingw() {
	declare SrcDir=/mnt

	declare Arch
	for Arch in x86_64; do
		{
			mkdir ~/build-mgw-"$Arch" &&
			cd ~/build-mgw-"$Arch" &&
			CXX="$Arch-w64-mingw32-g++-posix" CC="$Arch-w64-mingw32-gcc-posix" cmake "$SrcDir" -GNinja \
				-DINSTALL_FLAT=ON \
				-DCMAKE_BUILD_TYPE=Release \
				-DCMAKE_SYSTEM_NAME=Windows \
				-DCMAKE_FIND_ROOT_PATH="/usr/$Arch-w64-mingw32" \
				-DCMAKE_RC_COMPILER="$Arch-w64-mingw32-windres" \
				-DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" &&
			ninja &&
			ninja package &&
			cp wolfstoneextract-*.zip /results/
		} 2>&1 | tee "/results/build-$Arch.log"
		(( PIPESTATUS[0] == 0 )) || return "${PIPESTATUS[0]}"
	done
}
export -f build_mingw

# shellcheck disable=SC2034
declare -A ConfigMinGW=(
	[dockerfile]=dockerfile_mingw
	[dockerimage]='wolfstoneextract-mingw'
	[dockertag]=1
	[entrypoint]=build_mingw
	[prereq]=''
	[type]=build
)

# ------------------------------------------------------------------------------

declare -A ConfigList=(
	[mingw]=ConfigMinGW
	[ubuntumin]=ConfigUbuntuMinimum
	[ubuntupkg]=ConfigUbuntuPackage
)

main "$@"; exit
