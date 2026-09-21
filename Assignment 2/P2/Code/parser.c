#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_LINE 4096

/* ── helpers ─────────────────────────────────────────────────────────── */

static char **split_sep(const char *line, const char *sep, int *count) {
    *count = 0;
    int sep_len = (int)strlen(sep);

    int n = 1;
    const char *p = line;
    const char *f;
    while ((f = strstr(p, sep)) != NULL) { n++; p = f + sep_len; }

    char **parts = malloc(n * sizeof(char *));
    if (!parts) return NULL;

    p = line;
    for (int i = 0; i < n; i++) {
        const char *next = (i < n - 1) ? strstr(p, sep) : NULL;
        size_t len = next ? (size_t)(next - p) : strlen(p);
        while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r')) len--;
        parts[i] = malloc(len + 1);
        if (!parts[i]) {
            for (int j = 0; j < i; j++) free(parts[j]);
            free(parts);
            return NULL;
        }
        memcpy(parts[i], p, len);
        parts[i][len] = '\0';
        if (next) p = next + sep_len;
    }
    *count = n;
    return parts;
}

static void free_parts(char **parts, int count) {
    for (int i = 0; i < count; i++) free(parts[i]);
    free(parts);
}

static int extract_year(const char *title) {
    size_t len = strlen(title);
    if (len < 6) return 0;
    if (title[len-1] == ')' && title[len-6] == '(') {
        char buf[5];
        memcpy(buf, title + len - 5, 4);
        buf[4] = '\0';
        int year = atoi(buf);
        if (year >= 1800 && year <= 2100) return year;
    }
    return 0;
}

static int cmp_movie_id(const void *a, const void *b) {
    const Movie *ma = (const Movie *)a;
    const Movie *mb = (const Movie *)b;
    return (ma->id > mb->id) - (ma->id < mb->id);
}

static Movie *find_movie(MovieDB *db, int movie_id) {
    int lo = 0, hi = db->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (db->movies[mid].id == movie_id) return &db->movies[mid];
        if (db->movies[mid].id < movie_id) lo = mid + 1;
        else hi = mid - 1;
    }
    return NULL;
}

/* ── file loading helper with fallback paths ────────────────────────── */

static FILE *open_file_with_fallbacks(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) return f;

    /* Fallback 1: parent directory (e.g. if run from P1/Code and dat is in Assignment 2) */
    char buf[512];
    snprintf(buf, sizeof(buf), "../%s", path);
    f = fopen(buf, "r");
    if (f) return f;

    /* Fallback 2: two levels up */
    snprintf(buf, sizeof(buf), "../../%s", path);
    f = fopen(buf, "r");
    if (f) return f;

    /* Fallback 3: inside ml-10m subfolder */
    snprintf(buf, sizeof(buf), "ml-10m/%s", path);
    f = fopen(buf, "r");
    if (f) return f;

    snprintf(buf, sizeof(buf), "../ml-10m/%s", path);
    f = fopen(buf, "r");
    if (f) return f;

    snprintf(buf, sizeof(buf), "../../ml-10m/%s", path);
    return fopen(buf, "r");
}

/* ── movies ──────────────────────────────────────────────────────────── */

static int load_movies(MovieDB *db, const char *path) {
    FILE *f = open_file_with_fallbacks(path);
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return 0; }

    int capacity = 2048;
    db->movies = malloc(capacity * sizeof(Movie));
    db->count  = 0;
    if (!db->movies) { fclose(f); return 0; }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;

        int nparts;
        char **parts = split_sep(line, "::", &nparts);
        if (!parts || nparts < 3) { if (parts) free_parts(parts, nparts); continue; }

        if (db->count >= capacity) {
            capacity *= 2;
            Movie *tmp = realloc(db->movies, capacity * sizeof(Movie));
            if (!tmp) { free_parts(parts, nparts); break; }
            db->movies = tmp;
        }

        Movie *m  = &db->movies[db->count];
        m->id     = atoi(parts[0]);
        m->title  = parts[1]; parts[1] = NULL;
        m->year   = extract_year(m->title);
        m->tags   = NULL;
        m->num_tags = 0;

        int ng;
        char **genres = split_sep(parts[2], "|", &ng);
        m->genres     = genres;
        m->num_genres = (genres ? ng : 0);

        free(parts[0]);
        for (int k = 2; k < nparts; k++) free(parts[k]);
        free(parts);

        if (m->id > 0) db->count++;
    }

    fclose(f);
    return 1;
}

static int load_tags(MovieDB *db, const char *path) {
    FILE *f = open_file_with_fallbacks(path);
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return 0; }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;

        int nparts;
        char **parts = split_sep(line, "::", &nparts);
        if (!parts || nparts < 3) { if (parts) free_parts(parts, nparts); continue; }

        int movie_id = atoi(parts[1]);
        Movie *m     = find_movie(db, movie_id);
        if (m) {
            const char *tag = parts[2];
            int dup = 0;
            for (int i = 0; i < m->num_tags; i++) {
                if (strcasecmp(m->tags[i], tag) == 0) { dup = 1; break; }
            }
            if (!dup) {
                char **tmp = realloc(m->tags, (m->num_tags + 1) * sizeof(char *));
                if (tmp) {
                    m->tags = tmp;
                    m->tags[m->num_tags] = strdup(tag);
                    m->num_tags++;
                }
            }
        }
        free_parts(parts, nparts);
    }

    fclose(f);
    return 1;
}

MovieDB *load_database(void) {
    MovieDB *db = malloc(sizeof(MovieDB));
    if (!db) return NULL;
    db->movies = NULL;
    db->count  = 0;

    if (!load_movies(db, "movies.dat")) { free(db); return NULL; }
    qsort(db->movies, db->count, sizeof(Movie), cmp_movie_id);
    load_tags(db, "tags.dat");
    return db;
}

void free_database(MovieDB *db) {
    for (int i = 0; i < db->count; i++) {
        Movie *m = &db->movies[i];
        free(m->title);
        for (int j = 0; j < m->num_genres; j++) free(m->genres[j]);
        free(m->genres);
        for (int j = 0; j < m->num_tags; j++) free(m->tags[j]);
        free(m->tags);
    }
    free(db->movies);
    free(db);
}

/* ── ratings ─────────────────────────────────────────────────────────── */

RatingDB *load_ratings(const char *path) {
    FILE *f = open_file_with_fallbacks(path);
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }

    int capacity = 1 << 20;   /* start at 1 M entries */
    RatingDB *rdb = malloc(sizeof(RatingDB));
    if (!rdb) { fclose(f); return NULL; }
    rdb->ratings = malloc(capacity * sizeof(Rating));
    rdb->count   = 0;
    if (!rdb->ratings) { free(rdb); fclose(f); return NULL; }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') continue;

        int uid, mid;
        float r;
        /* format: userID::movieID::rating::timestamp */
        char *p = line;
        char *end;
        uid = (int)strtol(p, &end, 10); if (!end || end[0] != ':') continue; p = end + 2;
        mid = (int)strtol(p, &end, 10); if (!end || end[0] != ':') continue; p = end + 2;
        r   = strtof(p, &end);          if (!end || (end[0] != ':' && end[0] != '\n' && end[0] != '\r' && end[0] != '\0')) continue;

        if (rdb->count >= capacity) {
            capacity *= 2;
            Rating *tmp = realloc(rdb->ratings, capacity * sizeof(Rating));
            if (!tmp) break;
            rdb->ratings = tmp;
        }
        rdb->ratings[rdb->count].user_id  = uid;
        rdb->ratings[rdb->count].movie_id = mid;
        rdb->ratings[rdb->count].rating   = r;
        rdb->count++;
    }

    fclose(f);
    return rdb;
}

void free_ratings(RatingDB *rdb) {
    if (!rdb) return;
    free(rdb->ratings);
    free(rdb);
}
