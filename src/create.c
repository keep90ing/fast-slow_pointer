#include "create.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

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

static void append_node_indirect(struct list_node *node,
                                 struct list_node ***indirect)
{
    **indirect = node;
    *indirect = &node->next;
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
    struct list_node **indirect = &head;

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
        append_node_indirect(node, &indirect);
    }
    return head;
}

static struct list_node *create_randomized(int n)
{
    int i;
    struct list_node *head = NULL;
    struct list_node **indirect = &head;
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
    for (i = 0; i < n; ++i)
        append_node_indirect(nodes[perm[i]], &indirect);
    *indirect = NULL;

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
