#ifndef _nitrosketch_H
#define _nitrosketch_H

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <random>
#include "BOBHash.h"
#include "params.h"
//#include "ssummary.h"
//#include "BOBHASH64.h"
#define ns_d 4 
#define rep(i,a,n) for(int i=a;i<=n;i++)
using namespace std;
class Nitrosketch
{
private:
	int *counter[MAX_HASH_NUM];
	BOBHash * bobhash[MAX_HASH_NUM];
	int w, d;
	int next_packet, next_bucket;
	std::default_random_engine generator;
	double prob;
public:
	Nitrosketch(int _w, int _d, double _prob){ 

		w = _w; d = _d; prob=_prob;
		
		next_packet = 1; next_bucket = 0;
		for (int i=0; i<d; i++){
			counter[i] = new int[w];
			memset(counter[i], 0, sizeof(int) * w);	
			bobhash[i] = new BOBHash(i + 1000);	
		}
	}
	void clear()
	{
		for (int i = 0; i < d; i++)
			memset(counter[i], 0, sizeof(int) * w);	
	}
	void Insert(const char * str)
	{
		unsigned int hash[ns_d];
		next_packet--;
		if (next_packet == 0) {
			for (int i = 0; i < d; i++)
				hash[i]=(bobhash[i]->run(str, strlen(str))) % w;
			int i;
			for(;;) {
				i = next_bucket;
				double delta = 1.0/prob*(2*(int)(hash[i]&1)-1);
				counter[i][hash[i]] += (int)delta;

				int sample = 1;
  				if (prob < 1.0) {
 				   std::geometric_distribution<int> dist(prob);
 				   sample = 1 + dist(generator);
 				}

				next_bucket = next_bucket + sample;
				next_packet = next_bucket / ns_d;
				next_bucket %= ns_d;
				if (next_packet > 0)
					break;
			}
		}
	}
	double Query(const char * str)
	{
		int maxv = 0;
		unsigned int hash[ns_d];
		int values[ns_d];
		for (int i = 0; i < d; i++)
			hash[i]=(bobhash[i]->run(str, strlen(str))) % w;

		for(int i = 0; i < ns_d; i++)
		{
			values[i] = counter[i][hash[i]]*(2*(hash[i]&1)-1);
		}
		sort(values, values + ns_d);
		if (ns_d & 1)
			maxv = std::abs(values[ns_d/2]);
		else
			maxv = std::abs((values[ns_d/2-1] + values[ns_d/2]) / 2);
		return maxv;
	}
	~Nitrosketch()
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
#endif
