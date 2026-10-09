#!/bin/bash -eu

WAV=$1
RAW=$(basename "$WAV" .wav)
ROOT_DIR=$(dirname "$(cd "$(dirname "$0")" && pwd)")

ffmpeg -i "$WAV" -c:a libvorbis -b:a 64k "$ROOT_DIR/resources/audio/$RAW.ogg"
