#ifndef CACHE_H
#define CACHE_H

#define READ 'r'
#define WRITE 'w'

#include <vector>
#include <cstdint>
#include <cmath>
#include <climits>
#include <stdint.h>

using namespace std;

struct Block {
    bool valid = false;
    bool dirty = false;
    uint32_t tag = 0;
    uint32_t lru = 0;
};

struct EvictedInfo {
    bool happened;
    bool dirty;
    uint32_t addr;

    EvictedInfo() {
        happened = false;
        dirty = false;
        addr = 0;
    }
};

class Cache {
public:
    int sizeBytes;
    int blockSize;
    int assoc;
    int cycles;

    int numSets;
    int indexBits;
    int offsetBits;

    vector< vector<Block> > sets;

    uint32_t timer;

    Cache();

    Cache(int sizeBLog, int blockBLog, int assocLog, int cyc);

    void split(uint32_t addr, uint32_t &tag, uint32_t &index);

    uint32_t buildAddress(uint32_t tag, uint32_t setIdx);

    int findBlock(int setIdx, uint32_t tag);

    void invalidate(uint32_t addr);

    EvictedInfo insert(uint32_t addr, bool dirty);

    void markDirty(uint32_t addr);

    bool hit(uint32_t addr);
};

#endif // CACHE_H