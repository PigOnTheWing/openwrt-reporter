#include <stdlib.h>
#include <string.h>

#include "conf_operations.h"

static char *to_str(int val)
{
  int val_len;
  char *str;

  val_len = snprintf(NULL, 0, "%d", val);
  str = calloc(1, val_len + 1);
  sprintf(str, "%d", val);

  return str;
}

static char *conf_create_get_string(const char *file, const char *sec, const char *name)
{
  static const char conf_get_format[] = "%s.%s.%s";
  char *str = NULL;

  str = calloc(1, strlen(conf_get_format) + strlen(file) + strlen(sec) + strlen(name) + 1);
  if (!str)
  {
    fprintf(stderr, "OOM!\n");
  }

  sprintf(str, conf_get_format, file, sec, name);
  return str;
}

char *conf_get_string(struct uci_context *ctx, const char *file, const char *sec, const char *name)
{
  char *str_get = NULL;
  struct uci_ptr ptr;

  str_get = conf_create_get_string(file, sec, name);
  if (uci_lookup_ptr(ctx, &ptr, str_get, false) != UCI_OK)
  {
    fprintf(stderr, "Lookup fail\n");
    free(str_get);
    return NULL;
  }

  free(str_get);
  return ptr.o->v.string;
}

void conf_set_decimal(struct uci_context *ctx, const char *file, const char *sec, const char *name, const int val)
{
  char *val_str = NULL;

  val_str = to_str(val);
  conf_set_string(ctx, file, sec, name, val_str);
  free(val_str);
}

void conf_set_string(struct uci_context *ctx, const char *file, const char *sec, const char *name, const char *val)
{
  static const char conf_set_format[] = "%s=%s";
  char *str = NULL;
  char *str_get = NULL;
  struct uci_ptr ptr;

  fprintf(stderr, "Set UCI conf val start\n");

  str_get = conf_create_get_string(file, sec, name);

  str = calloc(1, strlen(conf_set_format) + strlen(str_get) + strlen(val) + 1);
  if (!str)
  {
    fprintf(stderr, "OOM!\n");
    return;
  }

  sprintf(str, conf_set_format, str_get, val);

  if (uci_lookup_ptr(ctx, &ptr, str, false) != UCI_OK)
  {
    fprintf(stderr, "Lookup fail\n");
    free(str_get);
    free(str);
    return;
  }

  uci_set(ctx, &ptr);
  uci_commit(ctx, &ptr.p, false);
  free(str_get);
  free(str);
  fprintf(stderr, "Set UCI conf val finish\n");
}
