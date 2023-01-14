#include "GVN.h"

#include "BasicBlock.h"
#include "Constant.h"
#include "DeadCode.h"
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace GVNExpression;
using std::string_literals::operator""s;
using std::shared_ptr;

static auto get_const_int_value = [](Value *v) { return dynamic_cast<ConstantInt *>(v)->get_value(); };
static auto get_const_fp_value = [](Value *v) { return dynamic_cast<ConstantFP *>(v)->get_value(); };
// Constant Propagation helper, folders are done for you
Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::add: return ConstantInt::get(get_const_int_value(value1) + get_const_int_value(value2), module_);
    case Instruction::sub: return ConstantInt::get(get_const_int_value(value1) - get_const_int_value(value2), module_);
    case Instruction::mul: return ConstantInt::get(get_const_int_value(value1) * get_const_int_value(value2), module_);
    case Instruction::sdiv: return ConstantInt::get(get_const_int_value(value1) / get_const_int_value(value2), module_);
    case Instruction::fadd: return ConstantFP::get(get_const_fp_value(value1) + get_const_fp_value(value2), module_);
    case Instruction::fsub: return ConstantFP::get(get_const_fp_value(value1) - get_const_fp_value(value2), module_);
    case Instruction::fmul: return ConstantFP::get(get_const_fp_value(value1) * get_const_fp_value(value2), module_);
    case Instruction::fdiv: return ConstantFP::get(get_const_fp_value(value1) / get_const_fp_value(value2), module_);

    case Instruction::cmp:
        switch (dynamic_cast<CmpInst *>(instr)->get_cmp_op()) {
        case CmpInst::EQ: return ConstantInt::get(get_const_int_value(value1) == get_const_int_value(value2), module_);
        case CmpInst::NE: return ConstantInt::get(get_const_int_value(value1) != get_const_int_value(value2), module_);
        case CmpInst::GT: return ConstantInt::get(get_const_int_value(value1) > get_const_int_value(value2), module_);
        case CmpInst::GE: return ConstantInt::get(get_const_int_value(value1) >= get_const_int_value(value2), module_);
        case CmpInst::LT: return ConstantInt::get(get_const_int_value(value1) < get_const_int_value(value2), module_);
        case CmpInst::LE: return ConstantInt::get(get_const_int_value(value1) <= get_const_int_value(value2), module_);
        }
    case Instruction::fcmp:
        switch (dynamic_cast<FCmpInst *>(instr)->get_cmp_op()) {
        case FCmpInst::EQ: return ConstantInt::get(get_const_fp_value(value1) == get_const_fp_value(value2), module_);
        case FCmpInst::NE: return ConstantInt::get(get_const_fp_value(value1) != get_const_fp_value(value2), module_);
        case FCmpInst::GT: return ConstantInt::get(get_const_fp_value(value1) > get_const_fp_value(value2), module_);
        case FCmpInst::GE: return ConstantInt::get(get_const_fp_value(value1) >= get_const_fp_value(value2), module_);
        case FCmpInst::LT: return ConstantInt::get(get_const_fp_value(value1) < get_const_fp_value(value2), module_);
        case FCmpInst::LE: return ConstantInt::get(get_const_fp_value(value1) <= get_const_fp_value(value2), module_);
        }
    default: return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::sitofp: return ConstantFP::get((float)get_const_int_value(value1), module_);
    case Instruction::fptosi: return ConstantInt::get((int)get_const_fp_value(value1), module_);
    case Instruction::zext: return ConstantInt::get((int)get_const_int_value(value1), module_);
    default: return nullptr;
    }
}

namespace utils {
static std::string print_congruence_class(const CongruenceClass &cc) {
    std::stringstream ss;
    if (cc.index_ == 0) {
        ss << "top class\n";
        return ss.str();
    }
    ss << "\nindex: " << cc.index_ << "\nleader: " << cc.leader_->print()
       << "\nvalue phi: " << (cc.value_phi_ ? cc.value_phi_->print() : "nullptr"s)
       << "\nvalue expr: " << (cc.value_expr_ ? cc.value_expr_->print() : "nullptr"s) << "\nmembers: {";
    for (auto &member : cc.members_)
        ss << member->print() << "; ";
    ss << "}\n";
    return ss.str();
}

static std::string dump_cc_json(const CongruenceClass &cc) {
    std::string json;
    json += "[\n";
    for (auto member : cc.members_) {
        if (auto c = dynamic_cast<Constant *>(member))
            json += member->print() + ", ";
        else
            json += "\"%" + member->get_name() + "\", ";
    }
    json += "\n]";
    return json;
}

static std::string dump_partition_json(const GVN::partitions &p) {
    std::string json;
    json += "[\n";
    for (auto cc : p)
        json += dump_cc_json(*cc) + ", \n";
    json += "\n]";
    return json;
}

static std::string dump_bb2partition(const std::map<BasicBlock *, GVN::partitions> &map) {
    std::string json;
    json += "{\n";
    for (auto [bb, p] : map)
        json += "\"" + bb->get_name() + "\": " + dump_partition_json(p) + ",";
    json += "\n}";
    return json;
}

// logging utility for you
static void print_partitions(const GVN::partitions &p) {
    if (p.empty()) {
        LOG_DEBUG << "empty partitions\n";
        //std::cout<<"empty partitions\n";
        return;
    }
    std::string log;
    for (auto &cc : p)
        log += print_congruence_class(*cc);
    LOG_DEBUG << log; // please don't use std::cout
    //std::cout<< log;
}
} // namespace utils


GVN::partitions GVN::join(const partitions &P1, const partitions &P2) {
    // TODO: do intersection pair-wise
    partitions P;
    for(auto &cc1 : P1) {
        for(auto &cc2 : P2) {
            // if one of the class is top, return the other
            if(!cc1->index_)
                return P2;

            if(!cc2->index_)
                return P1;

            auto cc = intersect(cc1, cc2);
            if(cc != nullptr) {
                P.insert(cc);
            }
        }
    }
    return P;
}

std::shared_ptr<CongruenceClass> GVN::intersect(std::shared_ptr<CongruenceClass> Ci,
                                                std::shared_ptr<CongruenceClass> Cj) {
    std::set<Value *> inter_mem;
    std::set_intersection(Ci->members_.begin(), Ci->members_.end(), Cj->members_.begin(), Cj->members_.end(),
                          std::inserter(inter_mem, inter_mem.begin()));
    if (*Ci == *Cj) return Ci; // if they are the same, return the same class
    if (inter_mem.empty())
        return nullptr;
    auto C = std::make_shared<CongruenceClass>(next_value_number_++);
    //C->index_ = ++index_;
    C->members_ = inter_mem;
    C->leader_ = *inter_mem.begin();
    
    if(Ci->leader_ == Cj->leader_) { // if they have the same leader, return the same class
        C->value_phi_ = Ci->value_phi_;
        C->value_expr_ = Ci->value_expr_;

        auto const_val = std::dynamic_pointer_cast<ConstantExpression>(C->value_expr_);
        if (const_val) {
            C->leader_ = const_val->get_constant();
        }
        C->value_phi_ = Ci->value_phi_;
    } else { // if they have different leaders, return a new class
        auto l = Ci->value_expr_;
        auto r = Cj->value_expr_;
        C->value_phi_ = PhiExpression::create(l, r);
        C->value_expr_ = C->value_phi_;
    }
    return C;
}

void GVN::detectEquivalences() {
    bool changed = false;
    GVN::initial();
    do {
        // see the pseudo code in documentation
        for (auto &bb : func_->get_basic_blocks()) { // you might need to visit the blocks in depth-first order
            // get PIN of bb by predecessor(s)
            // iterate through all instructions in the block
            // and the phi instruction in all the successors
            if(&bb == func_->get_entry_block())
                continue; 

            int bb_num = 1;
            partitions pin_l, pin_r;
            for(auto &bb_p : bb.get_pre_basic_blocks()){
                if(bb_num == 1) {
                    pin_l = clone(pout_[bb_p]);
                    pin_[&bb] = clone(pin_l);
                }
                else {
                    pin_r = join(pin_l, pout_[bb_p]);
                    pin_[&bb] = clone(pin_r);
                }
                bb_num++;
            }
            bb_num = 1;

            partitions pout_bb, pin_bb;
            pout_bb = clone(pin_[&bb]);
            for(auto &instr : bb.get_instructions())
            {
                if(instr.is_phi())
                    continue;
                pout_bb = transferFunction(&instr, pout_bb, bb);
                 
            }

            for(auto &bb_succ : bb.get_succ_basic_blocks())
            {
                for(auto &instr : bb_succ->get_instructions())
                {
                    if(!instr.is_phi())
                        continue;
                    pout_bb = transferFunction(&instr, pout_bb, bb);
                }
            }
            // check changes in pout
            changed = checkChanges(pout_bb, pout_[&bb]);
            pout_[&bb] = clone(pout_bb);  
        }
    } while (changed);
}

//partitions pout_entry;
void GVN::initial(void){
    partitions top;
    auto top_cc = GVN::createCongruenceClass();
    top.insert(top_cc);
    for(auto &bb0 : func_->get_basic_blocks()) {
        pout_[&bb0] = clone(top);
    }
    
    auto entry = func_->get_entry_block();
    pin_[entry] = {};

    pout_entry = clone(pin_[entry]);
    initial_gv();
    initial_args();
    for(auto &instr: entry->get_instructions()) {
        pout_entry = transferFunction(&instr, pout_entry, *entry); 
    }

    for(auto &bb_succ : entry->get_succ_basic_blocks()) {
        for(auto &instr : bb_succ->get_instructions()) {
            if(instr.is_phi())
                pout_entry = transferFunction(&instr, pout_entry, *entry);
        }
    }
    pout_[entry] = clone(pout_entry);
}

void GVN::initial_gv(){
    for(auto &gv : m_->get_global_variable()) {
        auto CC_N = GVN::createCongruenceClass(next_value_number_++);
        CC_N->leader_ = &gv;
        CC_N->members_.insert(&gv);
        CC_N->value_expr_ = GVariable::create(&gv);
        pout_entry.insert(CC_N);
    }
}

void GVN::initial_args(){
    for(auto &args : func_->get_args()) {
        auto CC_N = GVN::createCongruenceClass(next_value_number_++);
        CC_N->leader_ = args;
        CC_N->members_.insert(args);
        CC_N->value_expr_ = FuncArguments::create(args);
        pout_entry.insert(CC_N);
    }
}

bool GVN::checkChanges(partitions &pout_bb, partitions &pout_bb_old) {
    if(pout_bb == pout_bb_old)
        return false;
    else 
        return true;
}

std::shared_ptr<GVNExpression::Expression> GVN::valueExpr(Instruction *instr, partitions &pin, BasicBlock &bb) {
    // TODO
    std::shared_ptr<GVNExpression::Expression> le = nullptr, re = nullptr;

    if(instr->isBinary() || instr->is_cmp() || instr->is_fcmp()) {
        auto tmp = GVN::BIN_TYPE(instr, pin, bb);
        return tmp;
    }
    else if(instr->is_call()) {
        auto op0 = instr->get_operand(0);
        // if(ops->get_return_type()->is_void_type())
        //     return nullptr;
        auto func_op0 = dynamic_cast<Function*>(op0);
        bool is_pure = func_info_->is_pure_function(func_op0);
        if(func_op0->get_return_type()->is_void_type()) {
            return nullptr;
        }
        if(is_pure) {
            auto ops = instr->get_operands();
            return CallExpression::create(ops);
        }
    }
    else if(instr->is_fp2si() || instr->is_si2fp() || instr->is_zext()) {
        auto op1 = instr->get_operand(0);
        auto op1_const = dynamic_cast<Constant*>(op1);  //constant 
        if(op1_const) {
            return ConstantExpression::create(folder_->compute(instr, op1_const));  
        }
        auto le = GVN::getVE(pin, op1);
        auto le_const = std::dynamic_pointer_cast<ConstantExpression>(le);  //constant expression
        if(le_const) {
            return ConstantExpression::create(folder_->compute(instr, le_const->get_constant()));
        }
        return TypeConverseExpression::create(le, instr->get_instr_type());
    }
    else if(instr->is_gep()) {
        return GepExpression::create(instr->get_operands());
    }
    else if(instr->is_br() || instr->is_ret() || instr->is_store()) {
        return nullptr;
    }
    else if(instr->is_phi()) {
        auto lhs = instr->get_operand(0);
        auto rhs = instr->get_operand(2);
        auto lhs_const = dynamic_cast<Constant*>(lhs);
        auto rhs_const = dynamic_cast<Constant*>(rhs);
        auto label_l = dynamic_cast<BasicBlock*>(instr->get_operand(1));
        auto label_r = dynamic_cast<BasicBlock*>(instr->get_operand(3));
        auto le = GVN::getVE(pin, lhs);
        auto re = GVN::getVE(pin, rhs);
        
        if(label_l == &bb) {
            if(!le) {
                return ConstantExpression::create(lhs_const);
            }
            else {
                return le;
            }
        }
        else if(label_r == &bb) {
            if(!re) {
                return ConstantExpression::create(rhs_const);
            }
            else {
                return re;
            }
        }

    }
    return OtherExpression::create(next_otherExpression_number++);
}

std::shared_ptr<GVNExpression::Expression> GVN::BIN_TYPE(Instruction *instr, partitions &pin, BasicBlock &bb) {
    std::shared_ptr<GVNExpression::Expression> le = nullptr, re = nullptr;
    auto lhs = instr->get_operand(0);
    auto rhs = instr->get_operand(1);
    auto lhs_const = dynamic_cast<Constant*>(lhs);
    auto rhs_const = dynamic_cast<Constant*>(rhs);
    auto op = instr->get_instr_type();
    if(lhs == nullptr || rhs == nullptr){
        return nullptr;
    }
    if(lhs_const && rhs_const) {
        return ConstantExpression::create(folder_->compute(instr, lhs_const, rhs_const));
    }
    le = getVE(pin, lhs);
    re = getVE(pin, rhs);

    if(instr->isBinary()) {
        if(le && re) {
            bool le_IsConst, re_IsConst;
            if(le->get_expr_type() == le->e_constant)
                le_IsConst = true;
            else 
                le_IsConst = false;

            if(re->get_expr_type() == re->e_constant)
                re_IsConst = true;
            else 
                re_IsConst = false;

            if(le_IsConst && re_IsConst) {
            auto le_const=std::dynamic_pointer_cast<ConstantExpression>(le);
            auto re_const=std::dynamic_pointer_cast<ConstantExpression>(re);
            return ConstantExpression::create(folder_->compute(instr, le_const->get_constant(), re_const->get_constant()));
            }
            return BinaryExpression::create(op, le, re);
        }
        else if(!le && re) {
            bool re_IsConst;
            if(re->get_expr_type() == re->e_constant)
                re_IsConst = true;
            else 
                re_IsConst = false;
            le = ConstantExpression::create(lhs_const);
            auto le_const = std::dynamic_pointer_cast<ConstantExpression>(le);
            if(re_IsConst)
            {
                auto re_const = std::dynamic_pointer_cast<ConstantExpression>(re);
                return ConstantExpression::create(folder_->compute(instr, le_const->get_constant(), re_const->get_constant()));
            }
            return BinaryExpression::create(op, le, re);
        }
        else if(le && !re)
        {
            bool le_IsConst;
            if(le->get_expr_type() == le->e_constant)
                le_IsConst = true;
            else 
                le_IsConst = false;
            re = ConstantExpression::create(rhs_const);
            auto re_const = std::dynamic_pointer_cast<ConstantExpression>(re);
            if(le_IsConst)
            {
                auto le_const = std::dynamic_pointer_cast<ConstantExpression>(le);
                return ConstantExpression::create(folder_->compute(instr, le_const->get_constant(), re_const->get_constant()));
            }
            return BinaryExpression::create(op, le, re);
        }
    }
    else if(instr->is_cmp())
    {
        auto cmp_op = dynamic_cast<CmpInst *>(instr)->get_cmp_op();
        return CmpExpression::create(cmp_op, lhs, rhs);
    }  
    else if(instr->is_fcmp())
    {
        auto cmp_op =dynamic_cast<FCmpInst *>(instr)->get_cmp_op();
        return FCmpExpression::create(cmp_op, lhs, rhs);
    }
}

//instruction of the form `x = e`, mostly x is just e (SSA), but for copy stmt x is a phi instruction in the
//successor. Phi values (not copy stmt) should be handled in detectEquiv
/// \param bb basic block in which the transfer function is called
GVN::partitions GVN::transferFunction(Instruction *x, partitions pin, BasicBlock& bb) {
    partitions pout = clone(pin);
    // TODO: get different ValueExpr by Instruction::OpID, modify pout

    bool flag = false;
    if(!pout.empty()) {
        for(auto &cc : pout) {
            for(auto &member : cc->members_) {
                if(x == member) {
                    cc->members_.erase(x);
                    flag = true;
                    if(cc->members_.empty())
                        pout.erase(cc);
                    break;
                }
            }
            if(flag == true)
                break;
        }
    }

    auto val_exper = valueExpr(x, pin, bb);
    auto val_phi = valuePhiFunc(x, val_exper, pin, bb);
    if(!val_exper)
        return pout;

    bool is_InClass = false;
    if(!pout.empty()) {
        for(auto &cc : pout) {
            if((cc->value_expr_ && *(val_exper) == *(cc->value_expr_)) || (val_phi && cc->value_phi_ && *(val_phi) == *(cc->value_phi_))) {
                is_InClass = true;
                cc->members_.insert(x);
                break;
            }
        }
    }

    if(!is_InClass) {
        auto new_cc = GVN::createCongruenceClass(next_value_number_++);
        auto is_const = std::dynamic_pointer_cast<ConstantExpression>(val_exper);
        if(is_const) {
            auto ve_const = is_const->get_constant();
            new_cc->leader_ = ve_const;
        }
        else {
            new_cc->leader_ = x;
        }
        new_cc->members_ = {x};
        new_cc->value_expr_ = val_exper;
        new_cc->value_phi_ = val_phi;
        pout.insert(new_cc);
    }
    return pout;
}

std::shared_ptr<GVNExpression::PhiExpression> GVN::valuePhiFunc(Instruction* instr, std::shared_ptr<Expression> ve, const partitions &P, BasicBlock& bb) {
    // TODO:
    if(!ve)
        return nullptr;
    
    if(ve->get_expr_type() == Expression::e_bin) {
        auto ve_bin = std::dynamic_pointer_cast<BinaryExpression>(ve);
        auto lhs = ve_bin->get_lhs();
        auto rhs = ve_bin->get_rhs();
        auto phi_lhs = std::dynamic_pointer_cast<PhiExpression>(lhs);
        auto phi_rhs = std::dynamic_pointer_cast<PhiExpression>(rhs);
        if(phi_lhs && phi_rhs) {
            auto &pre_bb = bb.get_pre_basic_blocks();
            auto poutkl = pout_[pre_bb.front()];
            auto poutkr = pout_[pre_bb.back()];

            std::shared_ptr<GVNExpression::Expression> vi, vj;

            //left edge
            auto i1 = phi_lhs->get_lhs();
            auto i2 = phi_rhs->get_lhs();
            auto constant_i1 = std::dynamic_pointer_cast<ConstantExpression>(i1);
            auto constant_i2 = std::dynamic_pointer_cast<ConstantExpression>(i2);
            Constant *const_i1, *const_i2;
            if(constant_i1) {
                const_i1 = constant_i1->get_constant();
            }
            else {
                const_i1 = nullptr;
            }

            if(constant_i2) {
                const_i2 = constant_i2->get_constant();
            }
            else {
                const_i2 = nullptr;
            }

            if(const_i1 && const_i2) {
                vi = ConstantExpression::create(folder_->compute(instr, const_i1, const_i2));
            }
            else {
                auto le = BinaryExpression::create(ve_bin->get_op(), i1, i2);
                vi = getVN(poutkl, le);
                if(!vi) {
                    vi = valuePhiFunc(instr, le, poutkl, *pre_bb.front());
                }
            }

            //right edge
            auto j1 = phi_lhs->get_rhs();
            auto j2 = phi_rhs->get_rhs();
            auto constant_j1 = std::dynamic_pointer_cast<ConstantExpression>(j1);
            auto constant_j2 = std::dynamic_pointer_cast<ConstantExpression>(j2);
            Constant *const_j1, *const_j2;
            if(constant_j1) {
                const_j1 = constant_j1->get_constant();
            }
            else {
                const_j1 = nullptr;
            }

            if(constant_j2) {
                const_j2 = constant_j2->get_constant();
            }
            else {
                const_j2 = nullptr;
            }

            if(const_j1 && const_j2) {
                vj = ConstantExpression::create(folder_->compute(instr, const_j1, const_j2));
            }
            else {
                auto re = BinaryExpression::create(ve_bin->get_op(), j1, j2);
                vj = getVN(poutkr, re);
                if(!vj) {
                    vj = valuePhiFunc(instr, re, poutkr, *pre_bb.back());
                }
            }

            if(vi && vj) {
                return GVNExpression::PhiExpression::create(vi, vj);
            }
        }
    }
    return nullptr;
}

shared_ptr<Expression> GVN::getVN(const partitions &pout, shared_ptr<Expression> ve) {
    // TODO: return what?
    for (auto it = pout.begin(); it != pout.end(); it++)
        if ((*it)->value_expr_ and *(*it)->value_expr_ == *ve)
            return ve;
    return nullptr;
}

std::shared_ptr<GVNExpression::Expression> GVN::getVE(GVN::partitions &p, Value* v) {
    for(auto &c : p) {
        for(auto &member : c->members_) {
            if(v == member) {
                if(c->value_expr_)
                    return c->value_expr_;
                else   
                    return c->value_phi_;
            }
        }
    }
    return nullptr;
}

void GVN::initPerFunction() {
    next_value_number_ = 1;
    pin_.clear();
    pout_.clear();
}

void GVN::replace_cc_members() {
    for (auto &[_bb, part] : pout_) {
        auto bb = _bb; // workaround: structured bindings can't be captured in C++17
        for (auto &cc : part) {
            if (cc->index_ == 0)
                continue;
            // if you are planning to do constant propagation, leaders should be set to constant at some point
            for (auto &member : cc->members_) {
                bool member_is_phi = dynamic_cast<PhiInst *>(member);
                bool value_phi = cc->value_phi_ != nullptr;
                if (member != cc->leader_ and (value_phi or !member_is_phi)) {
                    // only replace the members if users are in the same block as bb
                    member->replace_use_with_when(cc->leader_, [bb](User *user) {
                        if (auto instr = dynamic_cast<Instruction *>(user)) {
                            auto parent = instr->get_parent();
                            auto &bb_pre = parent->get_pre_basic_blocks();
                            if (instr->is_phi()) // as copy stmt, the phi belongs to this block
                                return std::find(bb_pre.begin(), bb_pre.end(), bb) != bb_pre.end();
                            else
                                return parent == bb;
                        }
                        return false;
                    });
                }
            }
        }
    }
    return;
}

// top-level function, done for you
void GVN::run() {
    std::ofstream gvn_json;
    if (dump_json_) {
        gvn_json.open("gvn.json", std::ios::out);
        gvn_json << "[";
    }

    folder_ = std::make_unique<ConstFolder>(m_);
    func_info_ = std::make_unique<FuncInfo>(m_);
    func_info_->run();
    dce_ = std::make_unique<DeadCode>(m_);
    dce_->run(); // let dce take care of some dead phis with undef

    for (auto &f : m_->get_functions()) {
        if (f.get_basic_blocks().empty())
            continue;
        func_ = &f;
        initPerFunction();
        LOG_INFO << "Processing " << f.get_name();
        detectEquivalences();
        LOG_INFO << "===============pin=========================\n";
        for (auto &[bb, part] : pin_) {
            LOG_INFO << "\n===============bb: " << bb->get_name() << "=========================\npartitionIn: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        LOG_INFO << "\n===============pout=========================\n";
        for (auto &[bb, part] : pout_) {
            LOG_INFO << "\n=====bb: " << bb->get_name() << "=====\npartitionOut: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        if (dump_json_) {
            gvn_json << "{\n\"function\": ";
            gvn_json << "\"" << f.get_name() << "\", ";
            gvn_json << "\n\"pout\": " << utils::dump_bb2partition(pout_);
            gvn_json << "},";
        }
        replace_cc_members(); // don't delete instructions, just replace them
    }
    dce_->run(); // let dce do that for us
    if (dump_json_)
        gvn_json << "]";
}

template <typename T>
static bool equiv_as(const Expression &lhs, const Expression &rhs) {
    // we use static_cast because we are very sure that both operands are actually T, not other types.
    return static_cast<const T *>(&lhs)->equiv(static_cast<const T *>(&rhs));
}

bool GVNExpression::operator==(const Expression &lhs, const Expression &rhs) {
    if (lhs.get_expr_type() != rhs.get_expr_type())
        return false;
    switch (lhs.get_expr_type()) {
    case Expression::e_constant: return equiv_as<ConstantExpression>(lhs, rhs);
    case Expression::e_bin: return equiv_as<BinaryExpression>(lhs, rhs);
    case Expression::e_phi: return equiv_as<PhiExpression>(lhs, rhs);
    case Expression::e_cmp: return equiv_as<CmpExpression>(lhs,rhs);
    case Expression::e_call: return equiv_as<CallExpression>(lhs,rhs);
    case Expression::e_typeConverse: return equiv_as<TypeConverseExpression>(lhs,rhs);
    case Expression::e_gep: return equiv_as<GepExpression>(lhs,rhs);
    case Expression::e_args: return equiv_as<FuncArguments>(lhs,rhs);
    case Expression::e_other: return equiv_as<OtherExpression>(lhs,rhs);
    case Expression::e_global: return equiv_as<GVariable>(lhs,rhs);
    case Expression::e_fcmp: return equiv_as<FCmpExpression>(lhs,rhs);
    }
}

bool GVNExpression::operator==(const shared_ptr<Expression> &lhs, const shared_ptr<Expression> &rhs) {
    if (lhs == nullptr and rhs == nullptr) // is the nullptr check necessary here?
        return true;
    if(lhs and rhs)
        return *lhs == *rhs;
    return false;
}

GVN::partitions GVN::clone(const partitions &p) {
    partitions data;
    for (auto &cc : p) {
        data.insert(std::make_shared<CongruenceClass>(*cc));
    }
    return data;
}

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2) {
    // TODO: how to compare partitions?
    if(p1.size() != p2.size()) return false;
    auto p1_ = p1.begin();
    auto p2_ = p2.begin();
    
    while(p1_ != p1.end() and p2_ != p2.end()){
        if(!(**p1_ == **p2_)) return false;
        p1_++;
        p2_++;
    }
    return true;
}

bool CongruenceClass::operator==(const CongruenceClass &other) const {
    // TODO: which fields need to be compared?
    if (this->members_.size() != other.members_.size())
        return false;
    for (auto &member : this->members_) {
        if (other.members_.find(member) == other.members_.end())
            return false;
    }
    for (auto &member : other.members_) {
        if (this->members_.find(member) == this->members_.end())
            return false;
    }
    return true;
}

