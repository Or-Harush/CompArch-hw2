#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Cache.h"
#include <cmath>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

// Cache funcitons
Cache::Cache() {};

Cache::Cache(int sizeBLog, int blockBLog, int assocLog, int cyc){
	this->sizeBytes = 1 << sizeBLog;
	this->blockSize = 1 << blockBLog;
	this->assoc = 1 << assocLog;
	this->cycles = cyc;

	this->numSets = int(this->sizeBytes / (this->blockSize * this->assoc));
	this->indexBits = int(log2(numSets));
	this->offsetBits = blockBLog;

	this->timer = 0;
	this->sets.resize(this->numSets, vector<Block>(this->assoc));
}

void Cache::split(uint32_t addr, uint32_t &tag, uint32_t &index) {
	index = (addr >> this->offsetBits) & ((1U << this->indexBits) - 1);
    tag = addr >> (this->offsetBits + this->indexBits);
}

int Cache::findBlock(int setIdx, uint32_t tag) {
	for (int i = 0; i < this->assoc; i++) {
		if (this->sets[setIdx][i].valid && this->sets[setIdx][i].tag == tag)
			return i;
	}
	return -1;
}
uint32_t Cache::buildAddress(uint32_t tag, uint32_t setIdx) { // builds the address from the tag and set index for evicted block
	return (tag << (this->indexBits + this->offsetBits)) | (setIdx << this->offsetBits);
}


void Cache::markDirty(uint32_t addr) {
        uint32_t tag, setIdx;
        split(addr, tag, setIdx);

        int pos = findBlock(setIdx, tag);
        if (pos != -1) {
            sets[setIdx][pos].dirty = true;
            sets[setIdx][pos].lru = timer++;
        }
}

EvictedInfo Cache::insert(uint32_t addr, bool dirty) {
	EvictedInfo info;

	uint32_t tag, setIdx;
	split(addr, tag, setIdx);

	for (int i = 0; i < assoc; i++) {
		if (!sets[setIdx][i].valid) {
			sets[setIdx][i].valid = true;
			sets[setIdx][i].dirty = dirty;
			sets[setIdx][i].tag = tag;
			sets[setIdx][i].lru = timer++;
			return info;
		}
	}

	int victim = 0;
	uint64_t best = sets[setIdx][0].lru;

	for (int i = 1; i < assoc; i++) {
		if (sets[setIdx][i].lru < best) {
			best = sets[setIdx][i].lru;
			victim = i;
		}
	}

	info.happened = true;
	info.dirty = sets[setIdx][victim].dirty;
	info.addr = buildAddress(sets[setIdx][victim].tag, setIdx);

	sets[setIdx][victim].valid = true;
	sets[setIdx][victim].dirty = dirty;
	sets[setIdx][victim].tag = tag;
	sets[setIdx][victim].lru = timer++;

	return info;
}

void Cache::invalidate(uint32_t addr) {
	uint32_t tag, setIdx;
	split(addr, tag, setIdx);

	int pos = findBlock(setIdx, tag);
	if (pos != -1) {
		sets[setIdx][pos].valid = false;
		sets[setIdx][pos].dirty = false;
	}
}

bool Cache::hit(uint32_t addr) {
	uint32_t tag, setIdx;
	split(addr, tag, setIdx);
	int pos = findBlock(setIdx, tag);
	if (pos == -1) return false;

	sets[setIdx][pos].lru = timer++;
	return true;
}
// End Cache functions


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// build caches 
	Cache l1Cache = Cache(L1Size, BSize, L1Assoc, L1Cyc); 
	Cache l2Cache = Cache(L2Size, BSize, L2Assoc, L2Cyc); 

	int countTotalL1 = 0, countTotalL2 = 0;
	int countHitL1 = 0;
	int countHitL2 = 0;
	double totalTime = 0.0;

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		unsigned long int* lineTag = new(unsigned long int);
		unsigned long int* lineIndex = new(unsigned long int);

		countTotalL1++;
		if (l1Cache.hit(num)) // L1 hit!
		{
			totalTime += L1Cyc;
			countHitL1++;
			if (operation == WRITE) {
            	l1Cache.markDirty(num);
        	}
		}
		else if (l2Cache.hit(num)) { // L1 miss, L2 hit!
			countHitL2++;
			countTotalL2++;
			totalTime += L1Cyc + L2Cyc;
			if (operation == WRITE) {
				if (WrAlloc) { // Write allocate policy - write to L1 and mark dirty
					EvictedInfo evictedInfo = l1Cache.insert(num, true);
					if (evictedInfo.happened && evictedInfo.dirty) { // If evicted from L1 and dirty, mark dirty in L2
						l2Cache.markDirty(evictedInfo.addr);
					}
				}
				else { // Write no allocate policy - write to L2 and mark dirty
					l2Cache.markDirty(num);
				}
			}
			else {
				EvictedInfo evictedInfo = l1Cache.insert(num, false); // Insert into L1
				if (evictedInfo.happened && evictedInfo.dirty) { // If evicted from L1 and dirty, mark dirty in L2
					l2Cache.markDirty(evictedInfo.addr);
				}
			}
		}
		else { // Miss in both L1 and L2
			countTotalL2++;
			totalTime += L1Cyc + L2Cyc + MemCyc;

			if (operation == READ) {
				EvictedInfo ev1 = l1Cache.insert(num, false); // Insert into L1
				if (ev1.happened && ev1.dirty) { // If evicted from L1 and dirty, mark dirty in L2
					l2Cache.markDirty(ev1.addr);
				}

				EvictedInfo ev2 = l2Cache.insert(num, false); // Insert into L2 - inclusive policy
				if (ev2.happened) {
					l1Cache.invalidate(ev2.addr); // If evicted from L2, invalidate in L1 (inclusive policy)
				}
			}
			else {
				if (WrAlloc) { // Write allocate policy - write to L1 and mark dirty
					EvictedInfo ev1 = l1Cache.insert(num, true);
					if (ev1.happened && ev1.dirty) { // If evicted from L1 and dirty, mark dirty in L2
						l2Cache.markDirty(ev1.addr);
					}
					EvictedInfo ev2 = l2Cache.insert(num, false); // Insert into L2 - inclusive policy
					if (ev2.happened) {  // 
						l1Cache.invalidate(ev2.addr); // If evicted from L2, invalidate in L1 (inclusive policy)
					}				
				}
			}
		}
		

		// if hit l1
			// if read
				//totall1++, l1_hit++ continue
			// else (write)
				// mark dirty totall1++ l1hit++ continue 
		// else (miss l1) 
			// totall1++ totall2++	
			// if hit l2
				// l2hit++
				// if read 
					// write to l1
				// else (write)
					// write to l1 mark dirty (WB policy)
			// else (miss l2)
				// l2miss++
				// evict from l2 and l1 if needed (if dirty write back to mem)
				// if read					
					// write to l2 and l1 (inclusivness principle)
				// else (write)
					// if write allocate
						// write to l2 and l1 mark dirty (WB policy + inclusivness principle)
					// else (no write allocate)
						// write to l2 only

		// ## HANDLE ACCESS TIMES 
		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	L1MissRate = double(countTotalL1 - countHitL1) / countTotalL1;
	L2MissRate = double(countTotalL2 - countHitL2) / countTotalL2;
	avgAccTime = totalTime / countTotalL1;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
