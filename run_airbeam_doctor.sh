#!/usr/bin/env bash
set -xe

ROOT_DIR=$(dirname "$0")

rm -rf ./build

# should not remove, or CPM not work. I don't know why.
sleep 1

cmake -S . -B build -GNinja
cmake --build build --target AirBeamDoctor

${ROOT_DIR}/build/source/AirBeamDoctor/AirBeamDoctor --audio_pcm ${ROOT_DIR}/resources/audio.pcm --log ${ROOT_DIR}/mylog.log
