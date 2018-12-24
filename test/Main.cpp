#include "parser/parse.hpp"

#include <iostream>


int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "No input file given." << std::endl;
		return -1;
	}

	auto filename = argv[1];
	auto prog = parse(filename);

	return 0;
}
