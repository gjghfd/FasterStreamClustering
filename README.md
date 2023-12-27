# FasterStreamClustering
This repo aims to implement an optimized version of the streaming clustering algorithm proposed in [1].
We compare the implementation with StreamKM++ [2], one of the state-of-the-art streaming clustering algorithms.

### Usage
We only support Linux and MacOS currently.
```
cd src
make
./main
```
Then you can see results printed in the command line.

### TODO

- Support dynamic gird partition.
- Replace CountMin Sketch with Elastic Skecth.


### References

[1] Zhao Song, Lin F. Yang, Peilin Zhong (2018) Sensitivity Sampling Over Dynamic Geometric Data Streams with Applications to k-Clustering. CoRR abs/1802.00459.

[2] Ackermann MR, Ma¨rtens M, Raupach C, Swierkot K, Lammersen C, Sohler C (2012) StreamKM++: a clustering algorithm for data streams. J Exp Algorithmics 17:2.4:2.1–2.4:2.30.