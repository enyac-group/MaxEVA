
#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H


  void mm_kernel0(input_window_float* __restrict matA,
  		input_window_float* __restrict matB,
  		output_window_float* __restrict matC);

	void vectorized_add(input_window_float * __restrict in_1, input_window_float * __restrict in_2,
							output_window_float * __restrict out);


#endif
