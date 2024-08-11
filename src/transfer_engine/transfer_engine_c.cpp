// transfer_engine_c.cpp
// Copyright (C) 2024 Feng Ren

#include "transfer_engine/transfer_engine_c.h"
#include "transfer_engine/transfer_engine.h"

using namespace mooncake;

transfer_engine_t createTransferEngine(const char *metadata_uri, 
                                       const char *local_server_name,
                                       const char *nic_priority_matrix)
{
    auto metadata_client = std::make_unique<TransferMetadata>(metadata_uri);
    TransferEngine *native = new TransferEngine(
        metadata_client, local_server_name, nic_priority_matrix);
    return (transfer_engine_t) native;
}

void destroyTransferEngine(transfer_engine_t engine)
{
    TransferEngine *native = (TransferEngine *) engine;
    delete native;
}

int registerLocalMemory(transfer_engine_t engine, void *addr, size_t length, const char *location)
{
    TransferEngine *native = (TransferEngine *) engine;
    return native->registerLocalMemory(addr, length, location);
}

int unregisterLocalMemory(transfer_engine_t engine, void *addr)
{
    TransferEngine *native = (TransferEngine *) engine;
    return native->unregisterLocalMemory(addr);
}

batch_id_t allocateBatchID(transfer_engine_t engine, size_t batch_size)
{
    TransferEngine *native = (TransferEngine *) engine;
    return (batch_id_t) native->allocateBatchID(batch_size);
}

int submitTransfer(transfer_engine_t engine, 
                   batch_id_t batch_id,
                   struct transfer_request *entries,
                   size_t count) 
{
    TransferEngine *native = (TransferEngine *) engine;
    std::vector<TransferRequest> native_entries;
    native_entries.resize(count);
    for (size_t index = 0; index < count; index++)
    {
        native_entries[index].opcode = (TransferRequest::OpCode) entries[index].opcode;
        native_entries[index].source = entries[index].source;
        native_entries[index].target_id = entries[index].target_id;
        native_entries[index].target_offset = entries[index].target_offset;
        native_entries[index].length = entries[index].length;
    }
    return native->submitTransfer((BatchID) batch_id, native_entries);
}

int getTransferStatus(transfer_engine_t engine, 
                      batch_id_t batch_id,
                      size_t task_id,
                      struct transfer_status *status) 
{
    TransferEngine *native = (TransferEngine *) engine;
    TransferStatus native_status;
    int rc = native->getTransferStatus((BatchID) batch_id, task_id, native_status);
    if (rc == 0) {
        status->status = (int) native_status.s;
        status->transferred_bytes = native_status.transferred_bytes;
    }
    return rc;
}

int freeBatchID(transfer_engine_t engine, batch_id_t batch_id)
{
    TransferEngine *native = (TransferEngine *) engine;
    return native->freeBatchID(batch_id);
}

segment_id_t getSegmentID(transfer_engine_t engine, const char *segment_name)
{
    TransferEngine *native = (TransferEngine *) engine;
    return (segment_id_t) native->getSegmentID(segment_name);
}
