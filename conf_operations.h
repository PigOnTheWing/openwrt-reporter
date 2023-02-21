#ifndef CONF_OPERATIONS_H
#define CONF_OPERATIONS_H

#include <uci.h>

char *conf_get_string(struct uci_context *ctx, const char *file, const char *sec, const char *name);
void conf_set_string(struct uci_context *ctx, const char *file, const char *sec, const char *name, const char *val);
void conf_set_decimal(struct uci_context *ctx, const char *file, const char *sec, const char *name, const int val);

#endif /* CONF_OPERATIONS_H */
