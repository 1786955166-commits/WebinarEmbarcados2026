#ifndef NETWORK_APP_H
#define NETWORK_APP_H

#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

int network_task_init(struct net_if *iface);
int network_task_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_APP_H */
