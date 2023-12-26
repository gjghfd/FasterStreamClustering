#pragma once

#include "common.h"
#include "murmur3.h"



struct CountMap{
	uint32_t seed;
	unordered_map<uint32_t, int> cnt;
	uint64_t maxMem;
	bool sketch;

	CountMap(uint32_t seed_ = -1, bool sketch_ = false){
		if(seed_ > 0) seed = seed_;
		else seed = myRand(1) * (1<<30);
		sketch = sketch_;

		maxMem  = sizeof(seed);
	}

	uint32_t hash(int *key, int len){
		return MurmurHash3_x86_32(key, len, seed);
	}

	void ins(int *key, int len){
		uint32_t hashValue = hash(key, len);
		cnt[hashValue] ++;
	}

	void del(int *key, int len){
		uint32_t hashValue = hash(key, len);
		if((--cnt[hashValue]) == 0) cnt.erase(hashValue);
	}

	int query(int *key, int len){
		uint32_t hashValue = hash(key, len);
		auto loc = cnt.find(hashValue);
		if(loc == cnt.end()) return 0;
		return loc->second;
	}

	int query(uint32_t hashValue){
		// uint32_t hashValue = hash(key, len);
		auto loc = cnt.find(hashValue);
		if(loc == cnt.end()) return 0;
		return loc->second;
	}

	uint64_t getMemoryUsage(){
		return 0;
	}

	vector<uint32_t> getNonempty(){
		vector<uint32_t> ret;
		for(const auto &kv : cnt){
			if(kv.second > 0) ret.push_back(kv.first);
		}
		return ret;
	}
};

struct SampleMap{
	uint32_t seed;
	vector<unordered_map<uint32_t, Point> > cnt;
	uint64_t maxMem;
	CountMap* CM;
	int sample_size;

	SampleMap(CountMap* CM_ = 0, int len_ = 1, uint32_t seed_ = -1){
		if(seed_ > 0) seed = seed_;
		else seed = myRand(1) * (1<<30);
		CM = CM_;
		sample_size = len_;
		cnt.resize(sample_size);

		maxMem  = sizeof(seed);
	}

	void init(CountMap* CM_ = 0, int len_ = 1, uint32_t seed_ = -1){
		if(seed_ > 0) seed = seed_;
		else seed = myRand(1) * (1<<30);
		CM = CM_;
		sample_size = len_;
		cnt.resize(sample_size);

		maxMem  = sizeof(seed);
	}

	void ins(int *key, int len, const Point &point){
		uint32_t hashValue = CM -> hash(key, len);
		int sz = CM -> query(hashValue);
		for(int i = 0; i < sample_size; i++)
			if(myRand(1) <= 1.0 / sz) 
				cnt[i][hashValue] = point;
	}

	void del(int *key, int len){
		// uint32_t hashValue = MurmurHash3_x86_32(key, len, seed);
		// if((--cnt[hashValue]) == 0) cnt.delete(hashValue);
	}

	Point query(int *key, int len, int repl = 0){
		uint32_t hashValue = CM -> hash(key, len);
		if(!sample_size) return Point();
		
		auto loc = cnt[sample_size - 1].find(hashValue);
		Point ret;
		if(loc != cnt[sample_size - 1].end()) ret = loc->second;
		if(!repl) sample_size --;
		return ret;
	}

	Point query(int32_t hashValue, int repl = 0){
		// if(!repl) printf("%d\n", sample_size);
		// uint32_t hashValue = CM -> hash(key, len);
		if(!sample_size){
			// puts("fffff");
			return Point();
		}
		
		auto loc = cnt[sample_size - 1].find(hashValue);
		Point ret;
		if(loc != cnt[sample_size - 1].end()) ret = loc->second;
		// else puts("ffff");
		if(!repl) sample_size --;
		return ret;
	}

	uint64_t getMemoryUsage(){
		return 0;
	}
};