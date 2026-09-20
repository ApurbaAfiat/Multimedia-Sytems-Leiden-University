# Multimedia Systems (Leiden University) - Assignment 1 Context

## Overview
This document serves as context for subsequent assignments building upon Assignment 1.

Assignment 1 focused on building an in-memory parser and search tool in standard **C (C11)** for the **MovieLens 10M dataset**, capable of compiling cleanly on **LIACS Ubuntu** using `gcc` and `make`.

---

## Repository Structure
```
.
├── .gitignore
├── First Assignment.pdf
├── answers.txt
├── context.md
├── P1/
│   ├── Code/
│   │   ├── Makefile
│   │   ├── parser.h
│   │   ├── parser.c
│   │   └── moviesearch.c
│   └── screenshots/
│       └── README.txt
└── P2/
    └── .gitkeep
```

---

## Dataset Format (MovieLens 10M)
The dataset uses `::` as the field separator and `|` as the multi-value delimiter (for genres).

1. **`movies.dat`**:
   - Format: `movieID::title (year)::genre1|genre2|...`
   - Example: `1::Toy Story (1995)::Adventure|Animation|Children|Comedy|Fantasy`
   - Year is extracted from trailing `(YYYY)` in the title string.

2. **`tags.dat`**:
   - Format: `userID::movieID::tag::timestamp`
   - Example: `15::4973::excellent!::1215184630`
   - Multiple tags can exist per movie across multiple users. Case-insensitive deduplication is performed per movie.

3. **`ratings.dat`** *(Available in the dataset, but not loaded in Assignment 1)*:
   - Format: `userID::movieID::rating::timestamp`
   - Example: `1::122::5::838985046`
   - Note: Contains ~10 million rows (~265 MB). If required in Assignment 2, parsing and memory representation must be optimized.

---

## Code Architecture & Data Structures

### 1. Data Structures ([P1/Code/parser.h](file:///P1/Code/parser.h))
```c
typedef struct {
    int id;
    char *title;
    int year;
    char **genres;
    int num_genres;
    char **tags;
    int num_tags;
} Movie;

typedef struct {
    Movie *movies;
    int count;
} MovieDB;
```

### 2. Parser Logic ([P1/Code/parser.c](file:///P1/Code/parser.c))
- **`load_database()`**:
  - Opens and parses `movies.dat` (relative path). Dynamically grows array of `Movie` structs.
  - Sorts movies by `id` using `qsort()` to enable $O(\log N)$ binary search lookup.
  - Parses `tags.dat`, locates each movie via binary search, and dynamically appends new tags to `m->tags` while deduplicating (`strcasecmp`).
- **`free_database(MovieDB *db)`**:
  - Safely deallocates all allocated memory (strings, pointer arrays, Movie array, and database struct) without leaks.

### 3. Search Logic ([P1/Code/moviesearch.c](file:///P1/Code/moviesearch.c))
- Command-line tool: `moviesearch --title [keywords] --year [year] --genre [genres] --tag [tags]`
- Multi-criteria **AND** search behavior:
  - `--title`: Case-insensitive substring match. All keywords must match.
  - `--year`: Exact numeric year match.
  - `--genre`: Case-insensitive exact genre match. All genres must match.
  - `--tag`: Case-insensitive substring match on tags. All query tags must match at least one movie tag.
- Output format: matches MovieLens format: `movieID::title (year)::genre1|genre2|...`

---

## Compilation & Execution

### Makefile Flags
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
```
- Standard C11, no compiler warnings with `-Wall -Wextra`.
- `_POSIX_C_SOURCE=200809L` ensures POSIX string functions (`strdup`, `strcasecmp`, `strncasecmp`) are available.

### Commands
```bash
cd P1/Code
make
./moviesearch --title Blood --tag Upton
./moviesearch --title Las Vegas
```

---

## Key Notes & Considerations for Assignment 2
1. **Memory Handling**:
   - The current parser loads `movies.dat` and `tags.dat` into RAM.
   - If Assignment 2 requires **`ratings.dat`**, be aware that storing full rating objects in naive pointer structs could take significant RAM. Consider flat arrays, compact structs, or streaming if needed.
2. **Portability**:
   - Keep code strictly in C (not C++).
   - Use relative paths for `.dat` files (`movies.dat`, `tags.dat`, `ratings.dat`).
   - Do not commit `.dat` or `.zip` files to git.
   - Always verify build with `make` and `gcc` on Ubuntu.
