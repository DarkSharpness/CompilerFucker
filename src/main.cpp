#include <iostream>

#include "antlr4-runtime.h"
#include "Mxerror.h"
#include "MxLexer.h"
#include "MxParser.h"
#include "MxParserVisitor.h"

#include "ASTbuilder.h"
#include "ASTvisitor.h"
#include "IRbuilder.h"
#include "IRnode.h"

// using namespace antlr4;
//todo: regenerating files in directory named "generated" is dangerous.
//       if you really need to regenerate,please ask TA for help.
int main(int argc, const char* argv[]) {
    try {
        // freopen("test.in","r",stdin);
        MxErrorListener listener;


        antlr4::ANTLRInputStream input(std::cin);
        MxLexer lexer(&input);
        lexer.removeErrorListeners();
        lexer.addErrorListener(&listener);

        antlr4::CommonTokenStream tokens(&lexer);
        tokens.fill(); // Fill the tokens.
        MxParser parser = MxParser(&tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(&listener);

        auto *tree = parser.file_Input();
        dark::ASTbuilder Wankupi(tree);
        dark::AST::ASTvisitor Conless(Wankupi.global,Wankupi.mapping);
    } catch(dark::error &err) {
        return 1;
    } catch(std::exception &err) {
        std::cerr << err.what() << std::endl;
        return 2;
    } catch(...) {
        std::cerr << "Unknown error!" << std::endl;
        return 3;
    }
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