#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transfer_engine/transfer_engine_c.h"

#define WRITE 1
#define READ 0

void **init_args()
{
    void **args = (void **)malloc(2 * sizeof(void *));
    args[0] = malloc(16);
    args[1] = malloc(16);
    memset(args[0], 'a', 16);
    memset(args[1], 'b', 16);
    ((char *)args[0])[15] = '\0';
    ((char *)args[1])[15] = '\0';
    return args;
}

int main(void)
{
    char *metadata = "meta_server_addr";
    char *local = "local_server";
    transfer_engine_t *engine = createTransferEngine(metadata, local);
    void **args = init_args();
    installTransport(engine, "dummy", "dummy", args);
    
    registerSegment(engine, (void*)0x1000, 0x1000, "aaa");
    transport_t xport;
    segment_id_t seg = openSegment(engine, "dummy", &xport);
    const size_t batch_size = 16;
    batch_id_t batch = allocateBatchID(xport, batch_size);
    char *buf = (char *)malloc(1024 * batch_size);
    memset(buf, 1, 1024 * batch_size);

    struct transfer_request transfers[batch_size + 1];
    for (int i = 0; i < batch_size; i++)
    {
        transfers[i] = (struct transfer_request){
            .opcode = WRITE,
            .source = buf + i * 1024,
            .target_id = seg,
            .target_offset = 0,
            .length = 1024,
        };
    }
    submitTransfer(xport, batch, transfers, batch_size);

    for (int i = 0; i < batch_size; i++)
    {
        struct transfer_status status;
        int ret;
        // getTransferStatus(xport, batch, i, &status);
        while ((ret = getTransferStatus(xport, batch, i, &status)) == 0)
        {
            // busy waiting
        };
        printf("transfer %d: %zu bytes transferred, status = %d\n", i, status.transferred_bytes, status.status);
    }

    unregisterSegment(engine, (void*)0x1000);
    closeSegment(xport, seg);
    freeBatchID(xport, batch);
    uninstallTransport(engine, xport);
    destroyTransferEngine(engine);
    return 0;
}
