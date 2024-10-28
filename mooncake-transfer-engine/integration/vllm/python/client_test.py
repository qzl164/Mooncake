#!/usr/bin/env python3
import vllm_adaptor
from vllm_adaptor import MooncakeTransfer, MessageQueue
from multiprocessing import Process, Queue

if __name__ == "__main__":
    length = 1024
    mc = MooncakeTransfer()
    mq = MessageQueue()
    mc.initialize("192.168.0.138:10002", "192.168.0.139:2379", "rdma", "erdma_0")
    ptr = mc.allocate_managed_buffer(length)
    ptr_dst = mq.recv_ptr()
    mc.transfer_sync("192.168.0.138:10001", ptr, ptr_dst, length)
    mc.free_managed_buffer(ptr, length)