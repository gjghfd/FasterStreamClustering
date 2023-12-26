#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <string.h>
#include <ctime>
#include <time.h>
#include <iterator>
#include <math.h>
#include <vector>

#include "CMSketch.h"
#include "CUSketch.h"
#include "ASketch.h"
#include "PCUSketch.h"
#include "cuckoo_counter.h"
#include "ElasticSketch.h"
#include "NitroSketch.h"
#include "MVSketch.h"

using namespace std;


char * filename_stream = "../../data/";


char insert[10000000 + 1000000 / 5][105];
char query[10000000 + 1000000 / 5][105];


unordered_map<string, int> unmp;


#define testcycles 1
#define hh 0.00001
#define hc 0.00001
#define epoch 10

int main(int argc, char** argv)
{
    double memory = 0.1;
    if(argc >= 2)
    {
        filename_stream = argv[1];
    }
    if (argc >= 3)
    {
    	memory = stod(argv[2]);
    }
    

    unmp.clear();
    int val;



    int memory_ = memory * 1000;// MB
    //const int memory_ = memory * 1000;//KB
    int word_size = 64;


    int w = memory_ * 1024 * 8.0 / COUNTER_SIZE;	//how many counter;
    int w_p = memory * 1024 * 1024 * 8.0 / (word_size * 2);
    int m1 = memory * 1024 * 1024 * 1.0/4 / 8 / 8;
    int m2 = memory * 1024 * 1024 * 3.0/4 / 2 / 1;
    int m2_mv = memory * 1024 * 1024 / 4 / 4;

    printf("\n******************************************************************************\n");
    printf("Evaluation starts!\n\n");

    
    CMSketch *cmsketch;
    CUSketch *cusketch;
    ASketch *asketch;
    PCUSketch *pcusketch;
    CCCounter *cccounter;
    Nitrosketch *nitrosketch;
    Elasticsketch *elasticsketch;
    MVsketch *mvsketch;


    char _temp[200], temp2[200];
    int t = 0;

    int package_num = 0;


    FILE *file_stream = fopen(filename_stream, "r");

    while(fread(insert[package_num], 1, KEY_LEN, file_stream)==KEY_LEN)
    //while(fgets(insert[package_num], 105, file_stream)!=NULL)
    {
        unmp[string(insert[package_num])]++;
        package_num++;

        if(package_num == MAX_INSERT_PACKAGE)
            break;
    }
    fclose(file_stream);


 
    printf("memory = %dKB\n", memory_);
    printf("dataset name: %s\n", filename_stream);
    printf("total stream size = %d\n", package_num);
    printf("distinct item number = %d\n", unmp.size());
    
    int max_freq = 0;
    unordered_map<string, int>::iterator it = unmp.begin();

    for(int i = 0; i < unmp.size(); i++, it++)
    {
        strcpy(query[i], it->first.c_str());

        int temp2 = it->second;
        max_freq = max_freq > temp2 ? max_freq : temp2;
    }
    printf("max_freq = %d\n", max_freq);
    
    printf("*************************************\n");

printf("heavy change threshold = %d\n\n", (int)(package_num * hc));

/********************************heavy change detection*********************************/

    timespec time1, time2;
    long long resns;
    int window = package_num / epoch;
    int now = 0;

    vector<double> gb_heavy_change_cm(20, 0);
    vector<double> precision_heavy_change_cm(20, 0);
    vector<double> recall_heavy_change_cm(20, 0);
    vector<double> are_heavy_change_cm(20, 0);  
    vector<int>hc_cnt_cm(20, 0);
    unordered_map<string, int>lc_cnt_cm;
    unordered_map<string, int>lc_cnt_prev_cm;
    unordered_map<string, int>hit_cnt_cm;
    unordered_map<string, int>ph_cnt_cm;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        cmsketch = new CMSketch(w / LOW_HASH_NUM, LOW_HASH_NUM);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_cm[string(insert[i])]++;
		if (hit_cnt_cm.count(string(insert[i])) == 0) hit_cnt_cm[string(insert[i])] = cmsketch->Query(insert[i]);
            cmsketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    int th = package_num * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_cm) {
				    int efreq = cmsketch->Query(it.first.c_str());
				    ph_cnt_cm[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_cm) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_cm[it.first]) >= th) f1 = 1; 
				    int efreq = cmsketch->Query(it.first.c_str()) - hit_cnt_cm[it.first];
				    if (ph_cnt_cm.count(it.first) && abs(efreq - ph_cnt_cm[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_cm.count(it.first) && abs(efreq - ph_cnt_cm[it.first]) >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_cm[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }

			    }
			    ph_cnt_cm.clear();
			    for (auto it:lc_cnt_cm) {
				   int efreq = cmsketch->Query(it.first.c_str()) - hit_cnt_cm[it.first];
				   ph_cnt_cm[it.first] = efreq;
			    }
			    gb_heavy_change_cm[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_cm[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_cm[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_cm[i/window-1] /= (tp + fn);
			    hc_cnt_cm[i/window-1]++;
		    }
		    lc_cnt_prev_cm.clear();
		    for (auto it:lc_cnt_cm){
		    	lc_cnt_prev_cm[it.first] = it.second;
		    }
		    lc_cnt_cm = unordered_map<string, int>();
		    hit_cnt_cm = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_cm = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of CM (insert): %.6lf Mips\n", throughput_cm);
    printf("heavy change f1-score of CM: \n");
    for (int i = 1; i < 10; i++)
  	if (hc_cnt_cm[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_cm[i] / hc_cnt_cm[i], recall_heavy_change_cm[i], precision_heavy_change_cm[i], are_heavy_change_cm[i]); 
*/

    vector<double> gb_heavy_change_a(20, 0);
    vector<double> precision_heavy_change_a(20, 0);
    vector<double> recall_heavy_change_a(20, 0);
    vector<double> are_heavy_change_a(20, 0);    
    vector<int>hc_cnt_a(20, 0);
    unordered_map<string, int>lc_cnt_a;
    unordered_map<string, int>lc_cnt_prev_a;
    unordered_map<string, int>hit_cnt_a;
    unordered_map<string, int>ph_cnt_a;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        asketch = new ASketch(w / LOW_HASH_NUM, LOW_HASH_NUM);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_a[string(insert[i])]++;
		if (hit_cnt_a.count(string(insert[i])) == 0) hit_cnt_a[string(insert[i])] = asketch->Query(insert[i]);
            asketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_a) {
				    int efreq = asketch->Query(it.first.c_str());
				    ph_cnt_a[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_a) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_a[it.first]) >= th) f1 = 1;
				    int efreq = asketch->Query(it.first.c_str()) - hit_cnt_a[it.first];
				    if (ph_cnt_a.count(it.first) && abs(efreq - ph_cnt_a[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_a.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_a[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }

			    }
			    ph_cnt_a.clear();
			    for (auto it:lc_cnt_a) {
				    int efreq = asketch->Query(it.first.c_str()) - hit_cnt_a[it.first];
				    ph_cnt_a[it.first] = efreq;
			    }
			    gb_heavy_change_a[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_a[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_a[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_a[i/window-1] /= (tp + fn);
			    hc_cnt_a[i/window-1]++;
		    }
		    lc_cnt_prev_a.clear();
		    for (auto it:lc_cnt_a) {
			    lc_cnt_prev_a[it.first] = it.second;
		    }
		    lc_cnt_a = unordered_map<string, int>();
		    hit_cnt_a = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_a = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of A (insert): %.6lf Mips\n", throughput_a);
    printf("heavy change f1-score of A: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_a[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_a[i] / hc_cnt_a[i], recall_heavy_change_a[i], precision_heavy_change_a[i], are_heavy_change_a[i]); 
*/

  
    vector<double> gb_heavy_change_cu(20, 0);
    vector<double> precision_heavy_change_cu(20, 0);
    vector<double> recall_heavy_change_cu(20, 0);
    vector<double> are_heavy_change_cu(20, 0);
    vector<int>hc_cnt_cu(20, 0);
    unordered_map<string, int>lc_cnt_cu;
    unordered_map<string, int>lc_cnt_prev_cu;
    unordered_map<string, int>hit_cnt_cu;
    unordered_map<string, int>ph_cnt_cu;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        cusketch = new CUSketch(w / LOW_HASH_NUM, LOW_HASH_NUM);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_cu[string(insert[i])]++;
		if (hit_cnt_cu.count(string(insert[i])) == 0) hit_cnt_cu[string(insert[i])] = cusketch->Query(insert[i]);
            cusketch->Insert(insert[i]);
	    if (i && i % window == 0) {		// if i is the first package in a window
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_cu) {
				    int efreq = cusketch->Query(it.first.c_str());
				    ph_cnt_cu[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_cu) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_cu[it.first]) >= th) f1 = 1;
				    int efreq = cusketch->Query(it.first.c_str()) - hit_cnt_cu[it.first];
				    if (ph_cnt_cu.count(it.first) && abs(efreq - ph_cnt_cu[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_cu.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_cu[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }
			    }
			    ph_cnt_cu.clear();
			    for (auto it:lc_cnt_cu) {
				    int efreq = cusketch->Query(it.first.c_str()) - hit_cnt_cu[it.first];
				    ph_cnt_cu[it.first] = efreq;
			    }
			    gb_heavy_change_cu[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_cu[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_cu[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_cu[i/window-1] /= (tp + fn);
			    hc_cnt_cu[i/window-1]++;
		    }
		    lc_cnt_prev_cu.clear();
		    for (auto it:lc_cnt_cu) {
			    lc_cnt_prev_cu[it.first] = it.second;
		    }
		    lc_cnt_cu = unordered_map<string, int>();
		    hit_cnt_cu = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_cu = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of CU (insert): %.6lf Mips\n", throughput_cu);
    printf("heavy change f1-score of CU: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_cu[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_cu[i] / hc_cnt_cu[i], recall_heavy_change_cu[i], precision_heavy_change_cu[i], are_heavy_change_cu[i]); 
*/



    vector<double> gb_heavy_change_pcu(20, 0);
    vector<double> precision_heavy_change_pcu(20, 0);
    vector<double> recall_heavy_change_pcu(20, 0);
    vector<double> are_heavy_change_pcu(20, 0);   
    vector<int>hc_cnt_pcu(20, 0);
    unordered_map<string, int>lc_cnt_pcu;
    unordered_map<string, int>lc_cnt_prev_pcu;
    unordered_map<string, int>hit_cnt_pcu;
    unordered_map<string, int>ph_cnt_pcu;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        pcusketch = new PCUSketch(w_p, LOW_HASH_NUM, word_size);
	//return 0;
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_pcu[string(insert[i])]++;
		if (hit_cnt_pcu.count(string(insert[i])) == 0) hit_cnt_pcu[string(insert[i])] = pcusketch->Query(insert[i]);
            pcusketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_pcu) {
				    int efreq = pcusketch->Query(it.first.c_str());
				    ph_cnt_pcu[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_pcu) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_pcu[it.first]) >= th) f1 = 1;
				    int efreq = pcusketch->Query(it.first.c_str()) - hit_cnt_pcu[it.first];
				    if (ph_cnt_pcu.count(it.first) && abs(efreq - ph_cnt_pcu[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_pcu.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_pcu[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }

			    }
			    ph_cnt_pcu.clear();
			    for (auto it:lc_cnt_pcu) {
				    int efreq = pcusketch->Query(it.first.c_str()) - hit_cnt_pcu[it.first];
				    ph_cnt_pcu[it.first] = efreq;
			    }
			    gb_heavy_change_pcu[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_pcu[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_pcu[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_pcu[i/window-1] /= (tp + fn);
			    hc_cnt_pcu[i/window-1]++;
		    }
		    lc_cnt_prev_pcu.clear();
		    for (auto it:lc_cnt_pcu) {
			    lc_cnt_prev_pcu[it.first] = it.second;
		    }
		    lc_cnt_pcu = unordered_map<string, int>();
		    hit_cnt_pcu = unordered_map<string, int>();
	    }
        }
	//return 0;
    }
    //return 0;
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_pcu = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of PCU (insert): %.6lf Mips\n", throughput_pcu);
    printf("heavy change f1-score of PCU: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_pcu[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_pcu[i] / hc_cnt_pcu[i], recall_heavy_change_pcu[i], precision_heavy_change_pcu[i], are_heavy_change_pcu[i]); 
*/

	
    vector<double> gb_heavy_change_cc(20, 0);
    vector<double> precision_heavy_change_cc(20, 0);
    vector<double> recall_heavy_change_cc(20, 0);
    vector<double> are_heavy_change_cc(20, 0);     
    vector<int>hc_cnt_cc(20, 0);
    unordered_map<string, int>lc_cnt_cc;
    unordered_map<string, int>lc_cnt_prev_cc;
    unordered_map<string, int>hit_cnt_cc;
    unordered_map<string, int>ph_cnt_cc;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        cccounter = new CCCounter(memory * 1024 *1024/8);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_cc[string(insert[i])]++;
		if (hit_cnt_cc.count(string(insert[i])) == 0) hit_cnt_cc[string(insert[i])] = cccounter->Query(insert[i]);
            cccounter->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_cc) {
				    int efreq = cccounter->Query(it.first.c_str());
				    ph_cnt_cc[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_cc) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_cc[it.first]) >= th) f1 = 1;
				    int efreq = cccounter->Query(it.first.c_str()) - hit_cnt_cc[it.first];
				    if (ph_cnt_cc.count(it.first) && abs(efreq - ph_cnt_cc[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_cc.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_cc[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }
			    }
			    ph_cnt_cc.clear();
			    for (auto it:lc_cnt_cc) {
				    int efreq = cccounter->Query(it.first.c_str()) - hit_cnt_cc[it.first];
				    ph_cnt_cc[it.first] = efreq;
			    }
			    gb_heavy_change_cc[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_cc[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_cc[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_cc[i/window-1] /= (tp + fn);
	    		    hc_cnt_cc[i/window-1]++;
		    }
		    lc_cnt_prev_cc.clear();
		    for (auto it:lc_cnt_cc) {
			    lc_cnt_prev_cc[it.first] = it.second;
		    }
		    lc_cnt_cc = unordered_map<string, int>();
		    hit_cnt_cc = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_cc = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of CC (insert): %.6lf Mips\n", throughput_cc);
    printf("heavy change f1-score of CC: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_cc[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_cc[i] / hc_cnt_cc[i], recall_heavy_change_cc[i], precision_heavy_change_cc[i], are_heavy_change_cc[i]); 
*/


    vector<double> gb_heavy_change_es(20, 0);
    vector<double> precision_heavy_change_es(20, 0);
    vector<double> recall_heavy_change_es(20, 0);
    vector<double> are_heavy_change_es(20, 0);     
    vector<int>hc_cnt_es(20, 0);
    unordered_map<string, int>lc_cnt_es;
    unordered_map<string, int>lc_cnt_prev_es;
    unordered_map<string, int>hit_cnt_es;
    unordered_map<string, int>ph_cnt_es;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        elasticsketch = new Elasticsketch(m1, m2);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_es[string(insert[i])]++;
		if (hit_cnt_es.count(string(insert[i])) == 0) hit_cnt_es[string(insert[i])] = elasticsketch->Query(insert[i]);
            elasticsketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_es) {
				    int efreq = elasticsketch->Query(it.first.c_str());
				    ph_cnt_es[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_es) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_es[it.first]) >= th) f1 = 1;
				    int efreq = elasticsketch->Query(it.first.c_str()) - hit_cnt_es[it.first];
				    if (ph_cnt_es.count(it.first) && abs(efreq - ph_cnt_es[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_es.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_es[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }

			    }
			    ph_cnt_es.clear();
			    for (auto it:lc_cnt_es) {
				    int efreq = elasticsketch->Query(it.first.c_str()) - hit_cnt_es[it.first];
				    ph_cnt_es[it.first] = efreq;
			    }
			    gb_heavy_change_es[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_es[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_es[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_es[i/window-1] /= (tp + fn);
 			    hc_cnt_es[i/window-1]++;
		    }
		    lc_cnt_prev_es.clear();
		    for (auto it:lc_cnt_es) {
			    lc_cnt_prev_es[it.first] = it.second;
		    }
		    lc_cnt_es = unordered_map<string, int>();
		    hit_cnt_es = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_es = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of Elastic (insert): %.6lf Mips\n", throughput_es);
    printf("heavy change f1-score of Elastic: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_es[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_es[i] / hc_cnt_es[i], recall_heavy_change_es[i], precision_heavy_change_es[i], are_heavy_change_es[i]); 
*/


    vector<double> gb_heavy_change_ns(20, 0);
    vector<double> precision_heavy_change_ns(20, 0);
    vector<double> recall_heavy_change_ns(20, 0);
    vector<double> are_heavy_change_ns(20, 0);    
    vector<int>hc_cnt_ns(20, 0);
    unordered_map<string, int>lc_cnt_ns;
    unordered_map<string, int>lc_cnt_prev_ns;
    unordered_map<string, int>hit_cnt_ns;
    unordered_map<string, int>ph_cnt_ns;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        nitrosketch = new Nitrosketch(w / LOW_HASH_NUM, LOW_HASH_NUM, 1);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_ns[string(insert[i])]++;
		if (hit_cnt_ns.count(string(insert[i])) == 0) hit_cnt_ns[string(insert[i])] = nitrosketch->Query(insert[i]);
            nitrosketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_ns) {
				    int efreq = nitrosketch->Query(it.first.c_str());
				    ph_cnt_ns[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_ns) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_ns[it.first]) >= th) f1 = 1;
				    int efreq = nitrosketch->Query(it.first.c_str()) - hit_cnt_ns[it.first];
				    if (ph_cnt_ns.count(it.first) && abs(efreq - ph_cnt_ns[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_ns.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_ns[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }
			    }
			    ph_cnt_ns.clear();
			    for (auto it:lc_cnt_ns) {
				    int efreq = nitrosketch->Query(it.first.c_str()) - hit_cnt_ns[it.first];
				    ph_cnt_ns[it.first] = efreq;
			    }
			    gb_heavy_change_ns[i/window-1] += (2 * tp / (2 * tp + fp + fn));
			    recall_heavy_change_ns[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_ns[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_ns[i/window-1] /= (tp + fn);
			    hc_cnt_ns[i/window-1]++;
		    }
		    lc_cnt_prev_ns.clear();
		    for (auto it:lc_cnt_ns) {
			    lc_cnt_prev_ns[it.first] = it.second;
		    }
		    lc_cnt_ns = unordered_map<string, int>();
		    hit_cnt_ns = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_ns = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of Nitro (insert): %.6lf Mips\n", throughput_ns);
    printf("heavy change f1-score of Nitro: \n");
    for (int i = 1; i < 10; i++)
	  if (hc_cnt_ns[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_ns[i] / hc_cnt_ns[i], recall_heavy_change_ns[i], precision_heavy_change_ns[i], are_heavy_change_ns[i]); 
*/


    vector<double> gb_heavy_change_mv(20, 0);
    vector<double> precision_heavy_change_mv(20, 0);
    vector<double> recall_heavy_change_mv(20, 0);
    vector<double> are_heavy_change_mv(20, 0);      
    vector<int>hc_cnt_mv(20, 0);
    unordered_map<string, int>lc_cnt_mv;
    unordered_map<string, int>lc_cnt_prev_mv;
    unordered_map<string, int>hit_cnt_mv;
    unordered_map<string, int>ph_cnt_mv;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int t = 0; t < testcycles; t++)
    {
        mvsketch = new MVsketch(m2_mv);
        for(int i = 0; i <= package_num; i++)
        {
		lc_cnt_mv[string(insert[i])]++;
		if (hit_cnt_mv.count(string(insert[i])) == 0) hit_cnt_mv[string(insert[i])] = mvsketch->Query(insert[i]);
            mvsketch->Insert(insert[i]);
	    if (i && i % window == 0) {
		    double th = window * hc;
		    double tp = 0, fp = 0, tn = 0, fn = 0;
		    if (i / window == 1) {
			    for (auto it:lc_cnt_mv) {
				    int efreq = mvsketch->Query(it.first.c_str());
				    ph_cnt_mv[it.first] = efreq;
			    }
		    }		    
		    else {
			    for (auto it:lc_cnt_mv) {
				    bool f1 = 0, f2 = 0;
				    if (abs(it.second - lc_cnt_prev_mv[it.first]) >= th) f1 = 1;
				    int efreq = mvsketch->Query(it.first.c_str()) - hit_cnt_mv[it.first];
				    if (ph_cnt_mv.count(it.first) && abs(efreq - ph_cnt_mv[it.first]) >= th) f2 = 1;
				    else if (!ph_cnt_mv.count(it.first) && efreq >= th) f2 = 1;
				    if (f1 && f2) tp++;
				    else if (f1 && !f2) fn++;
				    else if (!f1 && f2) fp++;
				    else if (!f1 && !f2) tn++;
				    if (f1){
				    	are_heavy_change_mv[i/window-1] += fabs(efreq-it.second) / (it.second * 1.0);
				    }
	    		    }
			    ph_cnt_mv.clear();
			    for (auto it:lc_cnt_mv) {
				    int efreq = mvsketch->Query(it.first.c_str()) - hit_cnt_mv[it.first];
				    ph_cnt_mv[it.first] = efreq;
			    }
			    gb_heavy_change_mv[i/window-1] += (2 * tp / (2 * tp + fp + fn));
	 		    recall_heavy_change_mv[i/window-1] += (tp / (tp + fn));
			    precision_heavy_change_mv[i/window-1] += (tp / (tp + fp));
			    are_heavy_change_mv[i/window-1] /= (tp + fn);
	   	            hc_cnt_mv[i/window-1]++;
		    }
		    lc_cnt_prev_mv.clear();
		    for (auto it:lc_cnt_mv) {
			    lc_cnt_prev_mv[it.first] = it.second;
		    }
		    lc_cnt_mv = unordered_map<string, int>();
		    hit_cnt_mv = unordered_map<string, int>();
	    }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_mv = (double)1000.0 * testcycles * package_num / resns;
/*    printf("throughput of MV (insert): %.6lf Mips\n", throughput_mv);
    printf("heavy change f1-score of MV: \n");
    for (int i = 1; i < 10; i++)
 	  if (hc_cnt_mv[i] > 0) printf("F1=%.6lf, recall=%.6lf, prec=%.6lf, are=%.6lf\n", gb_heavy_change_mv[i] / hc_cnt_mv[i], recall_heavy_change_mv[i], precision_heavy_change_mv[i], are_heavy_change_mv[i]); 
*/

    printf("********************** Precision *********************\n");
    printf("\t CM\t AS\t CU\t PCU\t CC\t EL\t NI\t MV\n");
    for (int i=1; i<epoch; i++){
	printf("epoch%d\t", i);
	printf("%.4lf\t", precision_heavy_change_cm[i]);
 	printf("%.4lf\t", precision_heavy_change_a[i]);
   	printf("%.4lf\t", precision_heavy_change_cu[i]);
	printf("%.4lf\t", precision_heavy_change_pcu[i]);
	printf("%.4lf\t", precision_heavy_change_cc[i]);
	printf("%.4lf\t", precision_heavy_change_es[i]);
	printf("%.4lf\t", precision_heavy_change_ns[i]);
	printf("%.4lf\n", precision_heavy_change_mv[i]);
    }

    printf("********************** Recall *********************\n");
    printf("\t CM\t AS\t CU\t PCU\t CC\t EL\t NI\t MV\n");
    for (int i=1; i<epoch; i++){
	printf("epoch%d\t", i);
	printf("%.4lf\t", recall_heavy_change_cm[i]);
 	printf("%.4lf\t", recall_heavy_change_a[i]);
   	printf("%.4lf\t", recall_heavy_change_cu[i]);
	printf("%.4lf\t", recall_heavy_change_pcu[i]);
	printf("%.4lf\t", recall_heavy_change_cc[i]);
	printf("%.4lf\t", recall_heavy_change_es[i]);
	printf("%.4lf\t", recall_heavy_change_ns[i]);
	printf("%.4lf\n", recall_heavy_change_mv[i]);
    }

    printf("********************** F1 Score *********************\n");
    printf("\t CM\t AS\t CU\t PCU\t CC\t EL\t NI\t MV\n");
    for (int i=1; i<epoch; i++){
	printf("epoch%d\t", i);
	printf("%.4lf\t", gb_heavy_change_cm[i]);
 	printf("%.4lf\t", gb_heavy_change_a[i]);
   	printf("%.4lf\t", gb_heavy_change_cu[i]);
	printf("%.4lf\t", gb_heavy_change_pcu[i]);
	printf("%.4lf\t", gb_heavy_change_cc[i]);
	printf("%.4lf\t", gb_heavy_change_es[i]);
	printf("%.4lf\t", gb_heavy_change_ns[i]);
	printf("%.4lf\n", gb_heavy_change_mv[i]);
    }

    printf("********************** ARE *********************\n");
    printf("\t CM\t AS\t CU\t PCU\t CC\t EL\t NI\t MV\n");
    for (int i=1; i<epoch; i++){
	printf("epoch%d\t", i);
	printf("%.4lf\t", are_heavy_change_cm[i]);
 	printf("%.4lf\t", are_heavy_change_a[i]);
   	printf("%.4lf\t", are_heavy_change_cu[i]);
	printf("%.4lf\t", are_heavy_change_pcu[i]);
	printf("%.4lf\t", are_heavy_change_cc[i]);
	printf("%.4lf\t", are_heavy_change_es[i]);
	printf("%.4lf\t", are_heavy_change_ns[i]);
	printf("%.4lf\n", are_heavy_change_mv[i]);
    }
    	printf("******************************************************************************\n");
   	printf("Evaluation Ends!\n\n");
    return 0;
}
