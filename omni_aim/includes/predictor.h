#pragma once
#include "tensorflow/c/c_api.h"
#pragma comment(lib,"tensorflow.lib")
#include "framework.h"
#include "body.h"
#include "benchmark.h"
#include <string>
#include <vector>
#include <stdio.h>                                                                        
#include <stdlib.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

class Predictor {
private:
	TF_Buffer* read_file(const WCHAR* file);
	WCHAR model_path[MAX_PATH];
	TF_Graph* graph = nullptr;
	TF_Status* status = nullptr;
	TF_Session* session = nullptr;
	TF_SessionOptions* sessionOpts = nullptr;
	TF_Operation* signature_operation_input = nullptr;
	TF_Operation* signature_operation_output = nullptr;

	Benchmark benchmark = Benchmark(std::string("inference time"));

	const char* signature_operation_input_name = "serving_default_input";
	const char* signature_operation_output_name = "StatefulPartitionedCall";

	int64_t input_dims[4] = { 1, 768, 768, 3 };
	int input_num_dims = 4;
	int64_t output_dims[3] = { 1, 6, 16 };
	int output_num_dims = 3;


	TF_Tensor* input_tensor = nullptr;
	TF_Output* in_TF_Output = nullptr;
	TF_Output* out_TF_Output = nullptr;
	TF_Tensor** InputValues = nullptr;
	TF_Tensor** OutputValues = nullptr;

	void write_to_Humanoid(Humanoid& h, float* data);
	void write_to_Joint(Joint& j, float* data);


public:
	Predictor();
	void load();
	static void free_buffer(void* data, size_t length);
	static void free_buffer_with_args(void* data, size_t length, void* args);

	std::vector<Humanoid> predict();    // should return 6 vectors of joints, each having 17 joints
	void setup(int32_t* input, int input_width);

	void close();
};