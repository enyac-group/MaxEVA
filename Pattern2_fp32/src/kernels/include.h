#ifndef FUNCTION_INCLUDES_H
#define FUNCTION_INCLUDES_H


// define shift right for output values after matrix mult
#define SHIFT 0

// multiple AIE parameters (XxYxZ on manuscript)
#define mult_X 10
#define mult_Y 3
#define mult_Z 10


// single kernel dimensions (MxKxN on manuscript)
#define single_M 32
#define single_K 32
#define single_N 32


// From CHARM:
#define h1 single_M
#define w1 single_K
#define w2 single_N
const int boundary_i=h1/8;
const int boundary_j=w2/2;
const int boundary_k=w1/8-1;
const int judge_i=boundary_i-1;
const int judge_j=boundary_j-1;


#endif
