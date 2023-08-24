#include "mem2reg.h"


namespace dark::MEM {


void block_collector::visitFunction(IR::function *ctx) {
    for(auto __p : ctx->stmt) visitBlock(__p);
}
void block_collector::visitBlock(IR::block_stmt *ctx) {
    for(auto __p : ctx->stmt) visit(__p);
}

// do nothing
void block_collector::visitInit(IR::initialization *) {}
void block_collector::visitCompare(IR::compare_stmt *) {}
void block_collector::visitBinary(IR::binary_stmt *) {}
void block_collector::visitJump(IR::jump_stmt *) {}
void block_collector::visitBranch(IR::branch_stmt *) {}
void block_collector::visitCall(IR::call_stmt *) {}

void block_collector::visitLoad(IR::load_stmt *ctx) {

}

void block_collector::visitStore(IR::store_stmt *) {}
void block_collector::visitReturn(IR::return_stmt *) {}
void block_collector::visitAlloc(IR::allocate_stmt *) {}
void block_collector::visitGet(IR::get_stmt *) {}
void block_collector::visitPhi(IR::phi_stmt *) {}
void block_collector::visitUnreachable(IR::unreachable_stmt *) {}

}

namespace dark::MEM {

void block_updater::visitBlock(IR::block_stmt *) {}
void block_updater::visitFunction(IR::function *) {}
void block_updater::visitInit(IR::initialization *) {}

void block_updater::visitCompare(IR::compare_stmt *) {}
void block_updater::visitBinary(IR::binary_stmt *) {}
void block_updater::visitJump(IR::jump_stmt *) {}
void block_updater::visitBranch(IR::branch_stmt *) {}
void block_updater::visitCall(IR::call_stmt *) {}
void block_updater::visitLoad(IR::load_stmt *) {}
void block_updater::visitStore(IR::store_stmt *) {}
void block_updater::visitReturn(IR::return_stmt *) {}
void block_updater::visitAlloc(IR::allocate_stmt *) {}
void block_updater::visitGet(IR::get_stmt *) {}
void block_updater::visitPhi(IR::phi_stmt *) {}
void block_updater::visitUnreachable(IR::unreachable_stmt *) {}

}