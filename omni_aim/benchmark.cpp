#include "benchmark.h"

void Benchmark::add(std::string msg){
	this->timeStamps.push_back({ std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), msg});
	this->offset = msg.size() > offset ? msg.size() : offset;
}
std::string Benchmark::result() {
	std::string out("Benchmark Result - ");
	out += this->title;
	out += "\n";
	if (this->timeStamps.size() < 2) {
		out += "No result - you need at least 2 time stamps.";
		return out;
	}
	unsigned long begin = this->timeStamps[0].first;
	for (int i = 0; i < this->timeStamps.size(); i++) {
		out += this->timeStamps[i].second;
		out += ": ";
		out += std::string(offset - this->timeStamps[i].second.size(), ' '); // pad
		out += std::to_string((float)(this->timeStamps[i].first - begin) / 1000);
		out += " ms\n";

	}
	return out;
}
void Benchmark::clear() {
	this->timeStamps.clear();
	this->offset = 0;
}

Benchmark::Benchmark(std::string t) {
	this->title = t;
}
