#include "network.h"
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <string.h>
#include "debugScreen.h"

#define printf psvDebugScreenPrintf

void *net_memory = NULL;
SceUID net_memid = -1;

int load_network_modules() {
    printf("Loading network modules...\n");
    
    int ret = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
    if (ret != SCE_SYSMODULE_LOADED) {
        ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
        if (ret < 0) {
            printf("Error loading SceNet: 0x%08X\n", ret);
            return -1;
        }
    } else {
        printf("SceNet already loaded\n");
    }

    ret = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET_CTL);
    if (ret != SCE_SYSMODULE_LOADED) {
        ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET_CTL);
        if (ret < 0 && ret != 0x805A1000) { // Ignore error if already loaded
            printf("Error loading SceNetCtl: 0x%08X\n", ret);
            sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
            return -1;
        }
    } else {
        printf("SceNetCtl already loaded\n");
    }
    
    return 0;
}

int init_network() {
    FILE *log = fopen("ux0:data/ssh_client_log.txt", "a");
    if (log) {
        fprintf(log, "Reserving memory for network...\n");
        fclose(log);
    }

    net_memid = sceKernelAllocMemBlock("NetMem", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, 512 * 1024, NULL);
    if (net_memid < 0) {
        printf("Error reserving memory block: 0x%08X\n", net_memid);
        return -1;
    }

    sceKernelGetMemBlockBase(net_memid, &net_memory);
    if (!net_memory) {
        printf("Error getting memory base\n");
        sceKernelFreeMemBlock(net_memid);
        return -1;
    }

    SceNetInitParam param;
    memset(&param, 0, sizeof(param));
    param.memory = net_memory;
    param.size = 512 * 1024;
    param.flags = 0;

    printf("Initializing sceNet...\n");
    int ret = sceNetInit(&param);
    if (ret < 0) {
        printf("Error starting sceNet: 0x%08X\n", ret);
        sceKernelFreeMemBlock(net_memid);
        return ret;
    }

    printf("Initializing sceNetCtl...\n");
    ret = sceNetCtlInit();
    if (ret < 0) {
        printf("Error starting sceNetCtl: 0x%08X\n", ret);
        sceKernelFreeMemBlock(net_memid);
        return ret;
    }

    printf("Waiting for network connection...\n");
    int state = 0;
    int timeout = 10;
    while (timeout--) {
        ret = sceNetCtlInetGetState(&state);
        if (ret == 0 && state == SCE_NETCTL_STATE_CONNECTED) {
            printf("Network ready and connected\n");
            return 0;
        }
        printf("Network state: %d, ret: 0x%08X\n", state, ret);
        sceKernelDelayThread(1000 * 1000);
    }

    printf("Could not connect to network: state=%d\n", state);
    sceKernelFreeMemBlock(net_memid);
    return -1;
}

void cleanup_network() {
    if (net_memid >= 0) {
        sceKernelFreeMemBlock(net_memid);
        net_memid = -1;
    }
}
