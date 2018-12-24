#include "parse.hpp"

#include <sstream>
#include <iostream>
// #include <ifstream>
#include "antlr4-runtime.h"
#include "CoLaLexer.h"
#include "CoLaParser.h"
#include "ast/ast.hpp"
#include "parser/AstBuilder.hpp"

using namespace antlr4;
using namespace cola; // TODO: change namespace name


std::shared_ptr<Program> parse(std::string filename) {
	std::ifstream file(filename);
	return parse(file);
}

std::shared_ptr<Program> parse(std::istream& input) {
	ANTLRInputStream antlr(input);
	CoLaLexer lexer(&antlr);
	CommonTokenStream tokens(&lexer);

	// tokens.fill();
	// for (auto token : tokens.getTokens()) {
	// 	std::cout << token->toString() << std::endl;
	// }

	CoLaParser parser(&tokens);
	auto programContext = parser.program();

	if (parser.getNumberOfSyntaxErrors() != 0) {
		// TODO: proper error class
		throw std::logic_error("Parsing errors ==> aborting.");
	}

	// tree::ParseTree *tree = parser.program();
	// std::cout << tree->toStringTree(&parser) << std::endl << std::endl;

	return AstBuilder::buildFrom(programContext);
}
