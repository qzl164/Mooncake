<div align="center">
<h1>Mooncake</h1>
<h3>A Flexible Framework for Boost LLM Inference Efficiency</h3>
  <strong><a href="#show-cases">🔥 Show Cases</a> | <a href="#quick-start">🚀 Quick Start</a> | <a href="#tutorial">📃 Tutorial</a> | <a href="https://github.com/kvcache-ai/ktransformers/discussions">💬  Discussion </a> </strong>
</div>

<h2 id="intro">🎉 Introduction</h2>

**Mooncake** is an inference acceleration system for large language models (LLMs) with long context. By disaggregating prefill with decoding phases effectively, hardware resources of inference cluster (i.e. CPU, acceleration devices, and memory) are better utilized. 
<br/><br/>
**Mooncake** has been deployed in Kimi (Moonshot Inc.), processing requests 115% and 107% more efficiently than previous systems on NVIDIA A800 and H800 clusters, respectively.
<br/><br/>
To learn more about Mooncake, you can see our paper [arXiv 2407.00079](https://arxiv.org/abs/2407.00079).

<h2 id="show-cases">🔥 Show Cases</h2>
Comparing Mooncake Transfer Engine with Gloo (used by Distributed PyTorch) and TCP, Mooncake Transfer Engine has the lowest I/O latency.
<br/><br/>
With 40 GB of data (equivalent to the size of the KVCache generated by 128k tokens in the LLaMA3-70B model), Mooncake Transfer Engine delivers up to 87 GB/s and 190 GB/s of bandwidth in 4×200 Gbps and 8×400 Gbps RoCE networks respectively, which are about 2.4x and 4.6x faster than the TCP protocol.

![transfer-engine-performance.png](docs/fig/transfer-engine-performance.png)

<h2 id="quick-start">🚀 Quick Start</h2>

### Preparation
In order to install and use Mooncake, some preparation is required.
- Linux-x86_64 with gcc, g++ (9.4+) and cmake (3.16+).
  ```bash
  sudo apt-get update
  sudo apt-get install gcc g++ cmake
  ```
- RDMA Driver & SDK (e.g., Mellanox OFED).
  > If your device's vendor does not provide driver & SDK, you can try to install dependencies by the following command: 
  > ```bash
  > sudo apt-get install libibverbs-devel 
  > ```
- Python (3.10 or above)

In addition, to support more features of Mooncake Transfer Engine, we recommand to install the following components:

- CUDA 12.1 and above, including CUFILE. if you didn't have it yet, you may install from [here](https://developer.nvidia.com/cuda-downloads). 
  ```bash
  # Adding CUDA to PATH
  export PATH=/usr/local/cuda/bin:$PATH
  export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
  export CUDA_PATH=/usr/local/cuda
  ```

- Go 1.20+, you may download from [here](https://go.dev/dl/).
  ```bash
  wget https://go.dev/dl/go1.23.2.linux-amd64.tar.gz
  tar xf go1.23.2.linux-amd64.tar.gz -C /opt/go
  export PATH=/opt/go/bin:$PATH
  ```

- Rust Toolclain
  ```bash
  curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
  ```

### Installation
1. Init source code
   ```bash
   git clone https://github.com/kvcache-ai/mooncake-dev.git
   cd mooncake
   git checkout v0.1
   ```

   > The development branch often involves large changes, so do not use the clients compiled in the "development branch" for the production environment.

2. Install dependencies
   ```bash
   bash dependencies.sh
   ```

3. Compile Mooncake
   ```bash
   mkdir build
   cd build
   cmake ..
   make -j
   ```

#### Advanced Compile Options
Mooncake supports the following advanced compile options:
- `-DUSE_CUDA=[ON|OFF]`: Enable GPU Direct RDMA support. 
- `-DUSE_CXL=[ON|OFF]`: Enable CXL protocols. 
- `-DWITH_P2P_STORE=[ON|OFF]`: Enable Golang support and build P2P Store. 
- `-DWITH_ALLOCATOR=[ON|OFF]`: Build central allocator managed store.
- `-DWITH_WITH_RUST_EXAMPLE=[ON|OFF]`: Enable Rust language support.

### Demo Program
We provide a simple command-line program that you can run for testing Mooncake Transfer Engine.

#### Run Example over RDMA
```bash
# Begin from root of your cloned repo!

# 1. Start the etcd server
etcd --listen-client-urls http://0.0.0.0:2379 --advertise-client-urls http://localhost:2379
# You may need to terminate other etcd processes before running the above command

# 2. Run the server side
cd build/mooncake-transfer-engine/example
./transfer_engine_bench --mode=target \
                        --metadata_server=localhost:2379 \
                        --local_server_name=localhost:12345 \
                        --device_name=erdma_0

# 3. Run the client side
cd build/mooncake-transfer-engine/example
./transfer_engine_bench --mode=initiator \
                        --metadata_server=localhost:2379 \
                        --segment_id=localhost:12345 \
                        --local_server_name=localhost:12346 \
                        --device_name=erdma_1
```
It features the following arguments:
- `--metadata_server` (required): IP and port of etcd server, which holds the metadata of Transfer Engine.
- `--local_server_name` (required): Local IP/Hostname and port. Other nodes in this cluster may contact with this node using this IP/Hostname and port.
- `--segment_id` (required in client side): Name of destination segment for transfer. It should be exactly the same as `--local_server_name` of the server side.
- `--device_name` (required): RDMA device name to use.

#### Run Example over TCP
```bash
# Begin from root of your cloned repo!

# 1. Start the etcd server
etcd --listen-client-urls http://0.0.0.0:2379 --advertise-client-urls http://localhost:2379
# You may need to terminate other etcd processes before running the above command

# 2. Run the server side
cd build/mooncake-transfer-engine/example
./transfer_engine_bench --mode=target \
                        --metadata_server=localhost:2379 \
                        --local_server_name=localhost:12345 \
                        --protocol=tcp

# 3. Run the client side
cd build/mooncake-transfer-engine/example
./transfer_engine_bench --mode=initiator \
                        --metadata_server=localhost:2379 \
                        --segment_id=localhost:12345 \
                        --local_server_name=localhost:12346 \
                        --protocol=tcp
```
It features the following arguments:
- `--metadata_server` (required): IP and port of etcd server, which holds the metadata of Transfer Engine.
- `--local_server_name` (required): Local IP/Hostname and port. Other nodes in this cluster may contact with this node using this IP/Hostname and port.

## Contribute
We welcome contributions from the community! If you have any suggestions or find issues, please submit them via GitHub Issues or submit a Pull Request directly.
