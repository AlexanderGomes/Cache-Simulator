#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int cash_type, block_size, cash_size, number_of_blocks = 0;
int compulsry_misses = 0, capcity_misses = 0, conflict_misses = 0;

#define DRAM_SIZE (64 * 1024 * 1024)

unsigned int m_w = 0xABABAB55; /* must not be zero, nor 0x464fffff */
unsigned int m_z = 0x05080902; /* must not be zero, nor 0x9068ffff */

unsigned int rand_() {
  m_z = 36969 * (m_z & 65535) + (m_z >> 16);
  m_w = 18000 * (m_w & 65535) + (m_w >> 16);
  return (m_z << 16) + m_w; /* 32-bit result */
}

unsigned int memGen1() {
  static unsigned int addr = 0;
  return (addr++) % (DRAM_SIZE);
}

unsigned int memGen2() {
  static unsigned int addr = 0;
  return rand_() % (128 * 1024);
}

unsigned int memGen3() { return rand_() % (DRAM_SIZE); }

unsigned int memGen4() {
  static unsigned int addr = 0;
  return (addr++) % (1024);
}

unsigned int memGen5() {
  static unsigned int addr = 0;
  return (addr++) % (1024 * 64);
}

unsigned int memGen6() {
  static unsigned int addr = 0;
  return (addr += 256) % (DRAM_SIZE);
}

// Define cache coherence states
enum CacheState { INVALID, SHARED, MODIFIED };

// Cache Simulator with Cache Coherence
bool cacheSim(unsigned int addr, int cash[3][100000], int states[100000],
              int type, int &block_counter, int index_addr, int tag_addr,
              int core_id, int states_other_core[100000]) {
  int shift_offset = log2(block_size);
  bool detected = false;
  bool misses_flag = true;

  if (cash_type == 0) {  // Direct Mapped
    if (cash[0][index_addr] == tag_addr) {
      if (states[index_addr] == INVALID) {
        compulsry_misses++;
        states[index_addr] = SHARED;
        return false;  // Miss due to invalid state
      }
      if (states[index_addr] == SHARED &&
          states_other_core[index_addr] == MODIFIED) {
        states_other_core[index_addr] = SHARED;
      }
      return true;
    } else {
      cash[0][index_addr] = tag_addr;
      states[index_addr] = SHARED;
      for (int i = 0; i < number_of_blocks; i++) {
        if (cash[1][i] != 1) {
          misses_flag = false;
          i = number_of_blocks;
        }
      }
      if (misses_flag)
        capcity_misses++;
      else {
        if (cash[1][index_addr] == 1)
          conflict_misses++;
        else {
          compulsry_misses++;
        }
      }
      cash[1][index_addr] = 1;
      return false;
    }
  } else if (cash_type == 1) {  // Set Associative
    index_addr = index_addr * type;
    for (int i = 0; i < type; i++) {
      if (cash[0][index_addr + i] == tag_addr) {
        if (states[index_addr + i] == INVALID) {
          compulsry_misses++;
          states[index_addr + i] = SHARED;
          return false;  // Miss due to invalid state
        }
        if (states[index_addr + i] == SHARED &&
            states_other_core[index_addr + i] == MODIFIED) {
          states_other_core[index_addr + i] = SHARED;
        }
        return true;
      }
    }
    for (int j = 0; j < type; j++) {
      if (cash[1][index_addr + j] == -1) {
        compulsry_misses++;
        cash[0][index_addr + j] = tag_addr;
        cash[1][index_addr + j] = 1;
        states[index_addr + j] = SHARED;
        return false;
      }
    }
    srand(time(NULL));
    int x = rand() % (type);
    cash[0][index_addr + x] = tag_addr;
    cash[1][index_addr + x] = 1;
    states[index_addr + x] = SHARED;
    capcity_misses++;
    return false;
  } else if (cash_type == 2) {  // Fully Associative
    if (type == 0) {            // LRU
      if (block_counter < number_of_blocks) {
        for (int i = 0; i < number_of_blocks; i++) {
          if (cash[0][i] == addr >> shift_offset) {
            detected = true;
            if (states[i] == INVALID) {
              compulsry_misses++;
              states[i] = SHARED;
              return false;  // Miss due to invalid state
            }
            if (states[i] == SHARED && states_other_core[i] == MODIFIED) {
              states_other_core[i] = SHARED;
            }
            cash[1][i] = block_counter;
            block_counter--;
            return detected;
          }
        }
        if (!detected) {
          compulsry_misses++;
          cash[0][block_counter] = addr >> shift_offset;
          cash[1][block_counter] = block_counter;
          states[block_counter] = SHARED;
          return false;
        }
      } else {
        for (int i = 0; i < number_of_blocks; i++) {
          if (cash[0][i] == (addr >> shift_offset)) {
            detected = true;
            if (states[i] == INVALID) {
              compulsry_misses++;
              states[i] = SHARED;
              return false;  // Miss due to invalid state
            }
            if (states[i] == SHARED && states_other_core[i] == MODIFIED) {
              states_other_core[i] = SHARED;
            }
            cash[1][i] = block_counter;
            return detected;
          }
        }
        if (!detected) {
          int compare = 0;
          for (int i = 1; i < number_of_blocks; i++) {
            if (cash[1][compare] > cash[1][i]) compare = i;
          }
          cash[0][compare] = addr >> shift_offset;
          cash[1][compare] = block_counter;
          states[compare] = SHARED;
          capcity_misses++;
          return false;
        }
      }
    }
    // Additional handling for LFU, FIFO, RANDOM (similar to LRU, omitted for
    // brevity)
  }
  return true;
}

char *msg[2] = {"Miss", "Hit"};

///////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  int looper = 1000000, addr, flag, shift;

  cout << "Please, enter 0 for Direct mapped, 1 for set associative, 2 for "
          "fully associative: "
       << endl;
  cin >> cash_type;
  cout << "Please, enter the size of the block as a Power of 2 between 4 and "
          "128 byte: "
       << endl;
  cin >> block_size;
  cout << "Please, enter cache size: 1KB – 64KB; in steps that are power of 2: "
       << endl;
  cin >> cash_size;

  int cash[2][3][100000];         // 2 cores, 3 arrays for each cache
  int states[2][100000];          // 2 cores, states for each cache line
  int block_counter[2] = {0, 0};  // block counters for 2 cores
  int hit_counter[2] = {0, 0};    // hit counters for 2 cores
  int index_addr = 0, tag_addr = 0;

  for (int core = 0; core < 2; core++) {
    if (cash_type == 0) {  // Direct Mapped
      number_of_blocks = (cash_size * 1024) / block_size;
      for (int i = 0; i < 2; i++)  // Initialize cache with -1
        for (int j = 0; j < number_of_blocks; j++) cash[core][i][j] = -1;
      for (int j = 0; j < number_of_blocks; j++) states[core][j] = INVALID;

      for (int i = 0; i < looper; i++) {
        addr = memGen1();
        shift = log2(block_size);
        index_addr = (addr >> shift) % number_of_blocks;
        shift = log2(number_of_blocks + block_size);
        tag_addr = addr >> shift;  // Calculate index and tag
        flag = cacheSim(addr, cash[core], states[core], 0, block_counter[core],
                        index_addr, tag_addr, core, states[(core + 1) % 2]);
        index_addr = 0;
        tag_addr = 0;
        cout << "0x" << setfill('0') << setw(8) << hex << addr << " ("
             << msg[flag] << ") Core " << core << "\n";
        if (msg[flag] == "Hit") {
          hit_counter[core]++;
        }
      }
      cout << "Core " << core << " Hits: " << hit_counter[core] << endl
           << "Compulsory: " << compulsry_misses << endl
           << "Capacity: " << capcity_misses << endl
           << "Conflict: " << conflict_misses << endl;
    } else if (cash_type == 1) {  // Set Associative
      int number_of_ways;
      cout << "Please, enter the number of ways for the set associative cache: "
              "2, 4, 8, 16"
           << endl;
      cin >> number_of_ways;
      number_of_blocks = (cash_size * 1024) / (block_size * number_of_ways);
      for (int i = 0; i < 3; i++)  // Initialize cache with -1
        for (int j = 0; j < 100000; j++) cash[core][i][j] = -1;
      for (int j = 0; j < number_of_blocks; j++) states[core][j] = INVALID;

      for (int i = 0; i < looper; i++) {
        addr = memGen5();
        shift = log2(block_size);
        index_addr = (addr >> shift) % number_of_blocks;
        shift = log2(number_of_blocks + block_size);
        tag_addr = addr >> shift;  // Calculate index and tag
        flag = cacheSim(addr, cash[core], states[core], number_of_ways,
                        block_counter[core], index_addr, tag_addr, core,
                        states[(core + 1) % 2]);
        index_addr = 0;
        tag_addr = 0;
        cout << "0x" << setfill('0') << setw(8) << hex << addr << " ("
             << msg[flag] << ") Core " << core << "\n";
        if (msg[flag] == "Hit") {
          hit_counter[core]++;
        }
      }
      cout << "Core " << core << " Hits: " << hit_counter[core] << endl
           << "Compulsory: " << compulsry_misses << endl
           << "Capacity: " << capcity_misses << endl
           << "Conflict: " << conflict_misses << endl;
    } else if (cash_type == 2) {  // Fully Associative
      int replacement_type;
      cout << "Please, enter the type of replacement for the Fully "
              "Associative: LRU->0, LFU->1, FIFO->2, RANDOM->3: "
           << endl;
      cin >> replacement_type;
      number_of_blocks = (cash_size * 1024) / block_size;
      for (int i = 0; i < 2; i++)  // Initialize cache with -1
        for (int j = 0; j < number_of_blocks; j++) cash[core][i][j] = -1;
      for (int j = 0; j < number_of_blocks; j++) states[core][j] = INVALID;

      for (int i = 0; i < looper; i++) {
        addr = memGen4();
        flag = cacheSim(addr, cash[core], states[core], replacement_type,
                        block_counter[core], index_addr, tag_addr, core,
                        states[(core + 1) % 2]);
        cout << "0x" << setfill('0') << setw(8) << hex << addr << " ("
             << msg[flag] << ") Core " << core << "\n";
        if (msg[flag] == "Hit") {
          hit_counter[core]++;
        }
        block_counter[core]++;
      }
      cout << "Core " << core << " Hits: " << hit_counter[core] << endl
           << "Compulsory: " << compulsry_misses << endl
           << "Capacity: " << capcity_misses << endl
           << "Conflict: " << conflict_misses << endl;
    }
  }
}
