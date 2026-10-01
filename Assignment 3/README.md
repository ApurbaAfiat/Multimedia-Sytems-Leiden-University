# Darknet Object Detection Homework - Student Package

This package accompanies `assignment.pdf`.

## Package contents

- `assignment.pdf` - homework instructions
- `starter.cpp` - supplied C++ evaluator; **only the IoU function is intentionally incomplete**
- `prepare_dataset.py` - verifies the supplied 15-image COCO val2017 subset and ground truth
- `download_weights.sh` - downloads the pretrained YOLOv3 and YOLOv7 demo weights
- `dataset/selected_images.txt` - the fixed easy/medium/difficult image manifest
- `dataset/images/` - supplied 15 COCO validation images
- `dataset/ground_truth/` - supplied ground-truth files for the 15 selected images

The tested environments are:

- Ubuntu 26.04, CPU-only;
- LIACS Linux Workstation (LLW), Ubuntu 24.04.5, CPU-only.

Both were tested with Darknet V5 "Moonlit" commit
`f684e1d75d4594298c8e73acc727ba6cf2e81c60`.


## Teams and evaluation terminology

You may work **individually or in a team of 2 students**.

The supplied dataset includes ground-truth annotations in `dataset/ground_truth/`.
These annotations describe the correct object classes and bounding boxes for the
15 supplied images. The evaluator reports:

- **TP (True Positive):** a correct detection of a ground-truth object with the same class and sufficient IoU;
- **FP (False Positive):** a predicted object that is not successfully matched to ground truth;
- **FN (False Negative):** a ground-truth object that was not detected correctly;
- **Precision:** `TP / (TP + FP)`;
- **Recall:** `TP / (TP + FN)`;
- **Mean IoU:** the average IoU of correctly matched detections;
- **Average inference time:** the average time used to process one image.

For this assignment, a valid match requires the same class and `IoU >= 0.50`.
In your report, explain the minimum IoU required for a detection to count as
correct and what happens when the IoU is below that threshold.

## If you are on a LIACS PC room computer

The LIACS Linux Workstation computers already provide the required compiler and
libraries through environment modules. **Do not use `apt-get` or `sudo dpkg -i`
on a LIACS workstation.** Use the following setup instead.

Load the required modules:

```bash
module purge
module load OpenCV/4.8.1-foss-2023a-CUDA-12.1.1-contrib
module load protobuf/24.0-GCCcore-12.3.0
module load CMake/3.26.3-GCCcore-12.3.0
module load Doxygen/1.9.7-GCCcore-12.3.0
```

Build the tested Darknet revision locally in your home directory:

```bash
mkdir -p ~/src
cd ~/src
git clone https://codeberg.org/CCodeRun/darknet.git darknet_liacs
cd darknet_liacs
git checkout f684e1d75d4594298c8e73acc727ba6cf2e81c60

mkdir -p build
cd build
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DDARKNET_TRY_CUDA=OFF \
  -DDARKNET_TRY_ROCM=OFF \
  -DDARKNET_TRY_OPENBLAS=OFF \
  ..
cmake --build . -j4
```

OpenBLAS is deliberately disabled on the LIACS workstations because the
available LIACS OpenBLAS installation uses a different 64-bit CBLAS interface
from the one expected by this Darknet revision. This does not affect the
correctness of the assignment; Darknet runs normally in CPU-only mode.

Verify the local build:

```bash
LD_LIBRARY_PATH="$HOME/src/darknet_liacs/build/src-lib:${LD_LIBRARY_PATH:-}" \
  "$HOME/src/darknet_liacs/build/src-cli/darknet" version
```

Compile `starter.cpp` on LIACS with:

```bash
g++ -std=c++17 starter.cpp -o detector \
  -I"$HOME/src/darknet_liacs/src-lib" \
  -L"$HOME/src/darknet_liacs/build/src-lib" \
  -Wl,-rpath,"$HOME/src/darknet_liacs/build/src-lib" \
  -ldarknet \
  $(pkg-config --cflags --libs opencv4)
```

For the YOLOv3 and YOLOv7 commands below, use the configuration files from
`~/src/darknet_liacs/cfg/` instead of `~/src/darknet/cfg/`.

### Remote access to a LIACS workstation

The same LIACS PC room computers can be used remotely through the LIACS SSH
gateway. For example:

```bash
ssh YOUR_ULCN@0065066.student.liacs.nl -J YOUR_ULCN@ssh.liacs.nl
```

`0065066.student.liacs.nl` is only an example. Choose any powered-on LIACS
Linux Workstation from the current computer list at:
`https://rel.liacs.nl/LLW`.

Use `ssh.liacs.nl` as the jump host. The LIACS lab-room computers are not
directly reachable through the ISSC gateway `sshgw.leidenuniv.nl`.

After connecting remotely, follow the same LIACS setup commands above.

## 1. Install dependencies (your own Ubuntu system)

If you are using a LIACS workstation, skip Sections 1 and 2 and use the LIACS
instructions above.

```bash
sudo apt-get update
sudo apt-get install build-essential git libopencv-dev cmake \
  libprotobuf-dev protobuf-compiler \
  libopenblas64-0 libopenblas64-0-openmp libopenblas64-openmp-dev \
  python3 wget unzip
```

## 2. Build and install the tested Darknet version (your own Ubuntu system)

```bash
mkdir -p ~/src
cd ~/src
git clone https://codeberg.org/CCodeRun/darknet.git
cd darknet
git checkout f684e1d75d4594298c8e73acc727ba6cf2e81c60
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4 package
sudo dpkg -i darknet-5.1.104-Linux.deb
```

Verify:

```bash
darknet version
```

CPU-only execution is sufficient.

## 3. Prepare the fixed dataset

From the root of this student package:

```bash
python3 prepare_dataset.py
```

The 15 COCO validation images and their corresponding ground-truth files
are already included in the student package. This script verifies that the
dataset is complete and has the expected object counts.

A correct preparation prints these object counts:

```text
easy      5
medium   20
difficult 50
overall   75
```

## 4. Download the pretrained weights

```bash
./download_weights.sh
```

This creates:

```text
weights/yolov3.weights
weights/yolov7.weights
```

The weights are the MS COCO demo weights distributed by the Darknet project.
Interrupted downloads are resumed automatically, and the script verifies the
SHA-256 checksum of both model files before continuing.

## 5. Compile the supplied C++ program

On a LIACS workstation, use the LIACS-specific compile command above.
On your own Ubuntu installation, use:

```bash
g++ -std=c++17 starter.cpp -o detector \
  -ldarknet $(pkg-config --cflags --libs opencv4)
```

## 6. Complete and test IoU

Open `starter.cpp` and implement only:

```cpp
double calculate_iou(const cv::Rect2d &a, const cv::Rect2d &b)
```

Then run:

```bash
./detector --test-iou
```

Your implementation should print `PASS`.

## 7. Run YOLOv3

Your own Ubuntu installation:

```bash
./detector \
  ~/src/darknet/cfg/yolov3.cfg \
  ~/src/darknet/cfg/coco.names \
  weights/yolov3.weights \
  dataset \
  dataset/selected_images.txt
```

LIACS workstation:

```bash
./detector \
  ~/src/darknet_liacs/cfg/yolov3.cfg \
  ~/src/darknet_liacs/cfg/coco.names \
  weights/yolov3.weights \
  dataset \
  dataset/selected_images.txt
```

## 8. Run YOLOv7

Your own Ubuntu installation:

```bash
./detector \
  ~/src/darknet/cfg/yolov7.cfg \
  ~/src/darknet/cfg/coco.names \
  weights/yolov7.weights \
  dataset \
  dataset/selected_images.txt
```

LIACS workstation:

```bash
./detector \
  ~/src/darknet_liacs/cfg/yolov7.cfg \
  ~/src/darknet_liacs/cfg/coco.names \
  weights/yolov7.weights \
  dataset \
  dataset/selected_images.txt
```

The program prints per-image and summary statistics and saves annotated output
images under:

```text
output/yolov3/
output/yolov7/
```

Use the requested numbers and example images in your PDF report as described in
`assignment.pdf`.

## Notes

- The evaluator uses a detection threshold of 0.24 and an IoU match threshold of 0.50.
- A match requires the same class and IoU >= 0.50.
- Matching is one-to-one and greedily chooses the highest-IoU valid pair first.
- The easy/medium/difficult labels are pedagogical groups for this assignment,
  not official COCO difficulty categories.
- Do not modify the evaluation/matching code unless instructed by the teaching staff.
