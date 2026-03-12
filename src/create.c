#include "create.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef CREATE_NO_MAIN
static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <mode> <count>\n", prog);
    fprintf(stderr, "  mode: contiguous | sequential | random | 1 | 2 | 3\n");
    fprintf(stderr, "  count: number of nodes (> 0)\n");
}
#endif

int create_parse_mode(const char *mode_str)
{
    if (!strcmp(mode_str, "1") || !strcmp(mode_str, "contiguous"))
        return MODE_CONTIGUOUS;
    if (!strcmp(mode_str, "2") || !strcmp(mode_str, "sequential"))
        return MODE_SEQUENTIAL;
    if (!strcmp(mode_str, "3") || !strcmp(mode_str, "random"))
        return MODE_RANDOM;
    return 0;
}

int create_parse_count(const char *count_str, int *out)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(count_str, &end, 10);
    if (errno || *count_str == '\0' || *end != '\0')
        return -1;
    if (value <= 0 || value > INT_MAX)
        return -1;

    *out = (int)value;
    return 0;
}

static void shuffle_int_array(int *arr, int n)
{
    int i;
    for (i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

static struct list_node *create_contiguous(int n)
{
    int i;
    struct list_node *pool = malloc((size_t)n * sizeof(*pool));

    if (!pool)
        return NULL;

    for (i = 0; i < n; ++i) {
        pool[i].val = i;
        pool[i].next = (i + 1 < n) ? &pool[i + 1] : NULL;
    }
    return pool;
}

static struct list_node *create_sequential(int n)
{
    int i;
    struct list_node *head = NULL;
    struct list_node *tail = NULL;

    for (i = 0; i < n; ++i) {
        struct list_node *node = malloc(sizeof(*node));
        if (!node) {
            struct list_node *cur = head;
            while (cur) {
                struct list_node *next = cur->next;
                free(cur);
                cur = next;
            }
            return NULL;
        }

        node->val = i;
        node->next = NULL;
        if (!head)
            head = node;
        else
            tail->next = node;
        tail = node;
    }
    return head;
}

static struct list_node *create_randomized(int n)
{
    int i;
    struct list_node *head;
    struct list_node **nodes = malloc((size_t)n * sizeof(*nodes));
    int *perm = malloc((size_t)n * sizeof(*perm));

    if (!nodes || !perm) {
        free(nodes);
        free(perm);
        return NULL;
    }

    for (i = 0; i < n; ++i) {
        nodes[i] = malloc(sizeof(**nodes));
        if (!nodes[i]) {
            while (--i >= 0)
                free(nodes[i]);
            free(nodes);
            free(perm);
            return NULL;
        }
        nodes[i]->val = i;
        nodes[i]->next = NULL;
        perm[i] = i;
    }

    shuffle_int_array(perm, n);
    for (i = 0; i < n - 1; ++i)
        nodes[perm[i]]->next = nodes[perm[i + 1]];
    nodes[perm[n - 1]]->next = NULL;

    head = nodes[perm[0]];
    free(nodes);
    free(perm);
    return head;
}

struct list_node *create_list(int mode, int n)
{
    if (mode == MODE_CONTIGUOUS)
        return create_contiguous(n);
    if (mode == MODE_SEQUENTIAL)
        return create_sequential(n);
    if (mode == MODE_RANDOM)
        return create_randomized(n);
    return NULL;
}

void destroy_list(struct list_node *head, int mode)
{
    if (!head)
        return;

    if (mode == MODE_CONTIGUOUS) {
        free(head);
        return;
    }

    while (head) {
        struct list_node *next = head->next;
        free(head);
        head = next;
    }
}

const char *create_mode_name(int mode)
{
    if (mode == MODE_CONTIGUOUS)
        return "contiguous";
    if (mode == MODE_SEQUENTIAL)
        return "sequential";
    if (mode == MODE_RANDOM)
        return "random";
    return "unknown";
}

#ifndef CREATE_NO_MAIN
int main(int argc, char **argv)
{
    int mode;
    int count;
    int preview = 10;
    int shown = 0;
    struct list_node *head;
    struct list_node *cur;

    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    mode = create_parse_mode(argv[1]);
    if (!mode) {
        usage(argv[0]);
        return 1;
    }

    if (create_parse_count(argv[2], &count) < 0) {
        usage(argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));
    head = create_list(mode, count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    printf("mode=%s, count=%d\n", create_mode_name(mode), count);
    printf("head=%p\n", (void *)head);
    printf("preview(first %d nodes):\n", preview);

    cur = head;
    while (cur && shown < preview) {
        printf("  [%d] node=%p val=%d next=%p\n",
               shown, (void *)cur, cur->val, (void *)cur->next);
        cur = cur->next;
        ++shown;
    }

    destroy_list(head, mode);
    return 0;
}
#endif
