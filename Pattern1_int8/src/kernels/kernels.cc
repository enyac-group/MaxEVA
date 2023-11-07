#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "include.h"


/*
 *  Matrices should be in blocked format in memory, in the following order:
 * 	 ____________________________
 * 	|  1 |  2 |  3 | ...
 * 	|____|____|____|
 * 	|  x | x+1| x+2| ...
 * 	|____|____|____|
 * 	|.
 * 	|.
 * 	|.
 *
 * 	Tile size is defined from the AI Engine APIs 
 *  check include.h file, dimensions named M_API, K_API, N_API
 *
 */

// optimized matrix multiplication kernel
void opt_blocked_matrix_mult(input_window_int8 * __restrict matA, input_window_int8 * __restrict matB,
						output_window_int32 * __restrict matC) {




	// change M_API, K_API, N_API at include.h, based on AI Engine API
	using MMUL = aie::mmul<M_API, K_API, N_API, int8, int8>;

	// pointers of matrices
	const int8* __restrict pA = (int8*) matA->ptr;
	const int8* __restrict pB = (int8*) matB->ptr;
	int32* __restrict pC = (int32*) matC->ptr;

//	// for profiling
//	unsigned long long cycle_num[2];
//	aie::tile tile = aie::tile::current();
//	cycle_num[0] = tile.cycles();

	// unroll the loops for more optimization
	for (unsigned i = 0; i < (single_M/M_API); i+=2)
//		chess_prepare_for_pipelining
		chess_flatten_loop

	{

		int32 * __restrict pC1 = pC + (i * (single_N/N_API)) * MMUL::size_C;
		int32 * __restrict pC2 = pC + ((i+1) * (single_N/N_API)) * MMUL::size_C;;

		for (unsigned j = 0; j < (single_N/N_API); j+=2)
		chess_flatten_loop
//		chess_prepare_for_pipelining

		{

			const int8 * __restrict pA1 = pA + ( i * (single_K/K_API) + 0) * MMUL::size_A;
			const int8 * __restrict pA2 = pA + ( (i+1) * (single_K/K_API) + 0) * MMUL::size_A;

			const int8 * __restrict pB1 = pB + ( 0 * (single_N/N_API) + j) * MMUL::size_B;
			const int8 * __restrict pB2 = pB + ( 0 * (single_N/N_API) + (j+1)) * MMUL::size_B;


			aie::vector<int8, MMUL::size_A> A0 = aie::load_v<MMUL::size_A>(pA1); pA1 += MMUL::size_A;
			aie::vector<int8, MMUL::size_A> A1 = aie::load_v<MMUL::size_A>(pA2); pA2 += MMUL::size_A;

			aie::vector<int8, MMUL::size_B> B0 = aie::load_v<MMUL::size_B>(pB1); pB1 += MMUL::size_B * (single_N/N_API);
			aie::vector<int8, MMUL::size_B> B1 = aie::load_v<MMUL::size_B>(pB2); pB2 += MMUL::size_B * (single_N/N_API);

			MMUL C00;
			MMUL C01;
			MMUL C10;
			MMUL C11;

			// matrix multiply by initializing to 0
			C00.mul(A0, B0);
			C01.mul(A0, B1);
			C10.mul(A1, B0);
			C11.mul(A1, B1);

			for (unsigned k = 0; k < (single_K/K_API)-1; k++)
//			chess_prepare_for_pipelining
			chess_flatten_loop
			{
				A0 = aie::load_v<MMUL::size_A>(pA1); pA1 += MMUL::size_A;
				A1 = aie::load_v<MMUL::size_A>(pA2); pA2 += MMUL::size_A;

				B0 = aie::load_v<MMUL::size_B>(pB1); pB1 += MMUL::size_B * (single_N/N_API);
				B1 = aie::load_v<MMUL::size_B>(pB2); pB2 += MMUL::size_B * (single_N/N_API);

				// matrix multiply and adding partial blocks
				C00.mac(A0, B0);
				C01.mac(A0, B1);
				C10.mac(A1, B0);
				C11.mac(A1, B1);
			}

			aie::store_v(pC1, C00.template to_vector<int32>(SHIFT)); pC1 +=MMUL::size_C;
			aie::store_v(pC1, C01.template to_vector<int32>(SHIFT)); pC1 +=MMUL::size_C;
			aie::store_v(pC2, C10.template to_vector<int32>(SHIFT)); pC2 +=MMUL::size_C;
			aie::store_v(pC2, C11.template to_vector<int32>(SHIFT)); pC2 +=MMUL::size_C;


		}

	}
//	cycle_num[1] = tile.cycles();
//	printf("start=%llu, end=%llu, Kernel clock cycles=%llu\n", cycle_num[0], cycle_num[1], (cycle_num[1] - cycle_num[0]));


}


void vectorized_add(input_window_int32 * __restrict in_1, input_window_int32 * __restrict in_2,
						output_window_int32 * __restrict out) {

//	// for profiling
//	unsigned long long cycle_num[2];
//	aie::tile tile = aie::tile::current();
//	cycle_num[0] = tile.cycles();

	for (unsigned i=0; i<(single_M*single_N/8); i++)
//	chess_prepare_for_pipelining
	chess_flatten_loop

	{

		// load
		aie::vector<int32, 8> v_a = window_readincr_v<8>(in_1);
		aie::vector<int32, 8> v_b = window_readincr_v<8>(in_2);


		// compute
		aie::vector<int32, 8> v_c = aie::add(v_a, v_b);

		// store
		window_writeincr(out, v_c);
	}

//	cycle_num[1] = tile.cycles();
//	printf("start=%llu, end=%llu, Kernel clock cycles=%llu\n", cycle_num[0], cycle_num[1], (cycle_num[1] - cycle_num[0]));
}
