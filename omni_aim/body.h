#pragma once

#include <vector>
#include <format>




struct Joint {
	float x;
	float y;
	float score;

	Joint( float * data ) {
		this->x = *data;
		this->y = *(data + 1);
		this->score = *(data + 2);
	}
	// The order of the 17 keypoint joints is: [nose, left eye, right eye, left ear, right ear, left shoulder, right shoulder, left elbow, right elbow, left wrist, right wrist, left hip, right hip, left knee, right knee, left ankle, right ankle]
	std::string detail() {
		return std::format("x:{:3f}, y:{:3f}, score:{:3f}", this->x, this->y, this->score);
	}
};

 struct Humanoid {
	 std::vector<Joint> parts;
	 float ymin;
	 float xmin;
	 float ymax;
	 float xmax;
	 float score;

	 Humanoid(float* data) {
		 parts.reserve(17);
		 unsigned int i = 0;
		 for (; i < 17; i++) { // fill 224 bytes into humanoid
		 // Humanoid has 17 joints
		 // each consists of 3 data points
			 this->parts.push_back(Joint(data + i * 3)); // 3 floats offset
		 }
		 this->ymin = *(data + i++ * 3);
		 this->xmin = *(data + i++ * 3);
		 this->ymax = *(data + i++ * 3);
		 this->xmax = *(data + i++ * 3);
		 this->score = *(data + i++ * 3);
	 }

	 std::string detail() {
		 const std::vector<std::string> jointNames{
		 "nose",
		 "left eye",
		 "right eye",
		 "left ear",
		 "right ear",
		 "left shoulder",
		 "right shoulder",
		 "left elbow",
		 "right elbow",
		 "left wrist",
		 "right wrist",
		 "left hip",
		 "right hip",
		 "left knee",
		 "right knee",
		 "left ankle",
		 "right ankle"
		 };

		 std::string out("Humanoid:\n");
		 for (int i = 0; i < 17; i++) {
			 out += jointNames[i] + ": " + this->parts[i].detail() + "\n";
		 }
		 out += "ymin: " + std::to_string(this->ymin) + "\n";
		 out += "xmin: " + std::to_string(this->xmin) + "\n";
		 out += "ymax: " + std::to_string(this->ymax) + "\n";
		 out += "xmax: " + std::to_string(this->xmax) + "\n";
		 out += "score: " + std::to_string(this->score) + "\n";
		 return out;
	 }
 };