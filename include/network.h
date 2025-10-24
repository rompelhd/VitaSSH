#ifndef NETWORK_H
#define NETWORK_H

#include <psp2/kernel/sysmem.h>

#ifndef SCE_SYSMODULE_NET_CTL
#define SCE_SYSMODULE_NET_CTL 0x9
#endif

int init_network();
void cleanup_network();
int load_network_modules();

extern void *net_memory;
extern SceUID net_memid;

#endif
