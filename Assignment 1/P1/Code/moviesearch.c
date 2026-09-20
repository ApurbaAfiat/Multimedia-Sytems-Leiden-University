#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static int str_contains_ci(const char *haystack, const char *needle) {
  size_t nlen = strlen(needle);
  size_t hlen = strlen(haystack);
  if (nlen > hlen)
    return 0;
  for (size_t i = 0; i <= hlen - nlen; i++) {
    if (strncasecmp(haystack + i, needle, nlen) == 0)
      return 1;
  }
  return 0;
}

static int movie_matches(Movie *m, char **title_kws, int n_title, int year,
                         char **genres, int n_genres, char **tags, int n_tags) {
  for (int i = 0; i < n_title; i++) {
    if (!str_contains_ci(m->title, title_kws[i]))
      return 0;
  }

  if (year > 0 && m->year != year)
    return 0;

  for (int i = 0; i < n_genres; i++) {
    int found = 0;
    for (int j = 0; j < m->num_genres; j++) {
      if (strcasecmp(m->genres[j], genres[i]) == 0) {
        found = 1;
        break;
      }
    }
    if (!found)
      return 0;
  }

  for (int i = 0; i < n_tags; i++) {
    int found = 0;
    for (int j = 0; j < m->num_tags; j++) {
      if (str_contains_ci(m->tags[j], tags[i])) {
        found = 1;
        break;
      }
    }
    if (!found)
      return 0;
  }

  return 1;
}

static void print_movie(Movie *m) {
  printf("%d::%s::", m->id, m->title);
  for (int i = 0; i < m->num_genres; i++) {
    if (i > 0)
      printf("|");
    printf("%s", m->genres[i]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  char *title_kws[256];
  int n_title = 0;
  int year = 0;
  char *genres[256];
  int n_genres = 0;
  char *tags[256];
  int n_tags = 0;

  int mode = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--title") == 0)
      mode = 1;
    else if (strcmp(argv[i], "--year") == 0)
      mode = 2;
    else if (strcmp(argv[i], "--genre") == 0)
      mode = 3;
    else if (strcmp(argv[i], "--tag") == 0)
      mode = 4;
    else {
      if (mode == 1 && n_title < 256)
        title_kws[n_title++] = argv[i];
      else if (mode == 2) {
        year = atoi(argv[i]);
        mode = 0;
      } else if (mode == 3 && n_genres < 256)
        genres[n_genres++] = argv[i];
      else if (mode == 4 && n_tags < 256)
        tags[n_tags++] = argv[i];
    }
  }

  if (n_title == 0 && year == 0 && n_genres == 0 && n_tags == 0) {
    fprintf(stderr, "Usage: moviesearch [--title keywords] [--year year] "
                    "[--genre genres] [--tag tags]\n");
    return 1;
  }

  MovieDB *db = load_database();
  if (!db) {
    fprintf(stderr, "Failed to load database. Make sure movies.dat and "
                    "tags.dat are in the current directory.\n");
    return 1;
  }

  for (int i = 0; i < db->count; i++) {
    if (movie_matches(&db->movies[i], title_kws, n_title, year, genres,
                      n_genres, tags, n_tags)) {
      print_movie(&db->movies[i]);
    }
  }

  free_database(db);
  return 0;
}
