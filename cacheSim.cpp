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

	this->sets.resize(this->numSets, vector<Block>(this->assoc));
}

void Cache::split(uint32_t addr, uint32_t &tag, uint32_t &index) {
	index = (addr >> this->offsetBits) & ((1ULL << this->indexBits) - 1);
    tag = addr >> (this->offsetBits + this->indexBits);
}

int Cache::findBlock(int setIdx, uint32_t tag) {
	for (int i = 0; i < this->assoc; i++) {
		if (this->sets[setIdx][i].valid && this->sets[setIdx][i].tag == tag)
			return i;
	}
	return -1;
}

int Cache::chooseVictim(int setIdx) {
	int victim = 0;
	uint32_t minTime = UINT32_MAX;

	for (int i = 0; i < this->assoc; i++) {
		if (!this->sets[setIdx][i].valid)
			return i;

		if (this->sets[setIdx][i].lru < minTime) {
			minTime = sets[setIdx][i].lru;
			victim = i;
		}
	}
	return victim;
}

void Cache::insert(int setIdx, uint32_t tag, bool dirty) {
	int victim = chooseVictim(setIdx);

	this->sets[setIdx][victim].valid = true;
	this->sets[setIdx][victim].tag = tag;
	this->sets[setIdx][victim].dirty = dirty;
	this->sets[setIdx][victim].lru = this->time++;
}

bool Cache::access(uint32_t addr, bool isWrite, bool WriteAlloc, bool &evictedDirty) {
	uint32_t tag, idx;
	split(addr, tag, idx);

	int pos = findBlock(idx, tag);

	if (pos != -1) {
		this->sets[idx][pos].lru = this->time++;
		if (isWrite) this->sets[idx][pos].dirty = true;
		return true;
	}

	int victim = chooseVictim(idx);
	evictedDirty = this->sets[idx][victim].valid && this->sets[idx][victim].dirty;

	insert(idx, tag, isWrite);
	return false;
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

	int countTotalCommands = 0;
	int countHitL1 = 0;
	int countHitL2 = 0;

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

	// l1miss = (l1total - l1hit) / l1total
	// l2miss = (l2total - l2hit) / l2total

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
