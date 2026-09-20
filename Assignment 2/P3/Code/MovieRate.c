#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

/*
 * MovieRate — MDC Basic Collaborative Filtering
 * -----------------------------------------------
 * Predicts the rating a user would give to an unseen movie using the
 * Most-Similar-User (minimum RatingDistance) approach.
 *
 * Usage : MovieRate [UserID] [MovieID]
 * Output: The predicted rating for UserID: [U] and MovieID: [M] is [Rating]
 *         based on the most similar user: [SimUserID] with a ratingdistance: [Distance]
 *
 * Algorithm:
 *  1. Load all ratings into a flat array.
 *  2. Find all OTHER users who have rated MovieID.
 *  3. For each such candidate user, compute the RatingDistance to the query
 *     user (average |r1 - r2| over movies both have rated).
 *     Skip candidates with 0 shared movies (undefined distance).
 *  4. Pick the candidate with the smallest distance.
 *     Tie-break: choose the lowest user ID.
 *  5. That candidate's rating for MovieID is the predicted rating.
 *
 * Sparsity note:
 *  - If the query user has no ratings at all, we cannot compute any distance.
 *  - If no other user has rated MovieID, prediction is impossible.
 *  - These edge cases are reported with a clear error message.
 */

/* ── per-movie rating lookup table (sorted by movie_id) ─────────────── */
typedef struct {
    int   movie_id;
    float rating;
} MovieRating;

/* Compare for qsort / bsearch by movie_id */
static int cmp_movie_rating(const void *a, const void *b) {
    const MovieRating *x = (const MovieRating *)a;
    const MovieRating *y = (const MovieRating *)b;
    return (x->movie_id > y->movie_id) - (x->movie_id < y->movie_id);
}

/* ── build a sorted MovieRating[] for a single user ─────────────────── */
static MovieRating *build_user_profile(RatingDB *rdb, int uid, int *out_count) {
    int cnt = 0;
    for (int i = 0; i < rdb->count; i++)
        if (rdb->ratings[i].user_id == uid) cnt++;

    if (cnt == 0) { *out_count = 0; return NULL; }

    MovieRating *profile = malloc(cnt * sizeof(MovieRating));
    if (!profile) { *out_count = 0; return NULL; }

    int idx = 0;
    for (int i = 0; i < rdb->count; i++) {
        if (rdb->ratings[i].user_id == uid) {
            profile[idx].movie_id = rdb->ratings[i].movie_id;
            profile[idx].rating   = rdb->ratings[i].rating;
            idx++;
        }
    }
    qsort(profile, cnt, sizeof(MovieRating), cmp_movie_rating);
    *out_count = cnt;
    return profile;
}

/* ── binary search rating for a movie in a sorted profile ───────────── */
static int find_rating(MovieRating *profile, int cnt, int movie_id, float *out) {
    int lo = 0, hi = cnt - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (profile[mid].movie_id == movie_id) { *out = profile[mid].rating; return 1; }
        if (profile[mid].movie_id < movie_id) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

/* ── RatingDistance between two pre-built sorted profiles ───────────── */
static double rating_distance(MovieRating *p1, int c1,
                              MovieRating *p2, int c2,
                              int *shared) {
    *shared = 0;
    double total = 0.0;
    int i = 0, j = 0;
    while (i < c1 && j < c2) {
        if (p1[i].movie_id == p2[j].movie_id) {
            total += fabs(p1[i].rating - p2[j].rating);
            (*shared)++;
            i++; j++;
        } else if (p1[i].movie_id < p2[j].movie_id) {
            i++;
        } else {
            j++;
        }
    }
    return (*shared > 0) ? total / *shared : DBL_MAX;
}

/* ── main ────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: MovieRate [UserID] [MovieID]\n");
        return 1;
    }

    int query_uid = atoi(argv[1]);
    int query_mid = atoi(argv[2]);

    if (query_uid <= 0 || query_mid <= 0) {
        fprintf(stderr, "Error: UserID and MovieID must be positive integers.\n");
        return 1;
    }

    RatingDB *rdb = load_ratings("ratings.dat");
    if (!rdb) {
        fprintf(stderr, "Failed to load ratings.dat\n");
        return 1;
    }

    /* Build query user's profile */
    int         query_count;
    MovieRating *query_profile = build_user_profile(rdb, query_uid, &query_count);

    if (query_count == 0) {
        fprintf(stderr,
            "Error: User %d has no ratings — cannot compute any distance.\n",
            query_uid);
        free(query_profile);
        free_ratings(rdb);
        return 1;
    }

    /*
     * Iterate over all ratings for query_mid.
     * For each distinct other-user who rated it, compute distance to query_user.
     */
    double best_dist    = DBL_MAX;
    int    best_uid     = -1;
    float  best_rating  = 0.0f;

    /* We need to avoid re-processing the same candidate user.
     * Strategy: scan the flat ratings array; for each rating of query_mid
     * by a user != query_uid, build that user's profile on the fly and compute
     * the distance. To avoid rebuilding it multiple times, we track processed
     * user IDs with a simple visited array.
     *
     * With ~70 000 distinct users, a boolean array of size max_uid+1 is fine.
     */
    int max_uid = 0;
    for (int i = 0; i < rdb->count; i++)
        if (rdb->ratings[i].user_id > max_uid) max_uid = rdb->ratings[i].user_id;

    char *visited = calloc(max_uid + 1, 1);
    if (!visited) {
        fprintf(stderr, "Out of memory\n");
        free(query_profile);
        free_ratings(rdb);
        return 1;
    }

    int candidates_found = 0;

    for (int i = 0; i < rdb->count; i++) {
        if (rdb->ratings[i].movie_id != query_mid) continue;
        int cuid = rdb->ratings[i].user_id;
        if (cuid == query_uid) continue;
        if (visited[cuid]) continue;
        visited[cuid] = 1;
        candidates_found++;

        int         cand_count;
        MovieRating *cand_profile = build_user_profile(rdb, cuid, &cand_count);
        if (!cand_profile) continue;

        /* Get candidate's rating for the query movie */
        float cand_movie_rating;
        if (!find_rating(cand_profile, cand_count, query_mid, &cand_movie_rating)) {
            free(cand_profile);
            continue;
        }

        int    shared;
        double dist = rating_distance(query_profile, query_count,
                                      cand_profile,  cand_count, &shared);
        free(cand_profile);

        if (shared == 0) continue;  /* no common ground → skip */

        /* Update best: prefer smaller distance, tie-break on lower uid */
        if (dist < best_dist || (dist == best_dist && cuid < best_uid)) {
            best_dist   = dist;
            best_uid    = cuid;
            best_rating = cand_movie_rating;
        }
    }

    free(visited);
    free(query_profile);
    free_ratings(rdb);

    if (candidates_found == 0) {
        fprintf(stderr,
            "Error: No other user has rated movie %d — prediction impossible.\n",
            query_mid);
        return 1;
    }

    if (best_uid == -1) {
        fprintf(stderr,
            "Error: No candidate user shares any commonly rated movies "
            "with user %d — prediction impossible.\n", query_uid);
        return 1;
    }

    printf("The predicted rating for UserID: %d and MovieID: %d is %.0f "
           "based on the most similar user: %d with a ratingdistance: %.2f\n",
           query_uid, query_mid, (double)best_rating, best_uid, best_dist);

    return 0;
}
