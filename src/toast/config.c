#include <jansson.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <config.h>
#include <file.h>

#define DEFAULT_PATH "/config/"
#define DEFAULT_LEVEL 6

static int handle_parse_err(char *categ, char *field) {
  fprintf(stderr,
          "JSON didn't read properly, something went wrong on category %s, "
          "field %s\n",
          categ, field);
  return -1;
}

int init_config(Config *config) {
  Config local_config;
  networkConfig local_network;
  sslConfig local_ssl; // remember SSL actually means TLS
  compressionConfig local_compression;

  // Max length for ipv4 is 15 characters, adding 1 for nullterm.
  local_network.ip = (char *)malloc(16);
  strlcpy(local_network.ip, "127.0.0.1", 16);
  local_network.port = 8080;

  local_compression.enabled = true; // Compression is gzip
  local_compression.quality = 6;    // 6 is middleground and relatively fast
  local_compression.min_size = 150; // 150 recommended minimum

  local_ssl.enabled = false;
  local_ssl.mem_cached = false;
  local_ssl.cert_path = (char *)malloc(1024); // 1024 is usual max path for *nix
  local_ssl.key_path = (char *)malloc(1024);
  local_ssl.cert_path = NULL; // NULL by default
  local_ssl.key_path = NULL;  // writes as "" to file anyway.

  // Default site root is ./site/, can be relative and canonical path
  local_config.site_root = (char *)malloc(1024);
  strlcpy(local_config.site_root, "site/", 1024);

  local_config.log_type = Both; // Console, File, Both are the available options
  local_config.network = local_network;
  local_config.compression = local_compression;
  local_config.ssl = local_ssl;

  *config = local_config;

  return 0;
}

int write_config(Config *config) {
  char *default_path = DEFAULT_PATH;
  char *cwd = get_cwd();
  char *path = (char *)malloc(1024);
  char cwd_buf[1024] = {0};

  if (!cwd || !path)
    return -1;

  strlcpy(cwd_buf, cwd, 1024);
  free(cwd);

  snprintf(path, 1024, "%s%s", cwd_buf, default_path);

  if (path_exist(path) == false) {
    if (make_dir(path) != 0) {
      free(path);
      return -1;
    }
  }

  strlcat(path, "config.json", 1024);

  if (config == NULL)
    return -1;

  json_t *root = json_object();
  json_t *network_object = json_object();
  json_t *compression_object = json_object();
  json_t *ssl_object = json_object();

  char *site_root = config->site_root;
  json_object_set_new(root, "site_root", json_string(site_root));

  switch (config->log_type) {
  case Both:
    json_object_set_new(root, "log_type", json_string("both"));
    break;
  case File:
    json_object_set_new(root, "log_type", json_string("file"));
    break;
  case Console:
    json_object_set_new(root, "log_type", json_string("console"));
    break;
  default:
    json_decref(root);
    return -1;
  }

  if (config->network.ip == NULL)
    json_object_set_new(network_object, "ip", json_string("127.0.0.1"));
  else
    json_object_set_new(network_object, "ip", json_string(config->network.ip));

  if (config->network.port == 0 || config->network.port > 65535)
    json_object_set_new(network_object, "port", json_integer(8080));
  else
    json_object_set_new(network_object, "port",
                        json_integer(config->network.port));

  if (config->compression.enabled == true)
    json_object_set_new(compression_object, "enabled", json_true());
  else
    json_object_set_new(compression_object, "enabled", json_false());

  if (config->compression.quality > 9)
    json_object_set_new(compression_object, "quality",
                        json_integer(DEFAULT_LEVEL));
  else if (config->compression.quality == 0 &&
           config->compression.enabled == true)
    json_object_set_new(compression_object, "quality",
                        json_integer(DEFAULT_LEVEL));
  else
    json_object_set_new(compression_object, "quality",
                        json_integer(config->compression.quality));

  json_object_set_new(compression_object, "min_size",
                      json_integer(config->compression.min_size));

  if (config->ssl.enabled == true)
    json_object_set_new(ssl_object, "enabled", json_true());
  else
    json_object_set_new(ssl_object, "enabled", json_false());

  if (config->ssl.mem_cached == true)
    json_object_set_new(ssl_object, "mem_cached", json_true());
  else
    json_object_set_new(ssl_object, "mem_cached", json_false());

  if (config->ssl.cert_path == NULL) {
    json_object_set_new(ssl_object, "cert_path", json_string(""));
    json_object_set(ssl_object, "enabled", json_false());
  } else {
    json_object_set_new(ssl_object, "cert_path",
                        json_string(config->ssl.cert_path));
  }

  if (config->ssl.key_path == NULL) {
    json_object_set_new(ssl_object, "key_path", json_string(""));
    json_object_set(ssl_object, "enabled", json_false());
  } else {
    json_object_set_new(ssl_object, "key_path",
                        json_string(config->ssl.key_path));
  }

  json_object_set_new(root, "network", network_object);
  json_object_set_new(root, "compression", compression_object);
  json_object_set_new(root, "ssl", ssl_object);

  FILE *file = fopen(path, "w");
  json_dumpf(root, file, JSON_INDENT(2));

  fclose(file);
  json_decref(root);
  free(path);

  return 0;
}

int read_config(Config *config) {
  json_t *root;
  json_error_t error;

  char *default_path = DEFAULT_PATH;

  char *cwd = get_cwd();
  if (!cwd) {
    return -1;
  }

  char *path = (char *)malloc(1024);
  if (!path) {
    free(cwd);
    return -1;
  }

  snprintf(path, 1024, "%s%sconfig.json", cwd, default_path);
  free(cwd);

  if (!path_exist(path)) {
    return -1;
  }

  root = json_load_file(path, 0, &error);
  if (!root) {
    fprintf(stderr, "Error parsing json on line %d: %s\n", error.line,
            error.text);
    free(path);
    return -1;
  }

  free(path);

  json_t *site_root_string = json_object_get(root, "site_root");

  char *site_root = (char *)malloc(1024);
  if (!site_root) {
    json_decref(root);
    return -1;
  }

  if (json_is_string(site_root_string)) {
    strlcpy(site_root, json_string_value(site_root_string), 1024);
  } else {
    json_decref(root);
    free(site_root);

    return handle_parse_err("root", "site_root");
  }

  json_t *log_type_enum = json_object_get(root, "log_type");

  int log_type = Both;
  if (json_is_string(log_type_enum)) {
    if (strncasecmp("both", json_string_value(log_type_enum), 4) == 0)
      log_type = Both;
    else if (strncasecmp("file", json_string_value(log_type_enum), 4) == 0)
      log_type = File;
    else
      log_type = Console;
  } else {
    json_decref(root);
    free(site_root);
    return handle_parse_err("root", "log_type");
  }

  json_t *network_object = json_object_get(root, "network");
  if (!json_is_object(network_object)) {
    json_decref(root);
    free(site_root);
    return -1;
  }

  json_t *ip_string = json_object_get(network_object, "ip");

  char *ip = (char *)malloc(16);
  if (!ip) {
    json_decref(root);
    free(site_root);
    return -1;
  }

  if (json_is_string(ip_string)) {
    strlcpy(ip, json_string_value(ip_string), 16);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("network", "ip");
  }

  json_t *port_integer = json_object_get(network_object, "port");

  unsigned int port = 0;
  if (json_is_integer(port_integer)) {
    port = json_integer_value(port_integer);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("root", "port");
  }

  json_t *compression_object = json_object_get(root, "compression");
  if (!json_is_object(compression_object)) {
    json_decref(root);
    free(site_root);
    free(ip);

    return -1;
  }

  json_t *compression_enabled_bool =
      json_object_get(compression_object, "enabled");

  bool compression_enabled = false;
  if (json_is_boolean(compression_enabled_bool)) {
    compression_enabled = json_boolean_value(compression_enabled_bool);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("compression", "enabled");
  }

  json_t *compression_quality_uint =
      json_object_get(compression_object, "quality");

  unsigned int compression_quality = DEFAULT_LEVEL;
  if (json_is_integer(compression_quality_uint)) {
    compression_quality = json_integer_value(compression_quality_uint);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("compression", "quality");
  }

  json_t *compression_min_size_uint =
      json_object_get(compression_object, "min_size");

  unsigned int compression_min_size = 150;
  if (json_is_integer(compression_min_size_uint)) {
    compression_min_size = json_integer_value(compression_quality_uint);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("compression", "min_size");
  }

  json_t *ssl_object = json_object_get(root, "ssl");
  if (!json_is_object(ssl_object)) {
    json_decref(root);
    free(site_root);
    free(ip);

    return -1;
  }

  json_t *ssl_enabled_bool = json_object_get(ssl_object, "enabled");

  bool ssl_enabled = false;
  if (json_is_boolean(ssl_enabled_bool)) {
    ssl_enabled = json_boolean_value(ssl_enabled_bool);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("ssl", "enabled");
  }

  json_t *mem_cached_bool = json_object_get(ssl_object, "mem_cached");

  bool mem_cached = false;
  if (json_is_boolean(mem_cached_bool)) {
    mem_cached = json_boolean_value(mem_cached_bool);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);

    return handle_parse_err("ssl", "mem_cached");
  }

  json_t *cert_path_string = json_object_get(ssl_object, "cert_path");

  char *cert_path = (char *)malloc(1024);
  if (!cert_path) {
    json_decref(root);
    free(site_root);
    free(ip);

    return -1;
  }

  if (json_is_string(cert_path_string)) {
    strlcpy(cert_path, json_string_value(cert_path_string), 1024);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);
    free(cert_path);

    return handle_parse_err("ssl", "cert_path");
  }

  json_t *key_path_string = json_object_get(ssl_object, "key_path");

  char *key_path = (char *)malloc(1024);
  if (!key_path) {
    free(site_root);
    free(ip);
    free(cert_path);
    json_decref(root);

    return -1;
  }

  if (json_is_string(key_path_string)) {
    strlcpy(key_path, json_string_value(key_path_string), 1024);
  } else {
    json_decref(root);
    free(site_root);
    free(ip);
    free(cert_path);
    free(key_path);

    return handle_parse_err("ssl", "key_path");
  }

  config->site_root = site_root;
  config->log_type = log_type;

  config->network.ip = ip;
  config->network.port = port;

  config->compression.enabled = compression_enabled;
  config->compression.quality = compression_quality;
  config->compression.min_size = compression_min_size;

  config->ssl.enabled = ssl_enabled;
  config->ssl.mem_cached = mem_cached;
  config->ssl.cert_path = cert_path;
  config->ssl.key_path = key_path;

  json_decref(root);

  return 0;
}

int free_config(Config *config) {
  free(config->site_root);
  free(config->network.ip);
  free(config->ssl.cert_path);
  free(config->ssl.key_path);
  return 0;
}
