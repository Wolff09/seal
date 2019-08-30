#include "cola/parse.hpp"

#include <sstream>
#include <iostream>
#include <fstream>
// #include <ifstream>
#include "antlr4-runtime.h"
#include "CoLaLexer.h"
#include "CoLaParser.h"
#include "cola/ast.hpp"
#include "cola/parser/AstBuilder.hpp"
#include "cola/observer.hpp"
#include "cola/parser/ObserverBuilder.hpp"

using namespace antlr4;
using namespace cola;


std::string get_path(std::string filename) {
	// #include <filesystem>
	// return std::filesystem::path(filename).parent_path();

	auto find = filename.rfind('/');
	if (find != std::string::npos) {
		return filename.substr(0, find+1);
	} else {
		return "";
	}
}


std::shared_ptr<Program> cola::parse_program(std::string filename) {
	std::ifstream file(filename);
	auto result = parse_program(file);
	result->options["_path"] = get_path(filename);
	// std::cout << get_path(filename) << std::endl;
	return result;
}

std::shared_ptr<Program> cola::parse_program(std::istream& input) {
	ANTLRInputStream antlr(input);
	CoLaLexer lexer(&antlr);
	CommonTokenStream tokens(&lexer);

	CoLaParser parser(&tokens);
	auto programContext = parser.program();

	if (parser.getNumberOfSyntaxErrors() != 0) {
		// TODO: proper error class
		throw std::logic_error("Parsing errors ==> aborting.");
	}

	return AstBuilder::buildFrom(programContext);
}


std::vector<std::unique_ptr<Observer>> cola::parse_observer(std::string filename, const Program& program) {
	std::ifstream file(filename);
	return parse_observer(file, program);
}

std::vector<std::unique_ptr<Observer>> cola::parse_observer(std::istream& input, const Program& program) {
	ANTLRInputStream antlr(input);
	CoLaLexer lexer(&antlr);
	CommonTokenStream tokens(&lexer);

	CoLaParser parser(&tokens);
	auto observerContext = parser.observer();

	if (parser.getNumberOfSyntaxErrors() != 0) {
		// TODO: proper error class
		throw std::logic_error("Parsing errors ==> aborting.");
	}

	return ObserverBuilder::buildFrom(observerContext, program);
}
