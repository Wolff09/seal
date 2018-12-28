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

using namespace antlr4;
using namespace cola;


std::shared_ptr<Program> cola::parse(std::string filename) {
	std::ifstream file(filename);
	return parse(file);
}

std::shared_ptr<Program> cola::parse(std::istream& input) {
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
