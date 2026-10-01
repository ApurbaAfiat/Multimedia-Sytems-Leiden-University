#!/usr/bin/env python3
"""Verify the fixed 15-image COCO val2017 subset supplied with the assignment."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent
DATASET = ROOT / "dataset"
IMAGES = DATASET / "images"
GT = DATASET / "ground_truth"
MANIFEST = DATASET / "selected_images.txt"

EXPECTED_BUCKET_COUNTS = {
    "easy": 5,
    "medium": 20,
    "difficult": 50,
}

EXPECTED_TOTAL = 75


def read_manifest():
    rows = []

    for raw in MANIFEST.read_text(encoding="utf-8").splitlines():
        raw = raw.strip()

        if not raw or raw.startswith("#"):
            continue

        filename, bucket = raw.split()
        rows.append((filename, bucket))

    return rows


def main() -> int:
    rows = read_manifest()

    if len(rows) != 15:
        raise RuntimeError(
            f"Expected 15 images in manifest, found {len(rows)}"
        )

    counts = {
        "easy": 0,
        "medium": 0,
        "difficult": 0,
    }

    total = 0

    for filename, bucket in rows:
        image_file = IMAGES / filename
        gt_file = GT / f"{Path(filename).stem}.txt"

        if not image_file.exists():
            raise FileNotFoundError(f"Missing image: {image_file}")

        if not gt_file.exists():
            raise FileNotFoundError(
                f"Missing ground-truth file: {gt_file}"
            )

        lines = [
            line
            for line in gt_file.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]

        counts[bucket] += len(lines)
        total += len(lines)

    print("Dataset verification:")
    print("Images:     15")
    print("GT files:   15")
    print()
    print("Ground-truth object counts:")
    print(f"easy       {counts['easy']}")
    print(f"medium     {counts['medium']}")
    print(f"difficult  {counts['difficult']}")
    print(f"overall    {total}")

    if counts != EXPECTED_BUCKET_COUNTS:
        raise RuntimeError(f"Unexpected bucket counts: {counts}")

    if total != EXPECTED_TOTAL:
        raise RuntimeError(f"Unexpected total object count: {total}")

    print("\nDataset verification PASS.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
