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

bool type = false;

int main(int argc, const char* argv<::>) <%
    if(argc > 1) {
        std::string_view __v = argv[1];
        --argc , ++argv;
        if (__v == "-s") { type = true; }
        else if (__v != "-ll") {
            std::cerr << "Unknown option: " << __v << std::endl;
            return 1;
        }
    }
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

        if (dark::OPT::optimize_options::get_state().is_enabled() > 0)
            dark::OPT::SSAbuilder {Hastin.global_variables,Hastin.global_functions};
        if (!type)
            Hastin.debug_print(std::cout);

        dark::ASM::ASMbuilder YYU {Hastin.global_variables,Hastin.global_functions};
        if (type)
            YYU.global_info.print(std::cout);

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
