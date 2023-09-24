#pragma once

#include "ASMnode.h"
#include "ASManalyser.h"

#include <list>
#include <array>

namespace dark::ASM {


/* Linear scan allocator. */
struct ASMallocator final : ASMvisitor {

    const ASMlifeanalyser *__impl;
    ASMallocator(function *);

    struct address_info {
        enum : unsigned {
            STACK       = 0,    /* Spilled into stack.      */
            CALLER      = 1,    /* Caller save register     */
            CALLEE      = 2,    /* Callee save register     */
            TEMPORARY   = 3     /* Temporary register saved */
        } type;
        unsigned index; /* Index in stack/register/temporary */
    };

    using pair_pointer = decltype(__impl->usage_map.begin());

    /* Index of block position. */
    std::vector <int> block_pos;

    /* Worklist sorted by ascending beginning order */
    std::vector <pair_pointer> work_list;

    /* Map a virtual register to its index. */
    std::unordered_map <virtual_register *,address_info> vir_map;

    inline static constexpr size_t CALLEE_SIZE = 14;
    inline static constexpr size_t CALLER_SIZE = 14;

    /**
     * Remapping :
     * callee 0 ~ 13    Saved
     * caller 14 ~ 27   Temporaries
    */
    struct register_slot {
        static inline constexpr int NPOS = INT32_MAX - 2;
        /* A map to the death time of a virtual register within. */
        std::vector <pair_pointer> life_map;
        /* The sum of all live ranges. */
        std::list <std::pair <int,int>> live_range;
        /* Return the next use index. */
        int next_use() const noexcept;
        double spill_cost(bool) const noexcept;
        void merge_range(pair_pointer);
        bool update(const int) noexcept;
    } active_list[CALLEE_SIZE + CALLER_SIZE];

    struct stack_slot {
        /* The sum of all live ranges. */
        std::set <std::pair <int,int>> live_range =
            { {-1,-1}, {INT32_MAX,INT32_MAX} };
        bool contains(int,int) const noexcept;
        void merge_range(pair_pointer);
    };

    std::vector <stack_slot> stack_map;


    void expire_old(int);
    void linear_allocate(pair_pointer);

    std::pair <int,int> find_best_pair(int,int) const;
    int find_best_stack(pair_pointer) const;

    std::pair <double,int> get_callee_spill(int);
    std::pair <double,int> get_caller_spill(int);

    int spill_callee(int);
    int spill_caller(int);

    void use_callee(pair_pointer,unsigned);
    void use_caller(pair_pointer,unsigned);
    void use_stack(pair_pointer,int);

    void paint_color(function *);

    void visitArithReg(arith_register *__ptr) override;
    void visitArithImm(arith_immediat *__ptr) override;
    void visitMoveReg(move_register *__ptr) override;
    void visitSltReg(slt_register *__ptr) override;
    void visitSltImm(slt_immediat *__ptr) override;
    void visitBoolNot(bool_not *__ptr) override;
    void visitBoolConv(bool_convert *__ptr) override;
    void visitLoadSym(load_symbol *__ptr) override;
    void visitLoadMem(load_memory *__ptr) override;
    void visitStoreMem(store_memory *__ptr) override;
    void visitCallFunc(function_node *__ptr) override;
    void visitRetExpr(ret_expression *__ptr) override;
    void visitJumpExpr(jump_expression *__ptr) override;
    void visitBranchExpr(branch_expression *__ptr) override;

    static physical_register *find_temporary() noexcept {
        static bool __last = false;
        return get_physical((__last = !__last) + 5);
    }

    std::array <physical_register *,CALLEE_SIZE> callee_save;
    std::array <physical_register *,CALLER_SIZE> caller_save;

    function *top_func;
    std::vector <node *> expr_list;

    physical_register *resolve_temporary(temporary_register *__tmp) {
        auto __dst = find_temporary();
        expr_list.push_back(new arith_immediat {
            arith_immediat::ADD,__tmp->reg,__tmp->offset,__dst
        });
        return __dst;
    }

    physical_register *resolve_virtual_use(virtual_register *__vir) {
        auto __tmp = vir_map.at(__vir);
        switch(__tmp.type) {
            case address_info::CALLEE:
                return callee_save[__tmp.index];
            case address_info::CALLER:
                return caller_save[__tmp.index];
            case address_info::TEMPORARY:
                return get_physical(__tmp.index);
            case address_info::STACK: break;
            default: runtime_assert("Unreachable!");
        }
        /* In-STACK now. */
        auto __dst = find_temporary();
        expr_list.push_back(new load_memory {
            memory_base::WORD,__dst,stack_address {top_func,__tmp.index}
        });
        return __dst;
    }

    physical_register *resolve_virtual_def(virtual_register *__vir) {
        if (vir_map.find(__vir) == vir_map.end())
            std::cerr << __vir->data() << '\n';
        auto __tmp = vir_map.at(__vir);
        switch(__tmp.type) {
            case address_info::CALLEE:
                return callee_save[__tmp.index];
            case address_info::CALLER:
                return caller_save[__tmp.index];
            case address_info::TEMPORARY:
                return get_physical(__tmp.index);
            case address_info::STACK: break;
            default: runtime_assert("Unreachable!");
        }
        /* In-STACK now. */
        auto __dst = find_temporary();
        expr_list.push_back(new store_memory {
            memory_base::WORD,__dst,stack_address {top_func,__tmp.index}
        });
        return __dst;
    }



    physical_register *resolve_use(Register *__reg) {
        if (auto __tmp = dynamic_cast <temporary_register *>(__reg))
            return resolve_temporary(__tmp);
        if (auto __vir = dynamic_cast <virtual_register *>(__reg))
            return resolve_virtual_use(__vir);
        return safe_cast <physical_register *>(__reg);
    }

    physical_register *resolve_def(Register *__reg) {
        if (auto __vir = dynamic_cast <virtual_register *>(__reg))
            return resolve_virtual_def(__vir);
        return safe_cast <physical_register *>(__reg);
    }


};


}