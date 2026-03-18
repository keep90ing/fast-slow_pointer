#ifndef CREATE_H
#define CREATE_H

#include "list.h"

enum layout_mode {
    MODE_SEQUENTIAL = 1,
    MODE_RANDOM = 2
};

int create_parse_mode(const char *mode_str);
int create_parse_count(const char *count_str, int *out);
const char *create_mode_name(int mode);
struct list_node *create_list(int mode, int n);
void free_list(struct list_node *head);

#endif
