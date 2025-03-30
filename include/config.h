#ifndef CONFIG_H_IMPLEMENTATION
#define CONFIG_H_IMPLEMENTATION

#include <stdbool.h>

typedef struct {
  char *ip;
  unsigned int port;
} networkConfig;

typedef struct {
  bool enabled;
  unsigned int quality;
  unsigned int min_size;
} compressionConfig;

typedef struct {
  bool enabled;
  bool mem_cached;
  char *cert_path;
  char *key_path;
} sslConfig;

typedef struct {
  char *site_root;
  enum { File, Console, Both } log_type;
  networkConfig network;
  compressionConfig compression;
  sslConfig ssl;
} Config;

int init_config(Config *config);
int free_config(Config *config);

int read_config(Config *config);
int write_config(Config *config);

#endif // !CONFIG_H_IMPLEMENTATION
