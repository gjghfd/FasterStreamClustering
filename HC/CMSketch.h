#ifndef _CMSKETCH_H
#define _CMSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include "../common.h"
#include <iostream>

using namespace std;

class CMSketch
{	
private:
	BOBHash * bobhash[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int *counter[MAX_HASH_NUM];
	uint32_t *name[MAX_HASH_NUM];
	vector<Point> *sampler[MAX_HASH_NUM];
	short *pointer[MAX_HASH_NUM];
	int w, d, m;
	int MAX_CNT;
	int counter_index_size;
	uint64_t hash_value;

public:
	CMSketch(int _w, int _d, int m_)
	{

		counter_index_size = 20;
		w = _w;
		d = _d;
		m = m_;
		
		
		for(int i = 0; i < d; i++)	
		{
			counter[i] = new int[w];
			name[i] = new uint32_t[w];
			sampler[i] = new vector<Point>[w];
			pointer[i] = new short[w];
			memset(counter[i], 0, sizeof(int) * w);
			memset(name[i], 0, sizeof(uint32_t) * w);
			memset(pointer[i], 0, sizeof(short) * w);
		}

		MAX_CNT = MAX_INSERT_PACKAGE;//(1 << COUNTER_SIZE) - 1;

		for(int i = 0; i < d; i++)
		{
			bobhash[i] = new BOBHash(i + 1000);
		}
	}

	uint64_t getMemoryUsage(){
		uint64_t sum = MAX_HASH_NUM * sizeof(uint);
		sum += MAX_HASH_NUM * sizeof(int);
		sum += 4 * MAX_HASH_NUM * sizeof(int*);
		sum += 2 * w * d *sizeof(int);
		sum += w * d * sizeof(short);
		for(int i = 0; i < d; i++)
			for(int j = 0; j < w; j++){
				if(sampler[i][j].empty()) continue;
				sum += sampler[i][j].size() * sizeof(double) * (sampler[i][j][0].value.size() + 1);
			}
		return sum;
	}
	void Insert(char * str, int len, const Point &point)
	{
		//printf("the CM bucket is =%d\n", w);
		// int min_value = MAX_CNT, where = 0, temp;
		// for(int i = 0; i < d; i++){
		// 	index[i] = (bobhash[i]->run(str,len)) % w;
		// 	temp = counter[i][index[i]];
		// 	// name[i][index[i]] = *(uint32_t*)str;
		// 	min_value = temp < min_value ? temp : min_value;
		// 	where = temp == min_value ? i : where;
		// }
		// if(min_value == MAX_CNT) return;
		// for(int i = 0; i < d; i++){
		// 	if(counter[i][index[i]] > min_value) continue;
		// 	counter[i][index[i]]++;
		// 	name[i][index[i]] = *(uint32_t*)str;

		// 	if(2 << (2*sampler[i][index[i]].size()) < 1 + counter[i][index[i]]) sampler[i][index[i]].push_back(point);
		// 		else{
		// 			if(myRand(1) < sampler[i][index[i]].size() / (double)counter[i][index[i]]){
		// 				int idx = myRand(1) * sampler[i][index[i]].size();
		// 				sampler[i][index[i]][idx] = point;
		// 			}
		// 		}
			
		// 	// if(sampler[i][index[i]].size() < m) sampler[i][index[i]].push_back(point);
		// 	// else{
		// 	// 	if(myRand(1) < m / (double)counter[i][index[i]]){
		// 	// 		int idx = myRand(1) * m;
		// 	// 		sampler[i][index[i]][idx] = point;
		// 	// 	}
		// 	// }
		// }
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str,len)) % w;
			if(counter[i][index[i]] != MAX_CNT)
			{
				counter[i][index[i]]++;
				name[i][index[i]] = *(uint32_t*)str;
				if(2 << (2*sampler[i][index[i]].size()) < 1 + counter[i][index[i]]) sampler[i][index[i]].push_back(point);
				else{
					if(myRand(1) < sampler[i][index[i]].size() / (double)counter[i][index[i]]){
						int idx = myRand(1) * sampler[i][index[i]].size();
						sampler[i][index[i]][idx] = point;
					}
				}
			}
		}
	}
	double Query( char *str, int len)
	{
		int min_value = MAX_CNT;
		int temp;
		
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, len)) % w;
			temp = counter[i][index[i]];
			min_value = temp < min_value ? temp : min_value;
		}
		return min_value;
	
	}
	Point Sample(char *str, int len)
	{
		int min_value = MAX_CNT, where = 0;
		int temp;
		
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, len)) % w;
			temp = counter[i][index[i]];
			min_value = temp < min_value ? temp : min_value;
			where = temp == min_value ? i : where;
		}
		int idx = (pointer[where][index[where]] = (pointer[where][index[where]] + 1) % sampler[where][index[where]].size());
		return sampler[where][index[where]][idx];
	
	}
	void Delete( char * str, int len)
	{
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, len)) % w;
			counter[i][index[i]] --;
		}
	}
	vector<uint32_t> getNonempty(){
		vector<uint32_t> ret;
		for(int i = 0; i < d; i++)
			for(int j = 0; j < w; j++)
				if(counter[i][j] > 0) ret.push_back(name[i][j]);
		return ret;
	}
	~CMSketch()
	{
		for(int i = 0; i < d; i++)	
		{
			delete []counter[i];
		}


		for(int i = 0; i < d; i++)
		{
			delete bobhash[i];
		}
	}
};
#endif//_CMSKETCH_H
