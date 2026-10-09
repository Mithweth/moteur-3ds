#!/bin/bash -eu

WAV=${1?Input WAV file}
ROOM=${2?dedicated room}
RAW=$(basename "$WAV" .wav)
ROOT_DIR=$(dirname "$(cd "$(dirname "$0")" && pwd)")
mkdir -p "$ROOT_DIR/resources/$ROOM"

ffmpeg -i "$WAV" -ac 1 -ar 22050 -f s16le "$ROOT_DIR/resources/$ROOM/$RAW.raw"
