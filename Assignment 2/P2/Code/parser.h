#ifndef PARSER_H
#define PARSER_H

/* ── Movie / MovieDB ─────────────────────────────────────────────────── */
typedef struct {
    int   id;
    char *title;
    int   year;
    char **genres;
    int   num_genres;
    char **tags;
    int   num_tags;
} Movie;

typedef struct {
    Movie *movies;
    int    count;
} MovieDB;

MovieDB *load_database(void);
void     free_database(MovieDB *db);

/* ── Rating / RatingDB ───────────────────────────────────────────────── */
typedef struct {
    int   user_id;
    int   movie_id;
    float rating;
} Rating;

typedef struct {
    Rating *ratings;
    int     count;
} RatingDB;

RatingDB *load_ratings(const char *path);
void      free_ratings(RatingDB *rdb);

#endif
