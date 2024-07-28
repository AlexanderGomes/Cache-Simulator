# Cache Simulator with Cache Coherence

This project implements a cache simulator to model the behavior of different cache architectures and evaluate their performance in a multi-core environment. The simulator supports Direct Mapped, Set Associative, and Fully Associative cache types and employs the MSI (Modified, Shared, Invalid) protocol to maintain cache coherence between two cores.

## Features

- **Cache Architectures:**
  - **Direct Mapped Cache:** Each block of memory maps to exactly one cache line.
  - **Set Associative Cache:** Memory blocks are mapped to a set of cache lines. Supports multiple ways (2, 4, 8, 16).
  - **Fully Associative Cache:** Any block of memory can be stored in any cache line.

- **Cache Replacement Policies:**
  - **Least Recently Used (LRU)**
  - **Least Frequently Used (LFU)**
  - **First In First Out (FIFO)**
  - **Random Replacement**

- **Cache Coherence Protocol:**
  - Implements the MSI (Modified, Shared, Invalid) protocol to maintain coherence between two cores.
  - Ensures consistency of data across multiple cores by invalidating or updating cache lines as needed.

- **Memory Access Patterns:**
  - Sequential Access
  - Random Access
  - Stride Access

- **Performance Metrics:**
  - Tracks and reports cache hits and misses.
  - Differentiates between compulsory, capacity, and conflict misses.
  - Provides detailed output for each memory access, including the address, access type, and result (hit/miss).

## How It Works

The cache simulator models the behavior of a multi-core processor with two cores. Each core has its own private cache, and the simulator maintains coherence between the caches using the MSI protocol.

### Cache Coherence Protocol (MSI)

- **Modified (M):** The cache line is present only in the current core’s cache and has been modified. It is not coherent with main memory or other caches.
- **Shared (S):** The cache line may be present in multiple caches and is consistent with main memory.
- **Invalid (I):** The cache line is not valid in the current cache.

### Cache Operations

- **Read Operation:**
  - If a cache line is in the SHARED or MODIFIED state, it’s a hit.
  - If a cache line is in the INVALID state, it’s a miss, and the line is fetched from memory or another cache.

- **Write Operation:**
  - If a cache line is in the MODIFIED state, it can be directly modified.
  - If a cache line is in the SHARED state, it is invalidated in other caches and then modified.

## Usage

1. **Compile the Code:**
   ```sh
   g++ -std=c++20 cache_simulator.cpp -o cache_simulator
