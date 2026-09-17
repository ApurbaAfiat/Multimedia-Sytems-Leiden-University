#ifndef PARSER_H
#define PARSER_H

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

MovieDB *load_database(void);
void free_database(MovieDB *db);

#endif
