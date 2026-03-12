#include "single_pointer.h"

struct list_node *single_pointer_middle_node(struct list_node *head)
{
    struct list_node *cur = head;
    int n = 0;
    while (cur) {
        ++n;
        cur = cur->next;
    }
    int k = 0;
    cur = head;
    while (k < n / 2) {
        ++k;
        cur = cur->next;
    }
    return cur;
}
