#define _POSIX_C_SOURCE 200809L

#include "create.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <count> [seed]\n", prog);
    fprintf(stderr, "  count: number of nodes (> 0)\n");
    fprintf(stderr, "  seed: optional unsigned integer for deterministic runs\n");
}

static int parse_seed(const char *seed_str, unsigned int *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(seed_str, &end, 10);
    if (errno || *seed_str == '\0' || *end != '\0')
        return -1;
    if (value > UINT_MAX)
        return -1;

    *out = (unsigned int)value;
    return 0;
}

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

#include <stdlib.h>

void shuffle_with_array(struct list_node **head) {
    if (!head || !*head || !(*head)->next) return;

    // 1. compute the length of the linked list
    int len = 0;
    for (struct list_node *curr = *head; curr; curr = curr->next) {
        len++;
    }

    // 2. malloc an array to hold the node pointers
    struct list_node **arr = malloc(len * sizeof(struct list_node *));
    if (!arr) return; // malloc failed

    struct list_node *curr = *head;
    for (int i = 0; i < len; i++) {
        arr[i] = curr;
        curr = curr->next;
    }

    // 3. execute Fisher-Yates shuffle on the array
    for (int i = len - 1; i > 0; i--) {
        int random = rand() % (i + 1);
        struct list_node *tmp = arr[i];
        arr[i] = arr[random];
        arr[random] = tmp;
    }

    // 4. append shuffled nodes to a new linked list
    struct list_node **indirect = head;
    for (int i = 0; i < len; i++) {
        *indirect = arr[i];
        indirect = &(*indirect)->next;
    }
    *indirect = NULL;

    free(arr);
}

int main(int argc, char **argv)
{
    int count;
    unsigned int seed;
    struct list_node *fisher_list;
    struct timespec start, end;
    double fisher_sec;

    if (argc != 2 && argc != 3) {
        usage(argv[0]);
        return 1;
    }

    if (create_parse_count(argv[1], &count) < 0) {
        usage(argv[0]);
        return 1;
    }

    if (argc == 3) {
        if (parse_seed(argv[2], &seed) < 0) {
            usage(argv[0]);
            return 1;
        }
    } else {
        seed = (unsigned int)time(NULL);
    }

    fisher_list = create_list(MODE_SEQUENTIAL, count);

    if (!fisher_list) {
        fprintf(stderr, "Failed to create benchmark lists.\n");
        free_list(fisher_list);
        return 1;
    }

    srand(seed);
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime(start) failed");
        free_list(fisher_list);
        return 1;
    }
    shuffle_with_array(&fisher_list);
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        perror("clock_gettime(end) failed");
        free_list(fisher_list);
        return 1;
    }
    fisher_sec = elapsed_seconds(&start, &end);

    printf("count=%d\n", count);
    printf("seed=%u\n", seed);
    printf("fisher_yates_like_shuffle: %.6f sec\n", fisher_sec);

    free_list(fisher_list);
    return 0;
}