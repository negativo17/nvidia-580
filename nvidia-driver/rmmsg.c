#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ctrl0000system.h
#define NV0000_CTRL_CMD_SYSTEM_DEBUG_RMMSG_CTRL     (0x121U)
#define NV0000_CTRL_SYSTEM_DEBUG_RMMSG_SIZE         512U
#define NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_CMD_GET (0x00000000U)
#define NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_CMD_SET (0x00000001U)
#define NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_PARAMS_MESSAGE_ID (0x21U)
typedef struct NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_PARAMS {
    uint32_t cmd;
    uint32_t count;
    uint8_t  data[NV0000_CTRL_SYSTEM_DEBUG_RMMSG_SIZE];
} NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_PARAMS;

// nvos.h
typedef struct
{
    uint32_t hRoot;
    uint32_t hObjectParent;
    uint32_t hObjectNew;
    uint32_t hClass;
    uint64_t pAllocParms __attribute__ ((aligned (8)));
    uint32_t paramsSize;
    uint32_t status;
} NVOS21_PARAMETERS;

typedef struct
{
    uint32_t hClient;
    uint32_t hObject;
    uint32_t cmd;
    uint32_t flags;
    uint64_t params __attribute__ ((aligned (8)));
    uint32_t paramsSize;
    uint32_t status;
} NVOS54_PARAMETERS;

static int nvctl;
static inline void NvRmAlloc(NVOS21_PARAMETERS *params) {
    int status = ioctl(nvctl, _IOWR('F', 0x2B, NVOS21_PARAMETERS), params);
    if (status < 0) {
        perror("NvRmAlloc failed in OS");
        exit(-1);
    }
    if (params->status != 0) {
        fprintf(stderr, "NvRmAlloc failed in RM: 0x%08x\n", params->status);
        exit(-1);
    }
}

static inline void NvRmControl(NVOS54_PARAMETERS *params) {
    int status = ioctl(nvctl, _IOWR('F', 0x2A, NVOS54_PARAMETERS), params);
    if (status < 0) {
        perror("NvRmControl failed in OS");
        exit(-1);
    }
    if (params->status != 0) {
        fprintf(stderr, "NvRmControl failed in RM: 0x%08x\n", params->status);
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 2 || (argc == 2 && argv[1][0] == '-')) {
        printf(
            "rmmsg - Set NVRM RmMsg debug logs\n"
            "Usage:\n"
            "    rmmsg - display current RmMsg value\n"
            "    rmmsg <val> - set RmMsg value (requires root)\n"
            "\n"
            "more info: https://github.com/NVIDIA/open-gpu-kernel-modules/discussions/197\n");
        return 0;
    }

    nvctl = open("/dev/nvidiactl", O_RDWR);
    if (nvctl < 0) {
        perror("Unable to open /dev/nvidiactl");
        exit(-1);
    }

    NVOS21_PARAMETERS alloc = {0};
    NvRmAlloc(&alloc);
    uint32_t hClient = alloc.hObjectNew;

    NVOS54_PARAMETERS ctrl = {0};
    NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_PARAMS rmmsg = {0};

    ctrl.hClient = hClient;
    ctrl.hObject = hClient;
    ctrl.cmd = NV0000_CTRL_CMD_SYSTEM_DEBUG_RMMSG_CTRL;
    ctrl.params = (uintptr_t)&rmmsg;
    ctrl.paramsSize = sizeof(rmmsg);
    rmmsg.cmd = NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_CMD_GET;

    NvRmControl(&ctrl);

    if (argc < 2) {
        printf("RmMsg is '%s' (%d)\n", rmmsg.data, rmmsg.count);
        return 0;
    }

    uint8_t old[NV0000_CTRL_SYSTEM_DEBUG_RMMSG_SIZE];
    memcpy(old, rmmsg.data, NV0000_CTRL_SYSTEM_DEBUG_RMMSG_SIZE);

    memset(&rmmsg, 0, sizeof(rmmsg));
    strncpy(rmmsg.data, argv[1], sizeof(rmmsg.data) - 1);
    rmmsg.cmd = NV0000_CTRL_SYSTEM_DEBUG_RMMSG_CTRL_CMD_SET;
    NvRmControl(&ctrl);

    printf("Changed RmMsg from '%s' to '%s'\n", old, rmmsg.data);
}
