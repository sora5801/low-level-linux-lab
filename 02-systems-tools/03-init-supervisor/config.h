/* ===========================================================================
 * config.h — parse a tiny INI-style service file into struct sup_service[].
 * ===========================================================================
 */
#ifndef SUP_CONFIG_H
#define SUP_CONFIG_H

#include "service.h"

/* Parse `path` into `svc[0..SUP_MAX_SERVICES]`. On success returns the number
 * of services parsed (>= 0) and *err is left NULL. On failure returns -1 and
 * *err points to a static, human-readable reason (never freed). The parser also
 * resolves every `after` name to an index; an unknown dependency name is an
 * error, because starting in a broken order is worse than refusing to start. */
int sup_config_parse(const char *path, struct sup_service *svc, const char **err);

#endif /* SUP_CONFIG_H */
