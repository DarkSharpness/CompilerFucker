#include <iostream>
#include "antlr4-runtime.h"
// #include "ASTvisitor.h"

using namespace antlr4;
//todo: regenerating files in directory named "generated" is dangerous.
//       if you really need to regenerate,please ask TA for help.
int main(int argc, const char* argv[]) {
    
    std::cout << "Hello world!\n";

    // //todo:please don't modify the code below the construction of ifs if you want to use visitor mode
    // ANTLRInputStream input(std::cin);
    // Python3Lexer lexer(&input);
    // CommonTokenStream tokens(&lexer);
    // tokens.fill();
    // Python3Parser parser(&tokens);
    // tree::ParseTree* tree=parser.file_input();
    // EvalVisitor visitor;
    // visitor.visit(tree);
    return 0;
}