#ifndef CACHE_H
#define CACHE_H

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

    uint32_t time = 0;

    Cache();

    Cache(int sizeBLog, int blockBLog, int assocLog, int cyc);

    void split(uint32_t addr, uint32_t &tag, uint32_t &index);

    int findBlock(int setIdx, uint32_t tag);

    int chooseVictim(int setIdx);

    void insert(int setIdx, uint32_t tag, bool dirty);

    bool access(uint32_t addr, bool isWrite, bool writeBackPolicy, bool &evictedDirty);
};

#endif // CACHE_H