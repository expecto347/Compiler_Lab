#pragma once
#include "BasicBlock.h"
#include "Constant.h"
#include "DeadCode.h"
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "Module.h"
#include "PassManager.hpp"
#include "Value.h"
#include "IRprinter.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GVNExpression {

// fold the constant value
class ConstFolder {
  public:
    ConstFolder(Module *m) : module_(m) {}
    Constant *compute(Instruction *instr, Constant *value1, Constant *value2);
    Constant *compute(Instruction *instr, Constant *value1);

  private:
    Module *module_;
};

/**
 * for constructor of class derived from `Expression`, we make it public
 * because `std::make_shared` needs the constructor to be publicly available,
 * but you should call the static factory method `create` instead the constructor itself to get the desired data
 */
class Expression {
  public:
    // TODO: you need to extend expression types according to testcases
    enum gvn_expr_t { e_constant, e_bin, e_phi, e_cmp, e_fcmp, e_call, e_gep, e_typeConverse, e_other, e_global, e_args};
    Expression(gvn_expr_t t) : expr_type(t) {}
    virtual ~Expression() = default;
    virtual std::string print() = 0;
    gvn_expr_t get_expr_type() const { return expr_type; }

  private:
    gvn_expr_t expr_type;
};

bool operator==(const std::shared_ptr<Expression> &lhs, const std::shared_ptr<Expression> &rhs);
bool operator==(const GVNExpression::Expression &lhs, const GVNExpression::Expression &rhs);

class ConstantExpression : public Expression {
  public:
    static std::shared_ptr<ConstantExpression> create(Constant *c) { return std::make_shared<ConstantExpression>(c); }
    virtual std::string print() { return c_->print(); }
    // we leverage the fact that constants in lightIR have unique addresses
    bool equiv(const ConstantExpression *other) const { return c_ == other->c_; }
    ConstantExpression(Constant *c) : Expression(e_constant), c_(c) {}
    Constant *get_constant() const { return c_; }

  private:
    Constant *c_;
};

// arithmetic expression
class BinaryExpression : public Expression {
  public:
    static std::shared_ptr<BinaryExpression> create(Instruction::OpID op,
                                                    std::shared_ptr<Expression> lhs,
                                                    std::shared_ptr<Expression> rhs) {
        return std::make_shared<BinaryExpression>(op, lhs, rhs);
    }
    virtual std::string print() {
        return "(" + Instruction::get_instr_op_name(op_) + " " + lhs_->print() + " " + rhs_->print() + ")";
    }

    bool equiv(const BinaryExpression *other) const {
        if (op_ == other->op_ and *lhs_ == *other->lhs_ and *rhs_ == *other->rhs_)
            return true;
        else
            return false;
    }

    std::shared_ptr<Expression> get_lhs() {
      return lhs_;
    }

    std::shared_ptr<Expression> get_rhs() {
      return rhs_;
    }

    Instruction::OpID get_op() {
      return op_;
    }

    BinaryExpression(Instruction::OpID op, std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_bin), op_(op), lhs_(lhs), rhs_(rhs) {}

  private:
    Instruction::OpID op_;
    std::shared_ptr<Expression> lhs_, rhs_;
};

class PhiExpression : public Expression {
  public:
    static std::shared_ptr<PhiExpression> create(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs) {
        return std::make_shared<PhiExpression>(lhs, rhs);
    }
    virtual std::string print() { return "(phi " + lhs_->print() + " " + rhs_->print() + ")"; }
    bool equiv(const PhiExpression *other) const {
        if (*lhs_ == *other->lhs_ and *rhs_ == *other->rhs_)
            return true;
        else
            return false;
    }

    std::shared_ptr<Expression> get_lhs() {
      return lhs_;
    }

    std::shared_ptr<Expression> get_rhs() {
      return rhs_;
    }

    PhiExpression(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs)
        : Expression(e_phi), lhs_(lhs), rhs_(rhs) {}

  private:
    std::shared_ptr<Expression> lhs_, rhs_;
};

class CmpExpression : public Expression {
  public:
    static std::shared_ptr<CmpExpression> create(CmpInst::CmpOp op, Value* lhs, Value* rhs) {
      return std::make_shared<CmpExpression>(op, lhs, rhs);
    }
    virtual std::string print(){
      return {};
    }
    bool equiv(const CmpExpression *other) const {
        if (op_ == other->op_ and lhs_ == other->lhs_ and rhs_ == other->rhs_)
            return true;
        else
            return false;
    }

    CmpExpression(CmpInst::CmpOp op, Value* lhs, Value* rhs)
        : Expression(e_cmp), op_(op), lhs_(lhs), rhs_(rhs) {}

    private:
      CmpInst::CmpOp op_;
      Value *lhs_, *rhs_;
};

class FCmpExpression : public Expression {
  public:
    static std::shared_ptr<FCmpExpression> create(FCmpInst::CmpOp op, Value* lhs, Value* rhs) {
      return std::make_shared<FCmpExpression>(op, lhs, rhs);
    }
    virtual std::string print(){
      return {};
    }
    bool equiv(const FCmpExpression *other) const {
        if (op_ == other->op_ and lhs_ == other->lhs_ and rhs_ == other->rhs_)
            return true;
        else
            return false;
    }

    FCmpExpression(FCmpInst::CmpOp op, Value* lhs, Value* rhs)
        : Expression(e_fcmp), op_(op), lhs_(lhs), rhs_(rhs) {}

    private:
      FCmpInst::CmpOp op_;
      Value *lhs_, *rhs_;
};

class CallExpression : public Expression {
  public:
    static std::shared_ptr<CallExpression> create( std::vector<Value*> f) {
        return std::make_shared<CallExpression>(f);
    }
    virtual std::string print() {
      return {};
    }

    bool equiv(const CallExpression *other) const {
        return std::equal(this->func.begin(), this->func.end(), other->func.begin(), other->func.end());
    }

    CallExpression(std::vector<Value*> f)
        : Expression(e_call),func(f) {}

  private:
    std::vector<Value*> func;
};

class GepExpression : public Expression {
  public:
    static std::shared_ptr<GepExpression> create(std::vector<Value*> ptr) {
        return std::make_shared<GepExpression>(ptr);
    }
    virtual std::string print() {
      return {};
    }

    bool equiv(const GepExpression *other) const {
        return std::equal(this->ptr_.begin(), this->ptr_.end(), other->ptr_.begin(), other->ptr_.end());
    }

    GepExpression(std::vector<Value*> ptr) : Expression(e_gep),ptr_(ptr) {}

  private:
    std::vector<Value*> ptr_;
};

class TypeConverseExpression : public Expression{
  public:
    static std::shared_ptr<TypeConverseExpression> create(std::shared_ptr<Expression> exp, Instruction::OpID op) {
        return std::make_shared<TypeConverseExpression>(exp, op);
    }
    virtual std::string print() { 
      return {};
    }
    bool equiv(const TypeConverseExpression *other) const {
        if (*exp_ == *other->exp_ && op_==other->op_)
            return true;
        else
            return false;
    }
    TypeConverseExpression(std::shared_ptr<Expression> exp, Instruction::OpID op)
        : Expression(e_typeConverse), exp_(exp), op_(op) {}
private:
std::shared_ptr<Expression> exp_;
Instruction::OpID op_; 
};

class OtherExpression : public Expression {
  public:
    static std::shared_ptr<OtherExpression> create(int id) { 
      return std::make_shared<OtherExpression>(id); }
    virtual std::string print() { 
      return {};
    }
    bool equiv(const OtherExpression *other) const { 
      return this->id == other->id; 
    }
    OtherExpression(int id) : Expression(e_other), id(id){}
  private:
    int id;

};

class GVariable : public Expression {
  public:
    static std::shared_ptr<GVariable> create(GlobalVariable* gv) {
      return std::make_shared<GVariable>(gv);
    }

    virtual std::string print() {
      return {};
    }

    bool equiv(const GVariable *other) const {
      if(gv_ == other->gv_)
        return true;
      else 
        return false;
    }
    GVariable(GlobalVariable* gv) : Expression(e_global), gv_(gv) {}

  private:
    GlobalVariable* gv_;
};

class FuncArguments: public Expression {
  public:
    static std::shared_ptr<FuncArguments> create(Argument* arg) {
      return std::make_shared<FuncArguments>(arg);
    }

    virtual std::string print() {
      return {};
    }

    bool equiv(const FuncArguments* other) const {
      if(arg_ == other->arg_)
        return true;
      else
        return false;
    }

    FuncArguments(Argument* arg) : Expression(e_args), arg_(arg) {}
  private:
    Argument* arg_;
};

} // namespace GVNExpression

/**
 * Congruence class in each partitions
 * note: for constant propagation, you might need to add other fields
 * and for load/store redundancy detection, you most certainly need to modify the class
 */
struct CongruenceClass {
    size_t index_;
    // representative of the congruence class, used to replace all the members (except itself) when analysis is done
    Value *leader_;
    // value expression in congruence class
    std::shared_ptr<GVNExpression::Expression> value_expr_;
    // value φ-function is an annotation of the congruence class
    std::shared_ptr<GVNExpression::PhiExpression> value_phi_;
    // equivalent variables in one congruence class
    std::set<Value *> members_;

    CongruenceClass(size_t index) : index_(index), leader_{}, value_expr_{}, value_phi_{}, members_{} {}

    bool operator<(const CongruenceClass &other) const { return this->index_ < other.index_; }
    bool operator==(const CongruenceClass &other) const;
};

namespace std {
template <>
// overload std::less for std::shared_ptr<CongruenceClass>, i.e. how to sort the congruence classes
struct less<std::shared_ptr<CongruenceClass>> {
    bool operator()(const std::shared_ptr<CongruenceClass> &a, const std::shared_ptr<CongruenceClass> &b) const {
        // nullptrs should never appear in partitions, so we just dereference it
        return *a < *b;
    }
};
} // namespace std

class GVN : public Pass {
  public:
    using partitions = std::set<std::shared_ptr<CongruenceClass>>;
    GVN(Module *m, bool dump_json) : Pass(m), dump_json_(dump_json) {}
    // pass start
    void run() override;
    // init for pass metadata;
    void initPerFunction();

    // fill the following functions according to Pseudocode, **you might need to add more arguments**
    void detectEquivalences();
    void CountInstr();
    partitions join(const partitions &P1, const partitions &P2);
    std::shared_ptr<CongruenceClass> intersect(std::shared_ptr<CongruenceClass>, std::shared_ptr<CongruenceClass>);
    GVN::partitions transferFunction(Instruction *x, partitions pin,BasicBlock &bb);
    std::shared_ptr<GVNExpression::PhiExpression> valuePhiFunc(Instruction* instr,std::shared_ptr<GVNExpression::Expression>,
                                                               const partitions &,BasicBlock &bb);
    std::shared_ptr<GVNExpression::Expression> valueExpr(Instruction *instr,partitions &pin,BasicBlock &bb);
    std::shared_ptr<GVNExpression::Expression> getVN(const partitions &pout,
                                                     std::shared_ptr<GVNExpression::Expression> ve);
    std::shared_ptr<GVNExpression::Expression> getVE(GVN::partitions &p, Value* v);
    // replace cc members with leader
    void replace_cc_members();

    // note: be careful when to use copy constructor or clone
    partitions clone(const partitions &p);

    // create congruence class helper
    std::shared_ptr<CongruenceClass> createCongruenceClass(size_t index = 0) {
        return std::make_shared<CongruenceClass>(index);
    }

  private:
    bool dump_json_;
    std::uint64_t next_otherExpression_number = 1;
    std::uint64_t next_value_number_ = 1;
    Function *func_;
    std::map<BasicBlock *, partitions> pin_, pout_;
    std::unique_ptr<FuncInfo> func_info_;
    std::unique_ptr<GVNExpression::ConstFolder> folder_;
    std::unique_ptr<DeadCode> dce_;
    void initial(void);
    GVN::partitions pout_entry;
    void initial_gv();
    void initial_args();
    void initial_inst();
    void initial_bb_succ();
    bool checkChanges(partitions &pout_bb, partitions &pout_bb_old);
    std::shared_ptr<GVNExpression::Expression> BIN_TYPE(Instruction *instr, partitions &pin, BasicBlock &bb);
    //std::shared_ptr<GVNExpression::Expression> CALL_TYPE(Instruction *instr, partitions &pin, BasicBlock &bb);
    //std::shared_ptr<GVNExpression::Expression> CAST_TYPE(Instruction *instr, partitions &pin, BasicBlock &bb);
    //std::shared_ptr<GVNExpression::Expression> PHI_TYPE(Instruction *instr, partitions &pin, BasicBlock &bb);
    //std::shared_ptr<GVNExpression::PhiExpression> phiExpr_branch(Instruction* instr, std::shared_ptr<GVNExpression::Expression> ve, const partitions &P, BasicBlock& bb);
};

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2);
