#ifndef CREATE_H
#define CREATE_H

#include "list.h"

enum layout_mode {
    MODE_CONTIGUOUS = 1,
    MODE_SEQUENTIAL = 2,
    MODE_RANDOM = 3
};

int create_parse_mode(const char *mode_str);
int create_parse_count(const char *count_str, int *out);
const char *create_mode_name(int mode);
struct list_node *create_list(int mode, int n);
void destroy_list(struct list_node *head, int mode);

#endif
