#!/usr/bin/bash -eu

ACTION=${1:-install}

if ! [ $(id -u) = 0 ]; then
  echo "Need root privilege to install!"
  exit 1
fi

install() {
  echo "* Install devkitpro-pacman"

  # ensure apt is set up to work with https sources
  apt install -qqq -y apt-transport-https

  # Store devkitPro gpg key locally if we don't have it already
  if [ ! -f /usr/share/keyring/devkitpro-pub.gpg ]; then
    mkdir -p /usr/share/keyring/
    wget -U "dkp apt" -O /usr/share/keyring/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg
  fi

  # Add the devkitPro apt repository if we don't have it set up already
  if [ ! -f /etc/apt/sources.list.d/devkitpro.list ]; then
    echo "deb [signed-by=/usr/share/keyring/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" > /etc/apt/sources.list.d/devkitpro.list
  fi

  # Finally install devkitPro pacman
  apt update -qqq
  apt install -qqq -y devkitpro-pacman

  echo "* Install 3ds-dev tools"

  dkp-pacman -S --noconfirm -q libctru citro2d 3ds-examples 3dslink tex3ds 3ds-libvorbisidec 3dstools

  echo "* Create profile script"

  if [ ! -f /etc/profile.d/devkit-env.sh ]; then
    cat <<"EOF" > /etc/profile.d/devkit-env.sh
export DEVKITPRO="/opt/devkitpro"
export DEVKITARM="${DEVKITPRO}/devkitARM"
export PATH="$PATH:${DEVKITPRO}/tools/bin"
EOF
    chmod 644 /etc/profile.d/devkit-env.sh
  fi

  echo "* Install bannertool"
  if [ ! -x /opt/devkitpro/tools/bin/bannertool ]; then
    tmpdir=$(mktemp -d)
    curl -SsL -o "$tmpdir/bannertool.zip" https://github.com/Epicpkmn11/bannertool/releases/download/v1.2.2/bannertool.zip
    pushd "$tmpdir"
    unzip -qq bannertool.zip
    mv linux-x86_64/bannertool /opt/devkitpro/tools/bin/
    popd
    chmod 755 /opt/devkitpro/tools/bin/bannertool
    rm -rf "$tmpdir"
  fi

  echo "* Install makerom"
  if [ ! -x /opt/devkitpro/tools/bin/makerom ]; then
    tmpdir=$(mktemp -d)
    curl -SsL -o "$tmpdir/makerom.zip" https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.4/makerom-v0.18.4-ubuntu_x86_64.zip
    pushd "$tmpdir"
    unzip -qq makerom.zip
    mv makerom /opt/devkitpro/tools/bin/
    popd
    chmod 755 /opt/devkitpro/tools/bin/makerom
    rm -rf "$tmpdir"
  fi

  echo "* Completed"
  echo
  echo "Please run: source /etc/profile.d/devkit-env.sh"
}

uninstall() {
  if [ -f /etc/profile.d/devkit-env.sh ]; then
    rm -f /etc/profile.d/devkit-env.sh
  fi
  echo "* Uninstall 3ds-dev tools"
  if command -v dkp-pacman &>/dev/null; then
    dkp-pacman -R --noconfirm -q libctru citro2d 3ds-examples 3dslink tex3ds 3ds-libvorbisidec 3dstools
  fi
  if [ -d /opt/devkitpro ]; then
    rm -rf /opt/devkitpro
  fi

  echo "* Uninstall devkitpro-pacman"
  apt purge -y devkitpro-pacman

  if [ -f /usr/share/keyring/devkitpro-pub.gpg ]; then
    rm -f /usr/share/keyring/devkitpro-pub.gpg
  fi
  echo "* Completed"
}

case "$ACTION" in
install) install;;
uninstall) uninstall;;
*) echo "$ACTION not supported. Must be one of: install, uninstall"
esac
