#include "cola/parse.hpp"
#include "cola/transform.hpp"
#include "cola/util.hpp"

#include <iostream>


int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "No input file given." << std::endl;
		return -1;
	}

	auto filename = argv[1];
	auto prog = cola::parse(filename);
	
	std::cout << "Parsed program: " << std::endl;
	cola::print(*prog, std::cout);

	cola::simplify(*prog);

	std::cout << std::endl << std::endl << std::endl << std::endl;
	std::cout << "Simplified program: " << std::endl;
	cola::print(*prog, std::cout);

	return 0;
}
