#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * RatingDistance
 * --------------
 * Computes the average absolute difference in ratings between two users,
 * considering only movies that BOTH users have rated.
 *
 * Usage: RatingDistance [UserID1] [UserID2]
 * Output: The RatingDistance is [Distance]
 */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: RatingDistance [UserID1] [UserID2]\n");
        return 1;
    }

    int uid1 = atoi(argv[1]);
    int uid2 = atoi(argv[2]);

    if (uid1 <= 0 || uid2 <= 0) {
        fprintf(stderr, "Error: User IDs must be positive integers.\n");
        return 1;
    }

    /* Load ratings */
    RatingDB *rdb = load_ratings("ratings.dat");
    if (!rdb) {
        fprintf(stderr, "Failed to load ratings.dat\n");
        return 1;
    }

    /*
     * Strategy: two-pass scan.
     *
     * Pass 1: collect all (movieID → rating) for user1 into a flat array,
     *         sorted by movieID so we can binary-search in pass 2.
     */

    /* Count how many ratings user1 has */
    int u1_count = 0;
    for (int i = 0; i < rdb->count; i++)
        if (rdb->ratings[i].user_id == uid1) u1_count++;

    if (u1_count == 0) {
        printf("The RatingDistance is 0.00\n");
        printf("(User %d has no ratings)\n", uid1);
        free_ratings(rdb);
        return 0;
    }

    /* Collect user1 ratings into parallel arrays */
    int   *u1_movies  = malloc(u1_count * sizeof(int));
    float *u1_ratings = malloc(u1_count * sizeof(float));
    if (!u1_movies || !u1_ratings) {
        fprintf(stderr, "Out of memory\n");
        free(u1_movies); free(u1_ratings);
        free_ratings(rdb);
        return 1;
    }

    int idx = 0;
    for (int i = 0; i < rdb->count; i++) {
        if (rdb->ratings[i].user_id == uid1) {
            u1_movies[idx]  = rdb->ratings[i].movie_id;
            u1_ratings[idx] = rdb->ratings[i].rating;
            idx++;
        }
    }

    /* Sort user1 entries by movie_id for binary search */
    /* Simple insertion sort is fine for a per-user set (typically < 10k) */
    for (int i = 1; i < u1_count; i++) {
        int   km = u1_movies[i];
        float kr = u1_ratings[i];
        int j = i - 1;
        while (j >= 0 && u1_movies[j] > km) {
            u1_movies[j+1]  = u1_movies[j];
            u1_ratings[j+1] = u1_ratings[j];
            j--;
        }
        u1_movies[j+1]  = km;
        u1_ratings[j+1] = kr;
    }

    /*
     * Pass 2: iterate over user2's ratings; for each movieID, binary-search
     *         user1's array to see if user1 also rated that movie.
     */
    double total_dist = 0.0;
    int    shared     = 0;

    for (int i = 0; i < rdb->count; i++) {
        if (rdb->ratings[i].user_id != uid2) continue;

        int   mid = rdb->ratings[i].movie_id;
        float r2  = rdb->ratings[i].rating;

        /* Binary search in u1_movies */
        int lo = 0, hi = u1_count - 1, found = -1;
        while (lo <= hi) {
            int m2 = (lo + hi) / 2;
            if (u1_movies[m2] == mid) { found = m2; break; }
            else if (u1_movies[m2] < mid) lo = m2 + 1;
            else hi = m2 - 1;
        }
        if (found >= 0) {
            total_dist += fabs(u1_ratings[found] - r2);
            shared++;
        }
    }

    free(u1_movies);
    free(u1_ratings);
    free_ratings(rdb);

    if (shared == 0) {
        printf("The RatingDistance is 0.00\n");
        printf("(Users %d and %d share no commonly rated movies)\n", uid1, uid2);
    } else {
        double dist = total_dist / shared;
        printf("The RatingDistance is %.2f\n", dist);
    }

    return 0;
}
