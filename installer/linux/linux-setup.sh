#!/bin/bash
set -e

echo "Starting ELVM installation..."

sudo mkdir -p /usr/local/bin/library

sudo install -m 755 elvm /usr/local/bin/elvm
sudo install -m 755 ellib /usr/local/bin/ellib

echo "Installation successful!"