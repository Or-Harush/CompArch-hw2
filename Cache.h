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

    Cache() {}

    Cache(int sizeB, int blockB, int assoc_, int cyc) {
        sizeBytes = sizeB;
        blockSize = blockB;
        assoc = assoc_;
        cycles = cyc;

        numSets = sizeBytes / (blockSize * assoc);
        indexBits = log2(numSets);
        offsetBits = log2(blockSize);

        sets.resize(numSets, vector<Block>(assoc));
    }

    void split(uint32_t addr, uint32_t &tag, uint32_t &index) {
        index = (addr >> offsetBits) & ((1ULL << indexBits) - 1);
        tag = addr >> (offsetBits + indexBits);
    }

    int findBlock(int setIdx, uint32_t tag) {
        for (int i = 0; i < assoc; i++) {
            if (sets[setIdx][i].valid && sets[setIdx][i].tag == tag)
                return i;
        }
        return -1;
    }

    int chooseVictim(int setIdx) {
        int victim = 0;
        uint32_t minTime = UINT32_MAX;

        for (int i = 0; i < assoc; i++) {
            if (!sets[setIdx][i].valid)
                return i;

            if (sets[setIdx][i].lru < minTime) {
                minTime = sets[setIdx][i].lru;
                victim = i;
            }
        }
        return victim;
    }

    void insert(int setIdx, uint32_t tag, bool dirty) {
        int v = chooseVictim(setIdx);

        sets[setIdx][v].valid = true;
        sets[setIdx][v].tag = tag;
        sets[setIdx][v].dirty = dirty;
        sets[setIdx][v].lru = time++;
    }

    bool access(uint32_t addr, bool isWrite, bool writeBackPolicy, bool &evictedDirty) {
        uint32_t tag, idx;
        split(addr, tag, idx);

        int pos = findBlock(idx, tag);

        if (pos != -1) {
            sets[idx][pos].lru = time++;
            if (isWrite)
                sets[idx][pos].dirty = true;
            return true;
        }

        int victim = chooseVictim(idx);
        evictedDirty = sets[idx][victim].valid && sets[idx][victim].dirty;

        insert(idx, tag, isWrite);
        return false;
    }
};

#endif // CACHE_H