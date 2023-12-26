#ifndef _mvsketch_H
#define _mvsketch_H

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include "BOBHash.h"
#include "params.h"
//#include "ssummary.h"
//#include "BOBHASH64.h"
#define MV_d 4 
#define rep(i,a,n) for(int i=a;i<=n;i++)
using namespace std;
class MVsketch
{
private:
	//ssummary *ss;
	struct node { int C, FP, S; } HK[MV_d][MAX_MEM+10];
	BOBHash * bobhash_[LOW_HASH_NUM];
	BOBHash * bobhash;
	int K, M2;
public:
	MVsketch(int M2) :M2(M2) { 
		//ss = new ssummary(K); 
		//ss->clear(); 
		bobhash = new BOBHash(995);
		for (int i=0; i<MV_d; i++)
			bobhash_[i] = new BOBHash(i+1000); 
		clear();
	}
	void clear()
	{
		for (int i = 0; i < MV_d; i++)
			for (int j = 0; j <= M2 + 5; j++)
				HK[i][j].C = HK[i][j].FP = HK[i][j].S = 0;
	}
	/*unsigned long long Hash(string ST)
	{
		return (bobhash->run(ST.c_str(), ST.size()));
	}*/


	void Insert(const char * str)
	{
		//bool mon = false;
		//int p = ss->find(x);
		//if (p) mon = true;
		int tmpmaxv = 0;
		int maxv = 0;
		int FP = bobhash->run(str, strlen(str));
		unsigned int hash[MV_d];
		for (int i = 0; i < MV_d; i++)
			hash[i] = bobhash_[i]->run(str, strlen(str)) % M2;
		
		for(int i = 0; i < MV_d; i++)
		{
			HK[i][hash[i]].S++;
			if (HK[i][hash[i]].FP == FP) {
				HK[i][hash[i]].C++;
				//int c = HK[i][hash[i]].C;
				/*if (mon || c <= ss->getmin())
					HK[i][hash[i]].C++;
				maxv=max(maxv,HK[i][hash[i]].C);
				*///tmpmaxv = (HK[i][hash[i]].S + HK[i][hash[i]].C) / 2;
			}
			else if(HK[i][hash[i]].FP == 0)
			{
				HK[i][hash[i]].FP=FP;
				HK[i][hash[i]].C=1;
				//maxv=max(maxv,1);
				//tmpmaxv = (HK[i][hash[i]].S - HK[i][hash[i]].C) / 2;
			}
			else {
				HK[i][hash[i]].C--;
				if (HK[i][hash[i]].C < 0) {
					HK[i][hash[i]].FP = FP;
					HK[i][hash[i]].C = -HK[i][hash[i]].C;
					//maxv=max(maxv,HK[i][hash[i]].C);
					//tmpmaxv = (HK[i][hash[i]].S + HK[i][hash[i]].C) / 2;
				}
			}
			//if (i == 0) maxv = tmpmaxv;
			//else maxv = min(maxv, tmpmaxv);
		}
	}
	double Query(const char * str)
	{
		int FP = bobhash->run(str, strlen(str));
		unsigned int hash[MV_d];
		for (int i = 0; i < MV_d; i++)
			hash[i] = bobhash_[i]->run(str, strlen(str)) % M2;
		int maxv=MAX_INSERT_PACKAGE;
		int tmpmaxv=0;
		for (int i = 0; i < MV_d; i++)
		{
			if (HK[i][hash[i]].FP == FP)
			{
				tmpmaxv = (HK[i][hash[i]].S + HK[i][hash[i]].C) / 2;
				maxv = min(maxv, tmpmaxv);	
			}
			else
			{
				tmpmaxv = (HK[i][hash[i]].S - HK[i][hash[i]].C) / 2;
				maxv = min(maxv, tmpmaxv);	
			}
		}
		return maxv;
	}
	~MVsketch()
	{
		for (int i=0; i<MV_d; i++){
			delete []HK[i];
		}
		for (int i=0; i<MV_d; i++){
			delete []bobhash_[i];
		}
	}
};
#endif
