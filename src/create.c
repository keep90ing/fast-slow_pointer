#include "create.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int create_parse_mode(const char *mode_str)
{
    if (!strcmp(mode_str, "1") || !strcmp(mode_str, "sequential"))
        return MODE_SEQUENTIAL;
    if (!strcmp(mode_str, "2") || !strcmp(mode_str, "random") ||
        !strcmp(mode_str, "randomized"))
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

// build_list creates a simple linked list with values from 1 to n, where each node is allocated separately on the heap.
static struct list_node *build_list(int n) {
    struct list_node *head = NULL;
    struct list_node **indirect = &head;
    struct list_node *new;

    for (int i = 0; i <= n-1; i++) {
        new = malloc(1 * sizeof(*new));
        
        if (!new)
            goto err_free_list; // error handling: jump to centralized cleanup on malloc failure
        
        new->val = i, new->next = NULL;

        *indirect = new;
        indirect = &(*indirect)->next; 
    }

    return head;

err_free_list:
    /* safe cleanup */
    while (head) {
        new = head;
        head = head->next;
        free(new);
    }
    return NULL; 
}

static void shuffle_with_array(struct list_node **head) {
    if (!head || !*head || !(*head)->next) return;

    // Phase 1: compute the length of the linked list
    int len = 0;
    for (struct list_node *curr = *head; curr; curr = curr->next) {
        len++;
    }

    // Phase 2: malloc an array to hold the node pointers
    struct list_node **arr = malloc(len * sizeof(struct list_node *));
    if (!arr) return; // malloc failed

    struct list_node *curr = *head;
    for (int i = 0; i < len; i++) {
        arr[i] = curr;
        curr = curr->next;
    }

    // Phase 3: execute Fisher-Yates shuffle on the array
    for (int i = len - 1; i > 0; i--) {
        int random = rand() % (i + 1);
        struct list_node *tmp = arr[i];
        arr[i] = arr[random];
        arr[random] = tmp;
    }

    // Phase 4: append shuffled nodes to a new linked list
    struct list_node **indirect = head;
    for (int i = 0; i < len; i++) {
        *indirect = arr[i];
        indirect = &(*indirect)->next;
    }
    *indirect = NULL;

    free(arr);
}


static struct list_node *create_sequential(int n)
{
    struct list_node *head = build_list(n);
    return head;
}

static struct list_node *create_randomized(int n)
{
    struct list_node *head = build_list(n);
    shuffle_with_array(&head);
    return head;
}

struct list_node *create_list(int mode, int n)
{
    if (mode == MODE_SEQUENTIAL)
        return create_sequential(n);
    if (mode == MODE_RANDOM)
        return create_randomized(n);
    return NULL;
}

void free_list(struct list_node *head)
{
    if (!head)
        return;

    while (head) {
        struct list_node *temp = head;
        head = head->next;
        free(temp);
    }
}

const char *create_mode_name(int mode)
{
    if (mode == MODE_SEQUENTIAL)
        return "sequential";
    if (mode == MODE_RANDOM)
        return "random";
    return "unknown";
}
