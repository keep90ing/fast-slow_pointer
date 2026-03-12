#include "fast_and_slow.h"

struct list_node *fast_and_slow_middle_node(struct list_node *head)
{
    struct list_node *slow, *fast;
    slow = fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}
