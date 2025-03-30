#ifndef API_H_IMPLEMENTATION
#define API_H_IMPLEMENTATION

#include <h2o.h>

int get_cv(h2o_handler_t *self, h2o_req_t *req);
int get_uptime(h2o_handler_t *self, h2o_req_t *req);
int get_server_info(h2o_handler_t *self, h2o_req_t *req);
void init_start_date(void);

#endif // !API_H_IMPLEMENTATION
