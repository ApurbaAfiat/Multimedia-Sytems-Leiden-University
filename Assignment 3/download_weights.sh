#!/usr/bin/env bash
set -euo pipefail

mkdir -p weights
cd weights

YOLOV3_URL="https://github.com/hank-ai/darknet/releases/download/v2.0/yolov3.weights"
YOLOV7_URL="https://github.com/hank-ai/darknet/releases/download/v2.0/yolov7.weights"

YOLOV3_SHA256="523e4e69e1d015393a1b0a441cef1d9c7659e3eb2d7e15f793f060a21b32f297"
YOLOV7_SHA256="4ecf7ca13ec5039ec7b79b0f25b156fda5eaf819d6c2bb6828ba55fe4f928332"

echo "Downloading YOLOv3 weights..."
wget -c "$YOLOV3_URL" -O yolov3.weights

echo
echo "Downloading YOLOv7 weights..."
wget -c "$YOLOV7_URL" -O yolov7.weights

echo
echo "Verifying SHA-256 checksums..."

echo "$YOLOV3_SHA256  yolov3.weights" | sha256sum -c -
echo "$YOLOV7_SHA256  yolov7.weights" | sha256sum -c -

echo
echo "Weights downloaded and verified successfully:"
ls -lh yolov3.weights yolov7.weights
