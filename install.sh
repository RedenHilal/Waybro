#!/bin/bash
set -e

APP_NAME="Waybro";
CONFIG_DIR="$HOME/.config/waybro/";
SRC_DIR="$(pwd)";

gen_cmake(){
    local build_type="$1";
    case "${build_type,,}" in
        debug)
            BUILD_TYPE="Debug";
            ;;
        release)
            BUILD_TYPE="Release";
            ;;
        relwithdebinfo)
            BUILD_TYPE="RelWithDebInfo"
            ;;
        minsizerel)
            BUILD_TYPE="MinSizeRel"
            ;;
        *)
            echo "Invalid Build type: $build_type"
            echo "Available build type are debug, release, relwithdebinfo, and minsizerel"
            exit 1
            ;;
    esac

    mkdir -p "$SRC_DIR/build/" && cd "$SRC_DIR/build";
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="/usr/local" "$SRC_DIR";

}


echo "Building Waybro... be patient :3";

gen_cmake "$1"

make

sudo make install

mkdir -p "$CONFIG_DIR";
cp "$SRC_DIR/pconfig" "$CONFIG_DIR/";

echo "Done"



