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
#include "ASMallocator.h"

/**
 * Bit 0: -S / -s
 * Bit 1: -fsyntax-only
 * Bit 2: -ll
*/
std::bitset <3> BitMap = 0;

int main(int argc, const char* argv<::>) <%
    int __len = std::min(argc,2);
    for(int i = 0 ; i < __len ; ++i) {
        const char *__ptr = argv[i];
        if(std::strcmp("-S",__ptr) == 0) {
            BitMap.set(0);
            argc = 0; /* OJ-mode */
            break;
        } else if (std::strcmp("-s",__ptr) == 0) {
            ++argv, --argc;
            BitMap.set(0);
        } else if(std::strcmp("-fsyntax-only",__ptr) == 0) {
            BitMap.set(1);
            argc = 0; /* OJ-mode */
            break;
        } else if(std::strcmp("-ll",__ptr) == 0) {
            ++argv, --argc;
            BitMap.set(2);
            break;
        }
    }
    /* Invalid : OJ-mode and -s */
    if (BitMap.count() != 1) BitMap = 1,argc = 0;

    try {
        dark::OPT::optimize_options::init(argc,argv);
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

        if (BitMap[1]) return 0;

        if (dark::OPT::optimize_options::get_state().is_enabled() > 0)
            dark::OPT::SSAbuilder {Hastin.global_variables,Hastin.global_functions};

        if (BitMap[2]) {
            Hastin.debug_print(std::cout);
            return 0;
        }

        dark::ASM::ASMbuilder YYU {Hastin.global_variables,Hastin.global_functions};

        for(auto __func : YYU.global_info.function_list)
            dark::ASM::ASMallocator {__func};

        if (BitMap[0]) YYU.global_info.print(std::cout);
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
