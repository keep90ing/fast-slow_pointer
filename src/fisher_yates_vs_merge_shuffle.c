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

static void fisher_yates_shuffle(struct list_node **head)
{
    int len = 0;
    struct list_node **indirect = head;
    struct list_node *new_head = NULL;
    struct list_node **tail_next = &new_head;

    while (*indirect) {
        ++len;
        indirect = &(*indirect)->next;
    }

    while (len > 0) {
        int random = rand() % len;

        indirect = head;
        while (random-- > 0)
            indirect = &(*indirect)->next;

        *tail_next = *indirect;
        *indirect = (*indirect)->next;
        (*tail_next)->next = NULL;
        tail_next = &(*tail_next)->next;
        --len;
    }

    *head = new_head;
}

static void split_list(struct list_node *source, struct list_node **front,
                       struct list_node **back)
{
    struct list_node *fast = source->next;
    struct list_node *slow = source;

    while (fast) {
        fast = fast->next;
        if (fast) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = source;
    *back = slow->next;
    slow->next = NULL;
}

static struct list_node *random_merge(struct list_node *a, struct list_node *b)
{
    struct list_node dummy;
    struct list_node *tail = &dummy;

    dummy.next = NULL;
    while (a && b) {
        if (rand() & 1) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }

    tail->next = a ? a : b;
    return dummy.next;
}

static void merge_shuffle(struct list_node **head)
{
    struct list_node *front;
    struct list_node *back;

    if (!head || !*head || !(*head)->next)
        return;

    split_list(*head, &front, &back);
    merge_shuffle(&front);
    merge_shuffle(&back);
    *head = random_merge(front, back);
}

int main(int argc, char **argv)
{
    int count;
    unsigned int seed;
    struct list_node *fisher_list, *merge_list;
    struct timespec start, end;
    double fisher_sec, merge_sec;

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
    merge_list = create_list(MODE_SEQUENTIAL, count);
    if (!fisher_list || !merge_list) {
        fprintf(stderr, "Failed to create benchmark lists.\n");
        free_list(fisher_list);
        free_list(merge_list);
        return 1;
    }

    srand(seed);
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime(start) failed");
        free_list(fisher_list);
        free_list(merge_list);
        return 1;
    }
    fisher_yates_shuffle(&fisher_list);
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        perror("clock_gettime(end) failed");
        free_list(fisher_list);
        free_list(merge_list);
        return 1;
    }
    fisher_sec = elapsed_seconds(&start, &end);

    srand(seed);
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime(start) failed");
        free_list(fisher_list);
        free_list(merge_list);
        return 1;
    }
    merge_shuffle(&merge_list);
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        perror("clock_gettime(end) failed");
        free_list(fisher_list);
        free_list(merge_list);
        return 1;
    }
    merge_sec = elapsed_seconds(&start, &end);

    printf("count=%d\n", count);
    printf("seed=%u\n", seed);
    printf("fisher_yates_like_shuffle: %.6f sec\n", fisher_sec);
    printf("merge_shuffle: %.6f sec\n", merge_sec);

    free_list(fisher_list);
    free_list(merge_list);
    return 0;
}
