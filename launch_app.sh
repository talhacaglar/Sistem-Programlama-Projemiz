#!/bin/bash
# PicoRV32 Studio Desktop Launcher

# Ensure we are in the project directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

# Ensure binaries are built
if [ ! -f "./assembler_bin" ] || [ ! -f "./linker_bin" ]; then
    echo "Derleyiciler hazirlaniyor..."
    make all
fi

# Run the app using the virtual environment
echo "Uygulama baslatiliyor..."
./venv/bin/python3 gui/main_app.py
