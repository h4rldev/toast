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

#include <jansson.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#include <api.h>
#include <h2o.h>
#include <h2o/version.h>
#include <meta.h>

static struct tm start_date;
void init_start_date(void) {
  time_t now = time(NULL);
  localtime_r(&now, &start_date);
}

static int is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int year, int month) {
  if (month == 1)
    return is_leap_year(year) ? 29 : 28;
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return days[month];
}

static char *format_uptime(time_t current_time, struct tm start_date) {
  char *bufptr = malloc(48);
  struct tm end_date;
  localtime_r(&current_time, &end_date);

  int years = end_date.tm_year - start_date.tm_year;
  int months = end_date.tm_mon - start_date.tm_mon + years * 12;
  int days = end_date.tm_mday - start_date.tm_mday;
  int hours = end_date.tm_hour - start_date.tm_hour;
  int minutes = end_date.tm_min - start_date.tm_min;
  int seconds = end_date.tm_sec - start_date.tm_sec;

  if (days < 0) {
    --months;
    days += days_in_month(start_date.tm_year, start_date.tm_mon);
  }

  if (seconds < 0) {
    seconds += 60;
    --minutes;
  }
  if (minutes < 0) {
    minutes += 60;
    --hours;
  }
  if (hours < 0) {
    hours += 24;
    --days;
  }

  snprintf(bufptr, 48, "%dm %dd %02d:%02d:%02d", months, days, hours, minutes,
           seconds);

  return bufptr;
}

static char *get_uptime_str(void) {
  time_t current_time = time(NULL);

  char *uptime_buf = format_uptime(current_time, start_date);
  return uptime_buf;
}

int get_cv(h2o_handler_t *self, h2o_req_t *req) {
  char path[1024] = "./assets/cvs/";

  if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
    return -1;

  h2o_iovec_t mime_type;
  mime_type = h2o_strdup(&req->pool, "application/pdf", SIZE_MAX);

  ssize_t header_index =
      h2o_find_header_by_str(&req->headers, H2O_STRLIT("language"), 0);
  fprintf(stderr, "header index: %li\n", header_index);
  if (header_index == -1)
    return -1;

  if (h2o_memis(req->headers.entries[header_index].value.base,
                req->headers.entries[header_index].value.len,
                H2O_STRLIT("Swedish")))
    strlcat(path, "CV_-_Swedish.pdf", 1024);
  else
    strlcat(path, "CV_-_English.pdf", 1024);

  req->res.status = 200;
  req->res.reason = "OK";

  h2o_file_send(req, req->res.status, req->res.reason, path, mime_type, 0);
  return 0;
}

int get_server_info(h2o_handler_t *self, h2o_req_t *req) {
  static h2o_generator_t generator = {NULL, NULL};

  json_t *root = json_object();
  json_t *dependencies = json_object();
  char *uptime_buf = get_uptime_str();

  if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
    return -1;

  json_object_set_new(root, "name", json_string(__NAME__));
  json_object_set_new(root, "description", json_string(__DESCRIPTION__));
  json_object_set_new(root, "version", json_string(__PROJ_VERSION__));
  json_object_set_new(root, "uptime", json_string(uptime_buf));

  json_object_set_new(dependencies, "jansson_version",
                      json_string(JANSSON_VERSION));
  json_object_set_new(dependencies, "libh2o_version",
                      json_string(H2O_LIBRARY_VERSION));
  json_object_set_new(dependencies, "libuv_version",
                      json_string(uv_version_string()));
  json_object_set_new(dependencies, "zlib_version", json_string(ZLIB_VERSION));
  json_object_set_new(dependencies, "openssl_version",
                      json_string(OPENSSL_VERSION));

  json_object_set_new(root, "dependencies", dependencies);

  size_t size = json_dumpb(root, NULL, 0, JSON_INDENT(2));
  if (size == 0) {
    fprintf(stderr, "failed to dump json for uptime");
    return -1;
  }

  char *buf = malloc(size);
  (void)json_dumpb(root, buf, size, JSON_INDENT(2));
  json_decref(root);

  h2o_iovec_t body = h2o_strdup(&req->pool, buf, size);
  free(buf);
  free(uptime_buf);

  req->res.status = 200;
  req->res.reason = "OK";

  h2o_add_header(&req->pool, &req->headers, H2O_TOKEN_CONTENT_TYPE, NULL,
                 H2O_STRLIT("application/json"));
  h2o_start_response(req, &generator);
  h2o_send(req, &body, 1, 1);

  return 0;
}

int get_uptime(h2o_handler_t *self, h2o_req_t *req) {
  static h2o_generator_t generator = {NULL, NULL};

  json_t *root = json_object();

  char *uptime_buf = get_uptime_str();
  json_object_set_new(root, "uptime", json_string(uptime_buf));

  size_t size = json_dumpb(root, NULL, 0, JSON_INDENT(2));
  if (size == 0) {
    fprintf(stderr, "failed to dump json for uptime");
    return -1;
  }

  char *buf = malloc(size);
  (void)json_dumpb(root, buf, size, JSON_INDENT(2));
  json_decref(root);
  free(uptime_buf);

  h2o_iovec_t body = h2o_strdup(&req->pool, buf, size);
  free(buf);

  req->res.status = 200;
  req->res.reason = "OK";

  h2o_add_header(&req->pool, &req->headers, H2O_TOKEN_CONTENT_TYPE, NULL,
                 H2O_STRLIT("application/json"));
  h2o_start_response(req, &generator);
  h2o_send(req, &body, 1, 1);

  return 0;
}
