#include "predictor.h"
#include <stdexcept>


Predictor::Predictor() {
	// absolute path ->
	/*
	WCHAR curpath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curpath);
	PathCombineW(this->model_path, curpath, TEXT("model"));
	*/
	// relative path ->
	PathCombineW(this->model_path, nullptr, TEXT("model"));
}

void Predictor::load() {

	this->sessionOpts = TF_NewSessionOptions();
	TF_Buffer* runOpts = NULL;

	unsigned int char_path_size = MAX_PATH * sizeof(WCHAR) / sizeof(char);
	char *mb_path = (char*) malloc(char_path_size); // allocate a char array
	wcstombs_s(nullptr, mb_path, char_path_size, this->model_path, char_path_size);


	const char* tags = "serve"; // default model serving tag; can change in future
	int ntags = 1;
	this->graph = TF_NewGraph();
	this->status = TF_NewStatus();
	this->session = TF_LoadSessionFromSavedModel(this->sessionOpts, runOpts, mb_path, &tags, ntags, this->graph, nullptr, status);
	free(mb_path);
	if (TF_GetCode(this->status) != TF_OK)
	{
		if (TF_GetCode(this->status) == TF_NOT_FOUND) {
			std::string msg("Failed to load model. \nPlease ensure 'model' sub-folder exists in the same folder.\n");
			auto wmsg = std::wstring(msg.begin(), msg.end());
			MessageBox(nullptr, wmsg.c_str(), TEXT("Error loading saved model"), MB_OK);
		}
		else {
			std::string msg("message: ");
			msg += TF_Message(this->status);
			auto wmout = std::wstring(msg.begin(), msg.end());
			MessageBox(nullptr, wmout.c_str(), TEXT("Error loading saved model"), MB_OK);

		}
		exit(1);
	}
	else {
		// MessageBox(nullptr, TEXT("Model loaded -> OK "), TEXT("model load"), MB_OK);
	}

	int NumInputs = 1;
	TF_Output* Input = (TF_Output*)malloc(sizeof(TF_Output) * NumInputs);

	this->signature_operation_input = TF_GraphOperationByName(this->graph, this->signature_operation_input_name);
	if (this->signature_operation_input == NULL) {
		std::string msg("oper by name ");
		msg += this->signature_operation_input_name;
		msg += " does not exist";
		auto wmout = std::wstring(msg.begin(), msg.end());
		MessageBox(nullptr, wmout.c_str(), TEXT("graph no operation"), MB_OK);
		exit(1);
	}

	this->signature_operation_output = TF_GraphOperationByName(this->graph, this->signature_operation_output_name);
	if (this->signature_operation_output == NULL) {
		std::string msg("operation by name ");
		msg += this->signature_operation_output_name;
		msg += " does not exist";
		auto wmout = std::wstring(msg.begin(), msg.end());
		MessageBox(nullptr, wmout.c_str(), TEXT("graph no operation"), MB_OK);
		exit(1);
	}

	//Input[0] = t0;
}


TF_Buffer* Predictor::read_file(const WCHAR* file) {
	FILE* f;
	_wfopen_s(&f, file, TEXT("rb"));
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  //same as rewind(f);                                            

	void* data = malloc(fsize);
	fread(data, fsize, 1, f);
	fclose(f);

	TF_Buffer* buf = TF_NewBuffer();
	buf->data = data;
	buf->length = fsize;
	buf->data_deallocator = Predictor::free_buffer;
	return buf;
}

void Predictor::free_buffer(void* data, size_t length) {
	free(data);
}
void Predictor::free_buffer_with_args(void* data, size_t length, void* args) {
	free(data);
}

std::vector<Humanoid> Predictor::predict() {
	std::vector<Humanoid> result;    // result is a list of points
	

	this->benchmark.add("before SessionRun()");
	TF_SessionRun(
		this->session, // session
		nullptr, // run options
		in_TF_Output, InputValues, 1,    // input tensor
		out_TF_Output, OutputValues, 1,    // output tensor
		nullptr, 0,      // target operations
		nullptr,       // run metadata
		this->status   // run status
	);
	this->benchmark.add("after SessionRun()");
	if (TF_GetCode(this->status) != TF_OK)
	{
		std::string msg("status: ");
		msg += TF_Message(this->status);
		auto wmout = std::wstring(msg.begin(), msg.end());
		MessageBox(nullptr, wmout.c_str(), TEXT("session run status error"), MB_OK);
	}

	TF_Tensor* output_tensor = this->OutputValues[0];

	// output is [1,6,56] DT_FLOAT at 1344 bytes total
	float* output_data = (float*)TF_TensorData(output_tensor);
	if (output_data == nullptr) {
		MessageBox(nullptr, TEXT("output_data is null"), TEXT("output_tensor"), MB_OK);
	}
	result.reserve(6);
	for (int i = 0; i < 6; i++) {  // 224 bytes each  ( 4 byte per float * 3 float per Joint * 17 joints + 4 bytes per float * 5 extra variables ) // The third dimension represents the predicted bounding box/keypoint locations and scores. The first 17 * 3 elements are the keypoint locations and scores in the format: [y_0, x_0, s_0, y_1, x_1, s_1, …, y_16, x_16, s_16], where y_i, x_i, s_i are the yx-coordinates (normalized to image frame, e.g. range in [0.0, 1.0]) and confidence scores of the i-th joint correspondingly. The order of the 17 keypoint joints is: [nose, left eye, right eye, left ear, right ear, left shoulder, right shoulder, left elbow, right elbow, left wrist, right wrist, left hip, right hip, left knee, right knee, left ankle, right ankle]. The remaining 5 elements [ymin, xmin, ymax, xmax, score] represent the region of the bounding box (in normalized coordinates) and the confidence score of the instance.   //  https://tfhub.dev/google/movenet/multipose/lightning/1
		result.push_back(Humanoid(output_data + i * 56)); // 224 /4
	}

	// std::string msg = this->benchmark.result();
	// auto wmout = std::wstring(msg.begin(), msg.end());
	// MessageBox(nullptr, wmout.c_str(), TEXT("Predictor Benchmark"), MB_OK);

	TF_DeleteTensor(this->input_tensor);
	this->input_tensor = nullptr;

	this->benchmark.clear();
	return result;
}


void Predictor::setup(int32_t* input, int input_width) {
	// input is [1,512,512,3]   DT_INT32

	this->input_tensor = TF_NewTensor(TF_INT32, this->input_dims, this->input_num_dims, input, sizeof(int32_t) * input_width * input_width * 3, &Predictor::free_buffer_with_args, nullptr);

	if (this->input_tensor == nullptr) {
		throw std::runtime_error("failed to allocate tensor");
	}
	if (this->in_TF_Output == nullptr) {
		this->in_TF_Output = (TF_Output*)malloc(sizeof(TF_Output) * 1);
	}
	in_TF_Output[0] = { this->signature_operation_input, 0 };
	if (this->out_TF_Output == nullptr) {
		this->out_TF_Output = (TF_Output*)malloc(sizeof(TF_Output) * 1);
	}
	out_TF_Output[0] = { this->signature_operation_output, 0 };

	if (this->InputValues == nullptr) {
		this->InputValues = (TF_Tensor**)malloc(sizeof(TF_Tensor*) * 1);
	}
	if (this->OutputValues == nullptr) {
		this->OutputValues = (TF_Tensor**)malloc(sizeof(TF_Tensor*) * 1);
	}
	
	this->InputValues[0] = this->input_tensor;
}


void Predictor::close()
{
	TF_CloseSession(this->session, this->status);
	TF_DeleteSession(this->session, this->status);
	TF_DeleteSessionOptions(this->sessionOpts);
	TF_DeleteGraph(this->graph);
	TF_DeleteStatus(this->status);
}

