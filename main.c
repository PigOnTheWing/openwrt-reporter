#include <stdio.h>
#include <unistd.h>
#include <libubus.h>
#include <mosquitto.h>

#include "conf_operations.h"

/* Default value for period, in seconds */
#define DEFAULT_PERIOD 10
#define TO_MSEC(sec) (sec * 1000)

static struct ubus_context *ctx = NULL;
static struct uci_context *uci_ctx = NULL;
static struct mosquitto *mosq = NULL;

struct report_vals
{
  unsigned long utime;
  unsigned long stime;
  long rss;
};

enum 
{
  FILENAME,
  SECTION_NAME,
  PERIOD,
  ENABLED,
  __MAX_OPT
};

static int report_period = DEFAULT_PERIOD;

/* UCI conf file params */
static const char conf_filename[] = "reporter_conf";
static const char conf_section_name[] = "reporter";

static bool g_mosq_connected = false;

static void reporter_report_cb(struct uloop_timeout *tmo);
static int reporter_report(struct ubus_context *ctx, struct ubus_object *obj,
		                       struct ubus_request_data *req, const char *method,
		                       struct blob_attr *msg);

enum {
	REPORT_PERIOD,
	__REPORT_MAX
};

static const struct blobmsg_policy report_policy[__REPORT_MAX] = {
	[REPORT_PERIOD] = { .name = "period", .type = BLOBMSG_TYPE_INT32 },
};

struct report_req
{
  struct ubus_request_data req_data;
  struct uloop_timeout tmo;
};

static const struct ubus_method reporter_methods[] = {
	UBUS_METHOD("report", reporter_report, report_policy),
};

static struct ubus_object_type reporter_object_type =
	UBUS_OBJECT_TYPE("reporter", reporter_methods);

static struct ubus_object reporter_object = {
	.name = "reporter",
	.type = &reporter_object_type,
	.methods = reporter_methods,
	.n_methods = ARRAY_SIZE(reporter_methods),
};

static void get_stats(struct report_vals *vals)
{
  /* Variables to use in fscanf call for values in stat that need to be skipped */
  char c, s[16];
  int i = 1, ii; /* i as a loop counter */
  long l;
  long long ll;
  FILE *f;

  f = fopen("/proc/self/stat", "r");
  if (!f)
  {
    fprintf(stderr, "Failed to open \"/proc/self/stat\"\n");
    return;
  }

  while(i <= 24)
  {
    switch (i)
    {
      case 1:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        fscanf(f, "%d", &ii);
        break;
      case 2:
        fscanf(f, "%s", s);
        break;
      case 3:
        fscanf(f, " %c", &c);
        break;
      case 9:
        fscanf(f, "%u", &ii);
        break;
      case 10:
      case 11:
      case 12:
      case 13:
      case 23:
        fscanf(f, "%lu", &l);
        break;
      case 14:
        fscanf(f, "%lu", &vals->utime);
        break;
      case 15:
        fscanf(f, "%lu", &vals->stime);
        break;
      case 16:
      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
        fscanf(f, "%ld", &l);
        break;
      case 22:
        fscanf(f, "%llu", &ll);
        break;
      case 24:
        fscanf(f, "%ld", &vals->rss);
        break;
      default:
        break;
    }
    ++i;
  }

  fclose(f);
}

static void send_report(struct report_vals *vals)
{
  int rc = 0;
  char pl[60], *topic = NULL;

  topic = conf_get_string(uci_ctx, conf_filename, conf_section_name, "topic");
  if (topic == NULL)
  {
    fprintf(stderr, "Failed to read config, abort\n");
    return;
  }

  if (!g_mosq_connected)
  {
    fprintf(stderr, "Not connected to MQTT server, abort\n");
    return;
  }

  sprintf(pl, "%lu %lu %ld", vals->utime, vals->stime, vals->rss);
  rc = mosquitto_publish(mosq, NULL, topic, strlen(pl), pl, 2, false);
  if (rc != MOSQ_ERR_SUCCESS)
  {
    fprintf(stderr, "Failed to publish, error - %s\n", mosquitto_strerror(rc));
    return;
  }
}


static void reporter_report_cb(struct uloop_timeout *tmo)
{
  /* Report attributes */
  struct report_vals vals;

  fprintf(stderr, "Report attrs start\n");
  get_stats(&vals);
  fprintf(stderr, "Current stats:\nutime=%lu stime=%lu rss=%ld\n", vals.utime, vals.stime, vals.rss);
  send_report(&vals);

  /* Re-set the timeout*/
  uloop_timeout_set(tmo, TO_MSEC(report_period));
  fprintf(stderr, "Report attrs end\n");
}

static int reporter_report(struct ubus_context *ctx, struct ubus_object *obj,
		                       struct ubus_request_data *req, const char *method,
		                       struct blob_attr *msg)
{
  static struct uloop_timeout *report_tmo = NULL;
	struct blob_attr *tb[__REPORT_MAX];

	blobmsg_parse(report_policy, ARRAY_SIZE(report_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[REPORT_PERIOD])
  {
    report_period = blobmsg_get_u32(tb[REPORT_PERIOD]);
  }
  else
  {
    /* No period specified, fallback to default */
    report_period = DEFAULT_PERIOD;
  }

  if (report_period == 0 && report_tmo != NULL)
  {
    /* Disable reporting */
    uloop_timeout_cancel(report_tmo);
    free(report_tmo);
    report_tmo = NULL;

    /* Use strings as params to skip additional parsing calls */
    conf_set_string(uci_ctx, conf_filename, conf_section_name, "enabled", "0");
    conf_set_string(uci_ctx, conf_filename, conf_section_name, "period", "0");
    return 0;
  }

  if (report_tmo == NULL)
  {
    /* Reporting was off, alloc new timeout */
	  report_tmo = calloc(1, sizeof(struct uloop_timeout));
    conf_set_string(uci_ctx, conf_filename, conf_section_name, "enabled", "1");
  }

	report_tmo->cb = reporter_report_cb;
	uloop_timeout_set(report_tmo, TO_MSEC(report_period));
  conf_set_decimal(uci_ctx, conf_filename, conf_section_name, "period", report_period);

	return 0;
}

void on_connect(struct mosquitto *mosq, void *unused, int rc)
{
  g_mosq_connected = true;
  fprintf(stderr, "Connected\n");
}

void on_disconnect(struct mosquitto *mosq, void *unused, int rc)
{
  g_mosq_connected = false;
  fprintf(stderr, "Disconnected\n");
}

void on_publish(struct mosquitto *mosq, void *unused, int rc)
{
  fprintf(stderr, "Published\n");
}

int mosq_init(const char *host, int port, int keepalive)
{
  int rc = 0;

  mosquitto_lib_init();

  /* Init client with random Id and no additional params */
  mosq = mosquitto_new(NULL, true, NULL);
  if (!mosq)
  {
    fprintf(stderr, "Failed to alloc mosquitto client!\n");
    return -1;
  }

  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_publish_callback_set(mosq, on_publish);
  mosquitto_disconnect_callback_set(mosq, on_disconnect);

  rc = mosquitto_connect(mosq, host, port, keepalive);
  if (rc != MOSQ_ERR_SUCCESS)
  {
		mosquitto_destroy(mosq);
    fprintf(stderr, "Failed to connect to MQTT broker, err - %s\n", mosquitto_strerror(rc));
    return -1;
  }

  rc = mosquitto_loop_start(mosq);
  if(rc != MOSQ_ERR_SUCCESS)
  {
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return -1;
	}

  return 0;
}

int main_start(const char *ubus_socket)
{
  int ret = 0;
  const char *host = NULL;

  uci_ctx = uci_alloc_context();
  if (uci_ctx == NULL)
  {
    fprintf(stderr, "Failed to alloc uci context!\n");
    return -1;
  }

  host = conf_get_string(uci_ctx, conf_filename, conf_section_name, "host");

  if (host == NULL)
  {
    fprintf(stderr, "Error reading conf file\n");
    return -1;
  }

  if (mosq_init(host, 1883, 60))
  {
    return -1;
  }

	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	ret = ubus_add_object(ctx, &reporter_object);
	  
  if (ret)
  {
	  fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
    return -1;
  }

	uloop_run();
}

void main_deinit(void)
{
  uci_free_context(uci_ctx);

  mosquitto_disconnect(mosq);
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

	ubus_free(ctx);
	uloop_done();
}

int main(int argc, char **argv)
{
  const char *ubus_socket = NULL;
	int ch;
  int ret = 0;

	while ((ch = getopt(argc, argv, "c:s:")) != -1) {
		switch (ch) {
    case 's':
			ubus_socket = optarg;
			break;
		default:
			break;
		}
	}

  ret = main_start(ubus_socket);
  main_deinit();

	return ret;
}
