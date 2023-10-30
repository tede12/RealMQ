#!/bin/bash

# Function definitions for setting up the virtual environment
setup_venv() {
  echo "[INFO] Setting up Python virtual environment..."

  if [ ! -d "venv" ]; then
    python3 -m venv venv
  fi

  source venv/bin/activate

  echo "[INFO] Installing Python dependencies from requirements.txt..."
  if [ -f requirements.txt ]; then
    mkdir -p tmp
    pip freeze >tmp/current_requirements.txt
    if ! comm -12 <(sort requirements.txt | uniq) <(sort tmp/current_requirements.txt | uniq) &>/dev/null; then
      pip install -r requirements.txt
    else
      echo "[INFO] Python dependencies already satisfied."
    fi
    rm tmp/current_requirements.txt
  else
    echo "[WARN] requirements.txt not found. Skipping Python dependencies installation."
  fi
}

# Libczmq installation with drafts enabled (only for Debian-like systems)
install_libczmq_with_draft_enable() {
  echo "[INFO] Installing libczmq with drafts enabled..."

  echo "[INFO] Installing necessary dependencies for libczmq..."
  sudo apt-get install libtool pkg-config build-essential autoconf automake

  git clone https://github.com/zeromq/libzmq
  cd libzmq
  ./autogen.sh
  ./configure --with-libsodium --enable-drafts=yes
  make -j $(nproc)
  sudo make install
}

# MacOS installation
install_macos() {
  echo "[INFO] Installing dependencies on MacOS..."

  # Check and install Homebrew if not present
  if ! command -v brew &>/dev/null; then
    echo "[INFO] Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  else
    echo "[INFO] Homebrew already installed."
  fi

  # Install the packages
  for package in zeromq czmq json-c libyaml cmake make python; do
    if brew list $package &>/dev/null; then
      echo "[INFO] $package already installed."
    else
      brew install $package
    fi
  done

  setup_venv
}

# Debian-like installation
install_debian_like() {
  echo "[INFO] Installing dependencies on Debian-like system..."
  sudo apt-get update

  # Install the packages
  for package in libzmq3-dev libczmq-dev libjson-c-dev libyaml-dev cmake make python3 python3-venv python3-pip; do
    if dpkg -l | grep -q $package; then
      echo "[INFO] $package already installed."
    else
      sudo apt-get install -y $package
    fi
  done

  install_libczmq_with_draft_enable

  setup_venv
}



# Detect the operating system and initiate the appropriate installation
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  install_debian_like
elif [[ "$OSTYPE" == "darwin"* ]]; then
  install_macos
else
  echo "[ERROR] Sorry, only MacOS and Debian-like systems are supported by this script at the moment."
  exit 1
fi

echo "[INFO] Installation completed!"
