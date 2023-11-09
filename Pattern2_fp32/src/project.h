#include <vector>
#include <adf.h>
#include "kernels.h"
#include "include.h"

using namespace adf;

class simpleGraph : public adf::graph {
private:


  kernel mat_mul_k[mult_Y * mult_X * mult_Z];
  kernel add_k[(mult_Y - 1) * mult_X * mult_Z];

public:

  input_plio A[mult_X * mult_Y];
  input_plio B[mult_Y * mult_Z];

  output_plio C[mult_X * mult_Z];

  simpleGraph(){

	  // input and output PLIOs creation below
	  // A: 0   1    2  ...
	  //    x  x+1  x+2 ...
	  for (int i = 0; i < mult_X * mult_Y; i++){
		  A[i] = input_plio::create(plio_128_bits, "data/matA" + std::to_string(i) + ".txt");
	  }

	  // B: 0   y
	  //	1  y+1
	  //	2  y+2
	  //   ... ...
	  for (int i = 0; i < mult_Y * mult_Z; i++){
		  B[i] = input_plio::create(plio_128_bits, "data/matB" + std::to_string(i) + ".txt");
	  }

	  // C: 0   1    2  ...
	  //    x  x+1  x+2 ...
	  for (int i = 0; i < mult_X * mult_Z; i++){
		  C[i] = output_plio::create(plio_128_bits, "data/matC" + std::to_string(i) + ".txt");
	  }



	  // kernels creation
	  for (int i = 0; i < mult_Y * mult_X * mult_Z; i++){
		  mat_mul_k[i] = kernel::create(mm_kernel0);
	  }

	  for (int i = 0; i < (mult_Y - 1) * mult_X * mult_Z; i++){
		  add_k[i] = kernel::create(vectorized_add);
	  }



	  // automated graph generation
	  for (int i = 0; i < mult_X; i++){
		  for (int j = 0; j < mult_Z; j++){

			  for (int k = 0; k < mult_Y; k++){

				  // inputs (PL) --> mat mul kernels connection
				  connect< window<single_M*single_K*4> >  (A[i*mult_Y + k].out[0], mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + k].in[0]);
				  connect< window<single_K*single_N*4> >  (B[j*mult_Y + k].out[0], mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + k].in[1]);

				  // Place buffers in different banks to prevent memory stalls (see UG1076 for more details)
				  not_equal(location<buffer>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + k].in[0]), location<buffer>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + k].in[1]));

			  }

			  // adder tree automatic generation

			  // this vector holds the nodes to be added in the tree that haven't pair with others yet.
			  // Maximum number of entries is 2 (any 2 should pair together when possible)
			  vector<kernel> nodes_to_add = {};

			  int previous_k = mult_Y;
			  int k = mult_Y/2;
			  int iter = 0;
			  int adder_counter = 0;
			  int prev_layer_counter = 0;

			  while (k > 0){

			         if (iter == 0){ // first level of the tree

			             for (int l = 0; l < k; l++){

			            	 connect< window<single_M*single_N*4> >  (mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2*l].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]);
			            	 connect< window<single_M*single_N*4> >  (mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2*l + 1].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]);

							 // Place buffers in different banks to prevent memory stalls (see UG1076 for more details)
			            	 not_equal(location<buffer>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]), location<buffer>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]));


			                 adder_counter++;
			             }
			             prev_layer_counter = adder_counter;

			             // if one node is not yet part of the adder tree,
			             // add it to vector to pair it with another similar node
			             if (2*k < previous_k){

			            	 nodes_to_add.push_back(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + previous_k - 1]);
			             }

			         }
			         else {  // next level of the tree

			             int current_counter = 0;
			             for (int l = 0; l < k; l++){

			            	 connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - prev_layer_counter + l].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]);
			            	 connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - prev_layer_counter + l + 1].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]);

			                 adder_counter++;
			                 current_counter++;
			             }


			             // if one node is not yet part of the adder tree,
			             // add it to vector to pair it with another similar node
			             if (2*k < previous_k){

			            	 nodes_to_add.push_back(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - current_counter - 1]);
			             }

			             // check wheather 2 nodeds have been added to vector
			             // if there are 2, put an adder and remove from vector
			             if (nodes_to_add.size() == 2){

			            	 connect< window<single_M*single_N*4> >  (nodes_to_add[0].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]);
			            	 connect< window<single_M*single_N*4> >  (nodes_to_add[1].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]);

			                 // remove nodes
			                 nodes_to_add.pop_back();
			                 nodes_to_add.pop_back();

			                 adder_counter++;
			                 current_counter++;
			             }

			             prev_layer_counter = current_counter;

			         }

			         // next iteration
			         iter ++;
			         previous_k = prev_layer_counter;
			         k = prev_layer_counter/2;
			  }

			  // if counter of previous layer is > 1
			  // this means that it should be 2, and the adder came from vector
			  if (prev_layer_counter == 2){

				  connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - prev_layer_counter].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]);
				  connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - prev_layer_counter + 1].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]);

				  adder_counter++;
			  }

			  // there is one remaining node at the vector
			  // put an adder to it
			  else if (nodes_to_add.size() > 0){

				  connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - prev_layer_counter].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[0]);
				  connect< window<single_M*single_N*4> >  (nodes_to_add[0].out[0], add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter].in[1]);

				  nodes_to_add.pop_back();

				  adder_counter++;

			  }

			  // adder --> output (PL) connection
			  connect< window<single_M*single_N*4> >  (add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + adder_counter - 1].out[0], C[i*mult_Z + j].in[0]);

		  }
	  }



	  // direct the source file of kernels
	  for (int i = 0; i < mult_Y * mult_X * mult_Z; i++){
		  source(mat_mul_k[i]) = "kernels/kernels.cc";
	  }

	  for (int i = 0; i < (mult_Y - 1) * mult_X * mult_Z; i++){
		  source(add_k[i]) = "kernels/kernels.cc";
	  }



	  // runtime ratio and place constraints for MatMul kernels
	  for (int i = 0; i < mult_X; i++){
		  for (int j = 0; j < mult_Z; j++){

//			  for (int k = 0; k < mult_Y; k++){


				  runtime<ratio>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 0]) = 1.0;
				  runtime<ratio>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 1]) = 1.0;
				  runtime<ratio>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2]) = 1.0;



				  if (((i*mult_Z + j)%4 == 0)){
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 0]) = tile(((i*mult_Z + j)/4)*2 + 0, 0);
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 1]) = tile(((i*mult_Z + j)/4)*2 + 0, 1);
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2]) = tile(((i*mult_Z + j)/4)*2 + 1, 1);

				  }
				  else if (((i*mult_Z + j)%4 == 1)){
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 0]) = tile(((i*mult_Z + j)/4)*2 + 0, 2 + 0);
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 1]) = tile(((i*mult_Z + j)/4)*2 + 0, 2 + 1);
					  location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2]) = tile(((i*mult_Z + j)/4)*2 + 1, 2 + 1);

				  }
				  else if (((i*mult_Z + j)%4 == 2)){
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 0]) = tile(((i*mult_Z + j)/4)*2 + 0, 4 + 0);
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 1]) = tile(((i*mult_Z + j)/4)*2 + 0, 4 + 1);
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2]) = tile(((i*mult_Z + j)/4)*2 + 1, 4 + 1);


				  }
				  else if (((i*mult_Z + j)%4 == 3)){
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 0]) = tile(((i*mult_Z + j)/4)*2 + 0, 6 + 0);
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 1]) = tile(((i*mult_Z + j)/4)*2 + 0, 6 + 1);
					location<kernel>(mat_mul_k[i*mult_Z*mult_Y + j*mult_Y + 2]) = tile(((i*mult_Z + j)/4)*2 + 1, 6 + 1);


				  }


		  }
	  }


	  // runtime ratio and place constraints for Add kernels
	  for (int i = 0; i < mult_X; i++){
		  for (int j = 0; j < mult_Z; j++){

//			  for (int k = 0; k < (mult_Y-1); k++){

				  runtime<ratio>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 0]) = 0.04;
				  runtime<ratio>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 1]) = 0.04;



				  if (((i*mult_Z + j)%4 == 0)){
					  location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 0]) = tile(((i*mult_Z + j)/4)*2 + 1, 0);
					  location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 1]) = tile(((i*mult_Z + j)/4)*2 + 1, 0);
				  }
				  else if (((i*mult_Z + j)%4 == 1)){
					  location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 0]) = tile(((i*mult_Z + j)/4)*2 + 1, 2 + 0);
					  location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 1]) = tile(((i*mult_Z + j)/4)*2 + 1, 2 + 0);

				  }
				  else if (((i*mult_Z + j)%4 == 2)) {
					location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 0]) = tile(((i*mult_Z + j)/4)*2 + 1, 4 + 0);
					location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 1]) = tile(((i*mult_Z + j)/4)*2 + 1, 4 + 0);

				  }
				  else if (((i*mult_Z + j)%4 == 3)){
					location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 0]) = tile(((i*mult_Z + j)/4)*2 + 1, 6 + 0);
					location<kernel>(add_k[i*mult_Z*(mult_Y-1) + j*(mult_Y-1) + 1]) = tile(((i*mult_Z + j)/4)*2 + 1, 6 + 0);
				  }


		  }
	  }


  }
};
