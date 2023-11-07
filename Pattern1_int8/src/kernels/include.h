#ifndef FUNCTION_INCLUDES_H
#define FUNCTION_INCLUDES_H


// define shift right for output values after matrix mult
#define SHIFT 0


// multiple AIE parameters (XxYxZ on manuscript)
#define mult_X 13
#define mult_Y 4
#define mult_Z 6


// single kernel dimensions (MxKxN on manuscript)
#define single_M 32
#define single_K 128
#define single_N 32


// AI Engine API dimensions
#define M_API 4
#define K_API 8
#define N_API 4

#endif
