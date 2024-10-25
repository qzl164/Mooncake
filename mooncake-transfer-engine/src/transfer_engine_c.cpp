// transfer_engine_c.cpp
// Copyright (C) 2024 Feng Ren

#include "transfer_engine_c.h"

#include <cstdint>
#include <memory>

#include "transfer_engine.h"
#include "transport/transport.h"

using namespace mooncake;

transfer_engine_t createTransferEngine(const char *metadata_uri) {
    auto metadata_client = std::make_shared<TransferMetadata>(metadata_uri);
    TransferEngine *native = new TransferEngine(metadata_client);
    return (transfer_engine_t)native;
}

int initTransferEngine(transfer_engine_t engine, const char *local_server_name,
                       const char *connectable_name, uint64_t rpc_port) {
    TransferEngine *native = (TransferEngine *)engine;
    native->init(local_server_name, connectable_name, rpc_port);
    return 0;
}

transport_t installOrGetTransport(transfer_engine_t engine, const char *proto,
                                  void **args) {
    TransferEngine *native = (TransferEngine *)engine;
    return (transport_t)native->installOrGetTransport(proto, args);
}

int uninstallTransport(transfer_engine_t engine, const char *proto) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->uninstallTransport(proto);
}

void destroyTransferEngine(transfer_engine_t engine) {
    TransferEngine *native = (TransferEngine *)engine;
    delete native;
}

segment_id_t openSegment(transfer_engine_t engine, const char *segment_name) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->openSegment(segment_name);
}

int closeSegment(transfer_engine_t engine, segment_id_t segment_id) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->closeSegment(segment_id);
}

int registerLocalMemory(transfer_engine_t engine, void *addr, size_t length,
                        const char *location, int remote_accessible) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->registerLocalMemory(addr, length, location,
                                       remote_accessible, true);
}

int unregisterLocalMemory(transfer_engine_t engine, void *addr) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->unregisterLocalMemory(addr);
}

int registerLocalMemoryBatch(transfer_engine_t engine,
                             buffer_entry_t *buffer_list, size_t buffer_len,
                             const char *location) {
    TransferEngine *native = (TransferEngine *)engine;
    std::vector<BufferEntry> native_buffer_list;
    for (size_t i = 0; i < buffer_len; ++i) {
        BufferEntry entry;
        entry.addr = buffer_list[i].addr;
        entry.length = buffer_list[i].length;
        native_buffer_list.push_back(entry);
    }
    return native->registerLocalMemoryBatch(native_buffer_list, location);
}

int unregisterLocalMemoryBatch(transfer_engine_t engine, void **addr_list,
                               size_t addr_len) {
    TransferEngine *native = (TransferEngine *)engine;
    std::vector<void *> native_addr_list;
    for (size_t i = 0; i < addr_len; ++i)
        native_addr_list.push_back(addr_list[i]);
    return native->unregisterLocalMemoryBatch(native_addr_list);
}

batch_id_t allocateBatchID(transport_t xport, size_t batch_size) {
    Transport *native = (Transport *)xport;
    return (batch_id_t)native->allocateBatchID(batch_size);
}

int submitTransfer(transport_t xport, batch_id_t batch_id,
                   struct transfer_request *entries, size_t count) {
    Transport *native = (Transport *)xport;
    std::vector<Transport::TransferRequest> native_entries;
    native_entries.resize(count);
    for (size_t index = 0; index < count; index++) {
        native_entries[index].opcode =
            (Transport::TransferRequest::OpCode)entries[index].opcode;
        native_entries[index].source = entries[index].source;
        native_entries[index].target_id = entries[index].target_id;
        native_entries[index].target_offset = entries[index].target_offset;
        native_entries[index].length = entries[index].length;
    }
    return native->submitTransfer((Transport::BatchID)batch_id, native_entries);
}

int getTransferStatus(transport_t xport, batch_id_t batch_id, size_t task_id,
                      struct transfer_status *status) {
    Transport *native = (Transport *)xport;
    Transport::TransferStatus native_status;
    int rc = native->getTransferStatus((Transport::BatchID)batch_id, task_id,
                                       native_status);
    if (rc == 0) {
        status->status = (int)native_status.s;
        status->transferred_bytes = native_status.transferred_bytes;
    }
    return rc;
}

int freeBatchID(transport_t xport, batch_id_t batch_id) {
    Transport *native = (Transport *)xport;
    return native->freeBatchID(batch_id);
}

int syncSegmentCache(transfer_engine_t engine) {
    TransferEngine *native = (TransferEngine *)engine;
    return native->syncSegmentCache();
}
