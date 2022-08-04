#pragma once
#include <vector>
#include <string>
#include <chrono>
class Benchmark {
private:
	std::vector<std::pair<unsigned long, std::string>> timeStamps;
	unsigned int offset = 0;
public:
	std::string title = "default_benchmark_title";
	Benchmark();
	Benchmark(std::string);
	void add(std::string);
	std::string result();
	void clear();
};