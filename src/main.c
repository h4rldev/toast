/* toast
 * Copyright (C) 2025  enstore.cloud
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <h2o.h>
#include <h2o/file.h>
#include <h2o/http1.h>
#include <h2o/http2.h>
#include <h2o/memcached.h>

#include <api.h>
#include <cli.h>
#include <config.h>
#include <file.h>
#include <meta.h>

#ifdef API_H_IMPLEMENTATION
static h2o_pathconf_t *
register_handler(h2o_hostconf_t *hostconf, const char *path,
                 int (*on_req)(h2o_handler_t *, h2o_req_t *)) {
  h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, path, 0);
  h2o_handler_t *handler = h2o_create_handler(pathconf, sizeof(*handler));
  handler->on_req = on_req;
  return pathconf;
}
#endif

static h2o_globalconf_t config;
static h2o_context_t ctx;
static h2o_multithread_receiver_t libmemcached_receiver;
static h2o_accept_ctx_t accept_ctx;

static void on_accept(uv_stream_t *listener, int status) {
  uv_tcp_t *conn;
  h2o_socket_t *sock;

  if (status != 0)
    return;

  conn = h2o_mem_alloc(sizeof(*conn));
  uv_tcp_init(listener->loop, conn);

  if (uv_accept(listener, (uv_stream_t *)conn) != 0) {
    uv_close((uv_handle_t *)conn, (uv_close_cb)free);
    return;
  }

  sock = h2o_uv_socket_create((uv_handle_t *)conn, (uv_close_cb)free);
  h2o_accept(&accept_ctx, sock);
}

static int create_listener(char *ip, unsigned int port) {
  static uv_tcp_t listener;
  struct sockaddr_in addr;
  int r;

  uv_tcp_init(ctx.loop, &listener);
  uv_ip4_addr(ip, port, &addr);
  if ((r = uv_tcp_bind(&listener, (struct sockaddr *)&addr, 0)) != 0) {
    fprintf(stderr, "uv_tcp_bind:%s\n", uv_strerror(r));
    goto Error;
  }
  if ((r = uv_listen((uv_stream_t *)&listener, 128, on_accept)) != 0) {
    fprintf(stderr, "uv_listen:%s\n", uv_strerror(r));
    goto Error;
  }

  return 0;
Error:
  uv_close((uv_handle_t *)&listener, NULL);
  return r;
}

static int setup_ssl(const char *cert_file, const char *key_file,
                     const char *ciphers, char *ip, bool use_memcached) {
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  accept_ctx.ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  SSL_CTX_set_options(accept_ctx.ssl_ctx, SSL_OP_NO_SSLv2);

  if (use_memcached == true) {
    accept_ctx.libmemcached_receiver = &libmemcached_receiver;
    h2o_accept_setup_memcached_ssl_resumption(
        h2o_memcached_create_context(ip, 11211, 0, 1, "h2o:ssl-resumption:"),
        86400);
    h2o_socket_ssl_async_resumption_setup_ctx(accept_ctx.ssl_ctx);
  }

#ifdef SSL_CTX_set_ecdh_auto
  SSL_CTX_set_ecdh_auto(accept_ctx.ssl_ctx, 1);
#endif

  /* load certificate and private key */
  if (SSL_CTX_use_certificate_chain_file(accept_ctx.ssl_ctx, cert_file) != 1) {
    fprintf(
        stderr,
        "an error occurred while trying to load server certificate file:%s\n",
        cert_file);
    return -1;
  }
  if (SSL_CTX_use_PrivateKey_file(accept_ctx.ssl_ctx, key_file,
                                  SSL_FILETYPE_PEM) != 1) {
    fprintf(stderr,
            "an error occurred while trying to load private key file:%s\n",
            key_file);
    return -1;
  }

  if (SSL_CTX_set_cipher_list(accept_ctx.ssl_ctx, ciphers) != 1) {
    fprintf(stderr, "ciphers could not be set: %s\n", ciphers);
    return -1;
  }

/* setup protocol negotiation methods */
#if H2O_USE_NPN
  h2o_ssl_register_npn_protocols(accept_ctx.ssl_ctx, h2o_http2_npn_protocols);
#endif
#if H2O_USE_ALPN
  h2o_ssl_register_alpn_protocols(accept_ctx.ssl_ctx, h2o_http2_alpn_protocols);
#endif

  return 0;
}

static int get_year(void) {
  time_t time_container = time(NULL);
  struct tm *time = localtime(&time_container);
  return time->tm_year + 1900;
}

static int get_index(h2o_handler_t *handler, h2o_req_t *req) {
  h2o_generator_t generator = {NULL, NULL};

  char *html_buffer = malloc(1024);
  sprintf(html_buffer,
          "<title>Welcome to toast!</title>"
          "<style>"
          "body{font-family: sans-serif;}"
          "h1{font-size: 1.5em;}"
          "p{font-size: 1em; margin-bottom: 0.5em;}"
          "sub{font-size: 0.7em;}"
          "</style>"
          "<h1>Welcome to toast!</h1>"
          "<p>If you are seeing this, it means that toast is working "
          "but might be missing files in site root!</p>"
          "<sub><a href=\"https://git.enstore.cloud/enstore.cloud/toast\" "
          "target=\"_blank\">toast</a>, %d</sub>",
          get_year());

  h2o_iovec_t body = h2o_strdup(&req->pool, html_buffer, strlen(html_buffer));
  free(html_buffer);

  req->res.status = 200;
  req->res.reason = "OK";
  h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL,
                 H2O_STRLIT("text/html; charset=utf-8"));
  h2o_start_response(req, &generator);
  h2o_send(req, &body, 1, 1);

  return 0;
}

int main(int argc, char **argv) {
  Config server_config;

  time_t time_container = time(NULL);
  struct tm *time = localtime(&time_container);
  char log_fname[28] = {0};

  uv_loop_t loop;

  snprintf(log_fname, 28, "./logs/toast-%d-%02d-%02d.log", time->tm_year + 1900,
           time->tm_mon + 1, time->tm_mday);

  if (read_config(&server_config) != 0) {
    init_config(&server_config);
    write_config(&server_config);
  }

  h2o_access_log_filehandle_t *log2file = NULL;
  h2o_access_log_filehandle_t *logfh = NULL;

  switch (server_config.log_type) {
  case Both:
    if (path_exist("./logs/") == false)
      make_dir("./logs/");
    log2file =
        h2o_access_log_open_handle(log_fname, NULL, H2O_LOGCONF_ESCAPE_APACHE);
    logfh = h2o_access_log_open_handle("/dev/stdout", NULL,
                                       H2O_LOGCONF_ESCAPE_APACHE);
    break;
  case File:
    if (path_exist("./logs/") == false)
      make_dir("./logs/");
    log2file =
        h2o_access_log_open_handle(log_fname, NULL, H2O_LOGCONF_ESCAPE_APACHE);
    break;

  case Console:
    logfh = h2o_access_log_open_handle("/dev/stdout", NULL,
                                       H2O_LOGCONF_ESCAPE_APACHE);
    break;
  }

  h2o_hostconf_t *hostconf;
  h2o_pathconf_t *pathconf;

  signal(SIGPIPE, SIG_IGN);

  if (parse_args(&argc, &argv, &server_config) != 0) {
    fprintf(stderr, "toast: failed parsing args, something is very wrong..\n");
    free_config(&server_config);
    return -1;
  }

  h2o_config_init(&config);
  h2o_compress_register_configurator(&config);

  hostconf = h2o_config_register_host(
      &config, h2o_iovec_init(H2O_STRLIT("default")), 65535);

#ifdef API_H_IMPLEMENTATION
  pathconf = register_handler(hostconf, "/api/serverinfo", get_server_info);
  if (logfh != NULL)
    h2o_access_log_register(pathconf, logfh);

  if (log2file != NULL)
    h2o_access_log_register(pathconf, log2file);

  pathconf = register_handler(hostconf, "/api/uptime", get_uptime);
  if (logfh != NULL)
    h2o_access_log_register(pathconf, logfh);

  if (log2file != NULL)
    h2o_access_log_register(pathconf, log2file);

  pathconf = register_handler(hostconf, "/api/cv", get_cv);
  if (logfh != NULL)
    h2o_access_log_register(pathconf, logfh);

  if (log2file != NULL)
    h2o_access_log_register(pathconf, log2file);
#endif
  char *index_path = malloc(1024);
  sprintf(index_path, "%s/index.html", server_config.site_root);

  if (path_exist(index_path) == false) {
    pathconf = register_handler(hostconf, "/", get_index);
    if (logfh != NULL)
      h2o_access_log_register(pathconf, logfh);
    if (log2file != NULL)
      h2o_access_log_register(pathconf, log2file);
  } else {
    pathconf = h2o_config_register_path(hostconf, "/", 0);
    h2o_file_register(pathconf, server_config.site_root, NULL, NULL, 0);
    if (logfh != NULL)
      h2o_access_log_register(pathconf, logfh);

    if (log2file != NULL)
      h2o_access_log_register(pathconf, log2file);
  }

  free(index_path);

  if (server_config.compression.enabled == false)
    goto NotCompress;

  h2o_compress_args_t ca;
  ca.gzip.quality = server_config.compression.quality;

  if (server_config.compression.min_size != 0)
    ca.min_size = server_config.compression.min_size;

  h2o_compress_register(pathconf, &ca);
  if (logfh != NULL)
    h2o_access_log_register(pathconf, logfh);

  if (log2file != NULL)
    h2o_access_log_register(pathconf, log2file);

NotCompress:
  uv_loop_init(&loop);
  h2o_context_init(&ctx, &loop, &config);

  if (server_config.ssl.mem_cached == true)
    h2o_multithread_register_receiver(ctx.queue, &libmemcached_receiver,
                                      h2o_memcached_receiver);

  if (server_config.ssl.enabled == true &&
      setup_ssl(server_config.ssl.cert_path, server_config.ssl.key_path,
                "DEFAULT:!MD5:!DSS:!DES:!RC4:!RC2:!SEED:!IDEA:!"
                "NULL:!ADH:!EXP:!SRP:!PSK",
                server_config.network.ip, server_config.ssl.mem_cached) != 0)
    goto Error;

  accept_ctx.ctx = &ctx;
  accept_ctx.hosts = config.hosts;
  config.server_name = h2o_iovec_init(H2O_STRLIT("toast"));

  if (create_listener(server_config.network.ip, server_config.network.port) !=
      0) {
    fprintf(stderr, "failed to listen to 127.0.0.1:%u:%s\n",
            server_config.network.port, strerror(errno));
    goto Error;
  }

  printf("toast: running on %s:%u\n", server_config.network.ip,
         server_config.network.port);
  free_config(&server_config);
  init_start_date();
  uv_run(ctx.loop, UV_RUN_DEFAULT);

Error:
  return -1;
}
