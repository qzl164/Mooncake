<div align="center">
  <h1>Mooncake: A KVCache-centric Disaggregated<br/> Architecture for LLM Serving</h1>
  <a href="https://arxiv.org/abs/2407.00079" target="_blank"><strong>📃 Technical Report</strong></a>
</div>
<br/>

Mooncake is the serving platform for  <a href="https://kimi.ai/"><img src="image/kimi.png" alt="icon" style="height: 16px; vertical-align: middle;"> Kimi</a>, a leading LLM service provided by <a href="https://www.moonshot.cn/"><img src="image/moonshot.jpg" alt="icon" style="height: 16px; vertical-align: middle;"> Moonshot AI</a>.
Now the core of Mooncake - Transfer Engine is open-sourced!
This repository also hosts its technical report and the open sourced traces. 

<h2 id="updates">🔥 Updates</h2>

 - **Nov X, 2024**: We open sourced the Transfer Engine, the central component of Mooncake. We also provide two demonstrations of Transfer Engine: a P2P Store and vLLM integration.
 - **July 9, 2024**: We open sourced the trace as a <a href="https://github.com/kvcache-ai/Mooncake/blob/main/mooncake_trace.jsonl" target="_blank">jsonl file</a>!.
 - **June 27, 2024**: We present a series of Chinese blogs with more discussions on <a href="https://zhuanlan.zhihu.com/p/705754254">zhihu 1</a>, <a href="https://zhuanlan.zhihu.com/p/705910725">2</a>, <a href="https://zhuanlan.zhihu.com/p/706204757">3</a>, <a href="https://zhuanlan.zhihu.com/p/707997501">4</a>.
 - **June 26, 2024**: Initial technical report release.


<h2 id="overview">🎉 Overview</h2>

Mooncake features a KVCache-centric disaggregated architecture that separates the prefill and decoding clusters. It also leverages the underutilized CPU, DRAM, and SSD resources of the GPU cluster to implement a disaggregated cache of KVCache. 

![architecture](image/architecture.png)

The core of Mooncake is its KVCache-centric scheduler, which balances maximizing overall effective throughput while meeting latency-related Service Level Objectives (SLOs) requirements. Unlike traditional studies that assume all requests will be processed, Mooncake faces challenges due to highly overloaded scenarios. To mitigate these, we developed a prediction-based early rejection policy. Experiments show that Mooncake excels in long-context scenarios. Compared to the baseline method, Mooncake can achieve up to a 525% increase in throughput in certain simulated scenarios while adhering to SLOs. Under real workloads, Mooncake’s innovative architecture enables <a href="https://kimi.ai/">Kimi</a> to handle 75% more requests.

To enable efficient prefill/decode disaggregation, Mooncake proposes the Transfer Engine, which supports rapid, reliable and flexible data transfer over TCP, RDMA, NVIDIA GPUDirect-based RDMA and and NVMe over Fabric (NVMe-of) protocols. Comparing with Gloo (used by Distributed PyTorch) and TCP, Mooncake Transfer Engine has the lowest I/O latency.

<h2 id="show-cases">🔥 Show Cases</h2>

### Use Transfer Engine Standalone ([Guide](doc/en/transfer-engine.md))

Transfer Engine is a high-performance data transfer framework. Transfer Engine provides a unified interface to transfer data from DRAM, VRAM or NVMe, while the technical details related to hardware have been obscured. Transfer Engine supports TCP, RDMA (InfiniBand/RoCEv2/eRDMA/NVIDIA GPUDirect) and NVMe over Fabric (NVMe-of) protocols.

#### Highlights
- **Efficient use of multiple RDMA NIC devices.** Transfer Engine supports the use of multiple RDMA NIC devices to achieve the *aggregation of transfer bandwidth*.

- **Topology aware path selection.** Transfer Engine can *select optimal devices* based on the location (NUMA affinity, etc.) of both source and destination.

- **More robust on temporary network error.** Once transmission fails, Transfer Engine will try to use alternative paths for data delivery automatically.

#### Performance
With 40 GB of data (equivalent to the size of the KVCache generated by 128k tokens in the LLaMA3-70B model), Mooncake Transfer Engine delivers up to **87 GB/s** and **190 GB/s** of bandwidth in 4×200 Gbps and 8×400 Gbps RoCE networks respectively, which are about **2.4x and 4.6x faster** than the TCP protocol.

![transfer-engine-performance.png](image/transfer-engine-performance.png)

### P2P Store  ([Guide](doc/en/p2p-store.md))
P2P Store is built on the Transfer Engine and supports sharing temporary objects between peer nodes in a cluster. P2P Store is ideal for scenarios like checkpoint transfer, where data needs to be rapidly and efficiently shared across a cluster. 
**P2P Store has been used in the checkpoint transfer service of Moonshot AI.**

#### Highlights
- **Decentralized architecture.** P2P Store leverages a pure client-side architecture with global metadata managed by the etcd service.

- **Efficient data distribution.** Designed to enhance the efficiency of large-scale data distribution, P2P Store *avoids bandwidth saturation* issues by allowing replicated nodes to share data directly. This reduces the CPU/RDMA NIC pressures of data providers (e.g., trainers).

#### Performance
Thanks to the high performance of Transfer Engine, P2P Stores can also distribute objects with full utilization of *hardware incoming bandwidth* (e.g., A 25Gbps NIC was used in the following figure, and the throughput of get replica is about 3.1 GB/s).

![p2p-store.gif](image/p2p-store.gif)

### vLLM Integration ([Guide](doc/en/vllm-integration.md))
To optmize LLM inference, the vLLM's community is working at supporting [disaggregated prefilling (PR 8498)](https://github.com/vllm-project/vllm/pull/8498). This feature allows separating the **prefill** phase from the **decode** phase to improve server utilization. It uses `NCCL` as the network layer by default.

We have implemented vLLM integration, which uses Transfer Engine as the network layer instead of `NCCL`. Transfer Engine has simpler interface and more efficient use of RDMA devices.
In the future, we plan to build Mooncake Managed Store on the basis of Transfer Engine, which supports pooled prefill/decode disaggregation. 

#### Performance
（TODO 性能比较，需要阿里测一下并贴个 gif，比如能处理多少个 tokens/s，我记得大概60个）

**More advanced features will coming soon, so stay tuned!**

<h2 id="quick-start">🚀 Quick Start</h2>

### Preparation
In order to install and use Mooncake, some preparation is required.
- RDMA Driver & SDK (e.g., Mellanox OFED).
- Linux-x86_64 with gcc, g++ (9.4+) and cmake (3.16+).
- Python (3.10 or above)

In addition, to support more features of Mooncake Transfer Engine, we *recommand* you to install the following components:

- CUDA 12.1 and above, including NVIDIA GPUDirect Storage Support, if you want to build with `-DUSE_CUDA`. You may install them from [here](https://developer.nvidia.com/cuda-downloads). 
  ```bash
  # Adding CUDA to PATH
  export PATH=/usr/local/cuda/bin:$PATH
  export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
  export CUDA_PATH=/usr/local/cuda
  ```
- Go 1.20+, if you want to build with `-DWITH_P2P_STORE`. You may download it from [here](https://go.dev/dl/).
- Rust Toolclain, if you want to build with `-DWITH_WITH_RUST_EXAMPLE`.

### Installation
1. Init source code
   ```bash
   git clone https://github.com/kvcache-ai/Mooncake.git
   cd Mooncake
   ```

2. Install dependencies
   ```bash
   bash dependencies.sh
   ```

3. Compile Mooncake and examples
   ```bash
   mkdir build
   cd build
   cmake .. # (optional) Specify build options like -D
   make -j
   ```


<h2 id="trace">📦 Open Source Trace</h2>

```json
{
    "timestamp": 27482,
    "input_length": 6955,
    "output_length": 52,
    "hash_ids": [46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 2353, 2354]
}
{
    "timestamp": 30535,
    "input_length": 6472,
    "output_length": 26,
    "hash_ids": [46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 2366]
}
```
The above presents two samples from our trace dataset. The trace includes the timing of request arrivals, the number of input tokens, the number of output tokens, and the remapped block hash. To protect our customers' privacy, we applied several mechanisms to remove user-related information while preserving the dataset's utility for simulated evaluation. More descriptions of the trace (e.g., up to 50% cache hit ratio) can be found in Section 4 of the paper's Version 3.

<h2 id="citation">📑 Citation</h2>
Please kindly cite our paper if you find the paper or the trace is useful:

```bibtex
@article{qin2024mooncake,
  title        = {Mooncake: A KVCache-centric Disaggregated Architecture for LLM Serving},
  author       = {Ruoyu Qin, Zheming Li, Weiran He, Mingxing Zhang, Yongwei Wu, Weimin Zheng, and Xinran Xu},
  year         = {2024},
  url          = {https://arxiv.org/abs/2407.00079}
}
```
