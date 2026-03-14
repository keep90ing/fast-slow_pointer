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

    for (int i = 1; i <= n; i++) {
        
        struct list_node *new_node = malloc(sizeof(struct list_node));
        if (!new_node) {
            struct list_node *cur = head;
            while (cur) {
                struct list_node *temp = cur;
                cur = cur->next;
                free(temp);
            }
            return NULL; 
        }
        
        // initialize the new node
        new_node->val = i;
        new_node->next = NULL;

        // link the new node to the list
        *indirect = new_node;

        // move the indirect pointer to the next field of the new node
        indirect = &(*indirect)->next;
    }

    return head;
}

static void fisher_yates_shuffle(struct list_node **head)
{
    // First, we have to know how long is the linked list
    int len = 0;
    struct list_node **indirect = head;
    while (*indirect) {
        len++;
        indirect = &(*indirect)->next;
    }   

    // Append shuffling result to another linked list
    struct list_node *new = NULL;
    struct list_node **new_head = &new;
    struct list_node **new_tail = &new;

    while (len) {
        int random = rand() % len;
        indirect = head;

        while (random--)
            indirect = &(*indirect)->next;

        struct list_node *tmp = *indirect;
        *indirect = (*indirect)->next;

        tmp->next = NULL;
        if (new) {
            (*new_tail)->next = tmp;
            new_tail = &(*new_tail)->next;
        } else {
            new = tmp;
        }

        len--;
    }   

    *head = *new_head;
}

static struct list_node *create_sequential(int n)
{
    struct list_node *head = build_list(n);
    return head;
}

static struct list_node *create_randomized(int n)
{
    struct list_node *head = build_list(n);
    fisher_yates_shuffle(&head);
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
