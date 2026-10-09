#!/bin/bash

set -euo pipefail

DEBUG=0
IMAGE="devkitpro/devkitarm:latest"

docker_make() {
    docker run --rm \
        -u $(id -u):$(id -g) \
        -v "$PWD:/work" \
        -w /work \
        "$IMAGE" \
        make "$@"
}

install() {
    if [ -z "${NITRO_IP:-}" ]; then
        echo "NITRO_IP is not set, skipping install"
        return
    fi

    if ! command -v 3dslink &>/dev/null; then
        echo "3dslink not found, skipping install"
        return
    fi

    local args=(-a "$NITRO_IP")

    if [ "$DEBUG" = "1" ]; then
        args+=(-s)
    fi

    # The executable is named after TARGET in moteur.mk.
    local target
    target=$(sed -n 's/^TARGET[[:space:]]*:=[[:space:]]*//p' moteur.mk)

    until 3dslink "${args[@]}" "${target}.3dsx"; do
        echo "3DS not reachable, retrying in 5 seconds..."
        sleep 5
    done
}

run_command() {
    case "$1" in
        build)   docker_make DEBUG="$DEBUG";;
        install) install;;
        all)     docker_make clean
                 docker_make DEBUG="$DEBUG"
                 install;;
        *)       docker_make "$1";;
    esac
}

if [ "${1:-}" = "-d" ]; then
    DEBUG=1
    shift
fi

for command in "${@:-all}"; do
    run_command "$command"
done
