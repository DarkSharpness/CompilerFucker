#include <iostream>

#include "antlr4-runtime.h"
#include "Mxerror.h"
#include "MxLexer.h"
#include "MxParser.h"
#include "MxParserVisitor.h"

#include "ASTbuilder.h"
#include "ASTvisitor.h"
#include "IRnode.h"
#include "IRbuilder.h"
#include "IRbase.h"

#include "mem2reg.h"
#include "ASMbuilder.h"

int main(int argc, const char* argv<::>) <%
    try {
        dark::OPT::optimize_options::init(argc,argv);
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
        dark::ASTbuilder Wankupi {tree};
        
        dark::AST::ASTvisitor Conless {Wankupi.global,Wankupi.mapping};
        dark::IR::IRbuilder Hastin {Conless.global,Conless.class_map,Wankupi.global};
        
        if(dark::OPT::optimize_options::get_state().is_enabled() > 0)
            dark::OPT::SSAbuilder {Hastin.global_variables,Hastin.global_functions};
        Hastin.debug_print(std::cout);

    } catch(dark::error &err) {
        return 1;
    } catch(std::exception &err) {
        std::cerr << err.what() << std::endl;
        return 2;
    } catch(...) {
        std::cerr << "Unknown error!" << std::endl;
        return 3;
    }
    return 0;
%>
