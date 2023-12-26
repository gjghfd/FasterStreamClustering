#ifndef _elasticsketch_H
#define _elasticsketch_H

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
#define ES_d 1
#define BN 8
#define lambd 8
#define rep(i,a,n) for(int i=a;i<=n;i++)
using namespace std;
class Elasticsketch
{
private:
	//ssummary *ss;
	struct heavy { unsigned int FP, pvote, nvote, Flag; } HK[MAX_MEM+10][BN];
	struct light { unsigned int C; } LK[ES_d][MAX_MEM+10];
	BOBHash * bobhash;
	BOBHash * bobhash_[ES_d];
	int K, M1, M2;

public:
	Elasticsketch(int M1, int M2) :M1(M1), M2(M2){ 
		bobhash = new BOBHash(995);
		for (int i = 0; i < ES_d; i++){
			bobhash_[i] = new BOBHash(i+1000);
		}
		clear();
       	}
	void clear()
	{
		for (int i = 0; i < M1 + 5; i++)
			for (int j = 0; j < BN; j++)
				HK[i][j].FP = HK[i][j].pvote = HK[i][j].nvote = HK[i][j].Flag = 0;
		for (int i = 0; i < ES_d; i++)
			for (int j = 0; j < M2 + 5; j++)
				LK[i][j].C = 0;
			
	}

	void Insert(const char * str)
	{
		unsigned int tmpmaxv = MAX_INSERT_PACKAGE;
		unsigned int maxv = 0;
		unsigned int FP = bobhash->run(str, strlen(str));
		unsigned int H1 = FP % M1;
		unsigned int hash[ES_d];
		for (int i = 0; i < ES_d; i++)
			hash[i] = bobhash_[i]->run((const char *)&FP, 4) % M2;
	
		unsigned int min_size = MAX_INSERT_PACKAGE;
		int min_pos = -1;
		int flag = 0;
		for (int i = 0; i < BN; i++) {
			if (HK[H1][i].FP == FP) {
				HK[H1][i].pvote++;
				flag = 1;
				break;
			}
			else if (HK[H1][i].pvote == 0) {
				HK[H1][i].pvote = 1;
				HK[H1][i].FP = FP;
				flag = 1;
				break;
			}
			if (min_size > HK[H1][i].pvote) {
				min_pos = i;
				min_size = HK[H1][i].pvote;
			}
		}
		if (!flag) {
			HK[H1][min_pos].nvote++;
			if (HK[H1][min_pos].nvote / HK[H1][min_pos].pvote >= lambd) {
				for (int i = 0; i < ES_d; i++) {
					unsigned int tmphash = bobhash_[i]->run((const char *)&HK[H1][min_pos].FP, 4) % M2;
					LK[i][tmphash].C += HK[H1][min_pos].pvote;
				}
				HK[H1][min_pos].FP = FP;
				HK[H1][min_pos].Flag = 1;
				HK[H1][min_pos].nvote = 0;
				HK[H1][min_pos].pvote = 1;
				flag = 2;
			}
		}
		if (!flag) {
			for (int i = 0; i < ES_d; i++) {
				LK[i][hash[i]].C++;
			}
		}
		return;		
	}
	double Query(const char * str)
	{
		unsigned int tmpmaxv = MAX_INSERT_PACKAGE;
		unsigned int maxv = 0;
		unsigned int FP = bobhash->run(str, strlen(str));
		unsigned int H1 = FP % M1;
		unsigned int hash[ES_d];
		for (int i = 0; i < ES_d; i++)
			hash[i] = bobhash_[i]->run((const char *)&FP, 4) % M2;
		for (int i = 0; i < BN; i++) {
			if (HK[H1][i].FP == FP) {
				maxv = HK[H1][i].pvote;
				if (HK[H1][i].Flag == 1) {
					for (int i = 0; i < ES_d; i++) {
						tmpmaxv = min(tmpmaxv, LK[i][hash[i]].C);
					}
					maxv += tmpmaxv;
				}
				break;
			}
		}
		if (maxv == 0) {
			for (int i = 0; i < ES_d; i++) {
				tmpmaxv = min(tmpmaxv, LK[i][hash[i]].C);
			}
			maxv = tmpmaxv;
		}
		return maxv;
	}
	~Elasticsketch()
	{
		for (int i=0; i<MAX_MEM+10; i++){
			delete []HK[i];
		}
		for (int i=0; i<ES_d; i++){
			delete []LK[i];
		}
	}
};
#endif
