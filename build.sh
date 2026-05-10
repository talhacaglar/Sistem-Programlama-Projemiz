#!/bin/bash
# Tek tusla derleme ve calistirma betigi
set -e

echo "1. C kodlari derleniyor (Assembler & Linker)..."
make clean

echo ""
echo "2. Knight Rider test programi derleniyor (Assembly)..."
make build

echo ""
echo "[BASARILI] Tum adimlar tamamlandi. Ciktilar 'output/' klasorunde."
