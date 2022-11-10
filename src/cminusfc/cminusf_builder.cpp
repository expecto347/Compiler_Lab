#include "cminusf_builder.hpp"
#include"logging.hpp" // log

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())


// TODO: Global Variable Declarations 
// You can define global variables here
// to store state. You can expand these
// definitions if you need to.

//存储当前value
Value *tmp_val = nullptr;
// 当前函数
Function *cur_fun = nullptr;

// 表示是否在之前已经进入scope，用于CompoundStmt
// 进入CompoundStmt不仅包括通过Fundeclaration进入，也包括selection-stmt等。
// pre_enter_scope用于控制是否在CompoundStmt中添加scope.enter,scope.exit
bool pre_enter_scope = false;

bool var_l = false; // 表示当前变量是否是左值


// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl : node.declarations) {
        decl->accept(*this); // visit each declaration
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    if(node.type == TYPE_INT) {
        tmp_val = CONST_INT(node.i_val);
    } 
    else {
        tmp_val = CONST_FP(node.f_val);
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *tem_type;
    if(node.type == TYPE_INT) {
        tem_type = INT32_T;
    } 
    else {
        tem_type = FLOAT_T;
    } // set the type of the variable

    if(node.num == nullptr){
        // if the variable is not an array
        if(scope.in_global()){
            // global variable
            auto init = ConstantZero::get(tem_type, module.get());
            auto var = GlobalVariable::create(node.id, module.get(), tem_type, false, init);
            scope.push(node.id, var);
        }
        else {
            // local variable
            auto var = builder->create_alloca(tem_type);
            scope.push(node.id, var);
        }
    }
    else{
        // if the variable is an array
        if(scope.in_global()){
            // global variable
            auto array_t = ArrayType::get(tem_type, node.num->i_val);
            auto init = ConstantZero::get(array_t, module.get());
            auto var = GlobalVariable::create(node.id, module.get(), array_t, false, init);
            scope.push(node.id, var);
        }
        else{
            // local variable
            auto array_t = ArrayType::get(tem_type, node.num->i_val);
            auto var = builder->create_alloca(array_t);
            scope.push(node.id, var);
        }
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params) {
        if(param->type == TYPE_INT) {
            if(param->isarray){
                param_types.push_back(INT32PTR_T);
            }
            else{
                param_types.push_back(INT32_T);
            }
        } 
        else {
            if(param->isarray){
                param_types.push_back(FLOATPTR_T);
            }
            else{
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value *> args;
    for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++) {
        args.push_back(*arg); // store the arguments
    }

    for (int i = 0; i < node.params.size(); ++i) {
        if(node.params[i]->type == TYPE_INT){
            // if the parameter is an integer
            if(node.params[i]->isarray) {
                // if the parameter is an array
                auto var = builder->create_alloca(INT32PTR_T);
                builder->create_store(args[i], var);
                scope.push(node.params[i]->id, var, true); //isFuncParam = 1, which indicates that the variable is a function parameter
            }
            else {
                auto var = builder->create_alloca(INT32_T);
                builder->create_store(args[i], var);
                scope.push(node.params[i]->id, var, true);
            }
        }
        else {
            // if the parameter is a float
            if(node.params[i]->isarray) {
                auto var = builder->create_alloca(FLOATPTR_T);
                builder->create_store(args[i], var);
                scope.push(node.params[i]->id, var, true);
            }
            else{
                auto var = builder->create_alloca(FLOAT_T);
                builder->create_store(args[i], var);
                scope.push(node.params[i]->id, var, true);
            }
        }

    }

    node.compound_stmt->accept(*this); // visit the compound statement

    // if the function has no return statement
    if (builder->get_insert_block()->get_terminator() == nullptr) {
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) {
    // do nothing
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {  
bool need_exit_scope = !pre_enter_scope;//添加need_exit_scope变量
    if (pre_enter_scope) { // if the scope is entered before when function declaration
        pre_enter_scope = false;
    } else {
        scope.enter();
    }

    for (auto& decl: node.local_declarations) {//compound-stmt -> { local-declarations statement-list }
        decl->accept(*this);
    }

    for (auto& stmt: node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }

    if (need_exit_scope) {
        scope.exit();
    }
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    // expression-stmt→expression ; ∣ ;
    if(node.expression != nullptr){
        node.expression->accept(*this);
    } // if the expression is not null, visit it
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    // selection-stmt→if ( expression ) statement ∣ if ( expression ) statement else statement
    node.expression->accept(*this); // visit the expression

    Value *cond;

    auto mergeBB = BasicBlock::create(module.get(), "", cur_fun); // create the basic blocks
    
    if(tmp_val->get_type()->is_float_type()) { // if the expression is a float
        cond = builder->create_fcmp_ne(tmp_val, CONST_FP(0.));
    }
    else {
        cond = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }

    if(node.else_statement == nullptr) { // if there is no else statement
        auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
        builder->create_cond_br(cond, trueBB, mergeBB); // create the conditional branch
        builder->set_insert_point(trueBB);
        node.if_statement->accept(*this); // visit the if statement
        builder->create_br(mergeBB);
    }
    else {
        auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
        auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
        builder->create_cond_br(cond, trueBB, falseBB); // create the conditional branch
        builder->set_insert_point(trueBB);
        node.if_statement->accept(*this); // visit the if statement
        builder->create_br(mergeBB); // create the branch
        builder->set_insert_point(falseBB);
        node.else_statement->accept(*this); // visit the else statement
        builder->create_br(mergeBB);
    }

    builder->set_insert_point(mergeBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    // iteration-stmt→while ( expression ) statement
    Value *cond;

    auto loopBB = BasicBlock::create(module.get(), "", cur_fun);
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun); // create the basic blocks

    builder->create_br(loopBB);
    builder->set_insert_point(loopBB);
    node.expression->accept(*this); // visit the expression

    if(tmp_val->get_type()->is_float_type()) { // if the expression is a float
        cond = builder->create_fcmp_ne(tmp_val, CONST_FP(0.)); // create the comparison
    }
    else {
        cond = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    }

    builder->create_cond_br(cond, trueBB, falseBB); // create the conditional branch
    builder->set_insert_point(trueBB);
    node.statement->accept(*this); // visit the statement
    builder->create_br(loopBB); // create the branch
    builder->set_insert_point(falseBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
    } 
    else {
        node.expression->accept(*this);

        // return the value
        // 类型转换
        if(cur_fun->get_return_type()->is_float_type()) {
            if(tmp_val->get_type()->is_float_type()) {
                builder->create_ret(tmp_val);
            }
            else {
                builder->create_ret(builder->create_sitofp(tmp_val, FLOAT_T));
            }
        }
        else {
            if(tmp_val->get_type()->is_float_type()) {
                builder->create_ret(builder->create_fptosi(tmp_val, INT32_T));
            }
            else {
                builder->create_ret(tmp_val);
            }
        }
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    // var→ID ∣ ID [ expression ]
    auto var_l_tmp = var_l;
    var_l = false;
    if(var_l_tmp){
        if(node.expression == nullptr) { // if the expression is null, that means it is a variable
            tmp_val = scope.find(node.id);
            if(tmp_val == nullptr) {
                LOG(ERROR) << "Variable " << node.id << " is not defined.";
                builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {});
                if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }
            }
        }
        else {
            node.expression->accept(*this); // visit the expression
            auto ptr = scope.find(node.id); // get the pointer
            if(ptr == nullptr) {
                LOG(ERROR) << "Variable " << node.id << " is not defined.";
                builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {});
                if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }
            }

            // check the indexd
            Value *cond;
            if(tmp_val->get_type()->is_float_type()) 
                tmp_val = builder->create_fptosi(tmp_val, INT32_T); // convert the value to int
            
            auto normalBB = BasicBlock::create(module.get(), "", cur_fun);
            auto exceptionBB = BasicBlock::create(module.get(), "", cur_fun); // create the basic blocks

            cond = builder->create_icmp_lt(tmp_val, CONST_INT(0)); // check if the index is negative
            builder->create_cond_br(cond, exceptionBB, normalBB); // create the conditional branch

            builder->set_insert_point(exceptionBB);
            builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {}); // call the exception function
            if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }

            builder->set_insert_point(normalBB);
            if(scope.is_func_param(node.id)) { // if the variable is a function parameter
                ptr = builder->create_load(ptr);
                tmp_val = builder->create_gep(ptr, {tmp_val});
            }
            else {
                tmp_val = builder->create_gep(ptr, {CONST_INT(0), tmp_val}); // 我们仅仅在传递一个函数参数的时候需要用第一种赋值方法
            }
        }
    }
    else{
        if(node.expression == nullptr){ // if the expression is null, that means it is a variable
            tmp_val = scope.find(node.id);
            if(tmp_val->get_type()->get_pointer_element_type()->is_array_type())
                tmp_val = builder->create_gep(tmp_val, {CONST_INT(0), CONST_INT(0)}); //if the variable is a pointer point to an array, we need to get the first element
            else if(tmp_val == nullptr) {
                LOG(ERROR) << "Variable " << node.id << " is not defined.";
                builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {});
                if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }
            }
            else{
                tmp_val = builder->create_load(tmp_val);
            }
        }
        else{
            node.expression->accept(*this); // visit the expression
            auto ptr = scope.find(node.id); // get the pointer
            if(ptr == nullptr) {
                LOG(ERROR) << "Variable " << node.id << " is not defined.";
                builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {});
                if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }
            }

            // check the indexd
            Value *cond;
            if(tmp_val->get_type()->is_float_type()) 
                tmp_val = builder->create_fptosi(tmp_val, INT32_T); // convert the value to int
            
            auto normalBB = BasicBlock::create(module.get(), "", cur_fun);
            auto exceptionBB = BasicBlock::create(module.get(), "", cur_fun); // create the basic blocks

            cond = builder->create_icmp_lt(tmp_val, CONST_INT(0)); // check if the index is negative
            builder->create_cond_br(cond, exceptionBB, normalBB); // create the conditional branch

            builder->set_insert_point(exceptionBB);
            builder->create_call(static_cast<Function *>(scope.find("neg_idx_except")), {}); // call the exception function
            if(cur_fun->get_return_type()->is_void_type()) {
                    builder->create_void_ret();
                }
                else if(cur_fun->get_return_type()->is_float_type()) {
                    builder->create_ret(CONST_FP(0.0));
                }
                else {
                    builder->create_ret(CONST_INT(0));
                }

            builder->set_insert_point(normalBB);
            if(scope.is_func_param(node.id)) { // if the variable is a function parameter
                ptr = builder->create_load(ptr);
                tmp_val = builder->create_gep(ptr, {tmp_val});
                tmp_val = builder->create_load(tmp_val);
            }
            else {
                tmp_val = builder->create_gep(ptr, {CONST_INT(0), tmp_val}); // 我们仅仅在传递一个函数参数的时候需要用第一种赋值方法
                tmp_val = builder->create_load(tmp_val);
            }
        }
    }
    var_l = false;
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    // assign-expression→var = expression
    // 我们需要先访问var，然后再访问expression
    // 其中如果访问var, 我们希望tmp_val是一个指向指针的指针，而访问expression，我们希望tmp_val是一个指向值的指针
    var_l = true;
    node.var->accept(*this); // visit the var
    auto ptr = tmp_val;
    var_l = false; // set the flag to false
    node.expression->accept(*this); // visit the expression
    auto val = tmp_val;

    // 类型转换
    if(ptr->get_type()->get_pointer_element_type() != val->get_type()) {
        if(ptr->get_type()->get_pointer_element_type()->is_float_type()) {
                builder->create_store(builder->create_sitofp(val, FLOAT_T), ptr);
                tmp_val = builder->create_load(ptr);
        }
        else {
                builder->create_store(builder->create_fptosi(val, INT32_T), ptr);
                tmp_val = builder->create_load(ptr);
        }
    }
    else {
        builder->create_store(val, ptr);
        tmp_val = val;
    }
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    // simple-expression→additive-expression relop additive-expression ∣ additive-expression
    if(node.additive_expression_r == nullptr) node.additive_expression_l->accept(*this);

    else {
        node.additive_expression_l->accept(*this); // visit the left additive expression
        auto val_l = tmp_val;
        node.additive_expression_r->accept(*this); // visit the right additive expression
        auto val_r = tmp_val;

        bool isIntParam;

        // 类型转换
       if(val_l->get_type() != val_r->get_type()){
            if(val_l->get_type()->is_float_type()) {
                val_r = builder->create_sitofp(val_r, FLOAT_T);
                isIntParam = false;
            }
            else {
                val_l = builder->create_sitofp(val_l, FLOAT_T);
                isIntParam = false;
            }
        }
        else{
            isIntParam = val_l->get_type()->is_integer_type();
        }

        if(isIntParam){
            switch(node.op) {
                case OP_EQ: // ==
                    tmp_val = builder->create_icmp_eq(val_l, val_r);
                    break; 
                case OP_NEQ: // !=
                    tmp_val = builder->create_icmp_ne(val_l, val_r);
                    break;
                case OP_LT: // <
                    tmp_val = builder->create_icmp_lt(val_l, val_r);
                    break;
                case OP_LE: // <=
                    tmp_val = builder->create_icmp_le(val_l, val_r);
                    break;
                case OP_GT: // >
                    tmp_val = builder->create_icmp_gt(val_l, val_r);
                    break;
                case OP_GE: // >=
                    tmp_val = builder->create_icmp_ge(val_l, val_r);
                    break;
            }
        }
        else{
            switch(node.op) {
                case OP_EQ: // ==
                    tmp_val = builder->create_fcmp_eq(val_l, val_r);
                    break; 
                case OP_NEQ: // !=
                    tmp_val = builder->create_fcmp_ne(val_l, val_r);
                    break;
                case OP_LT: // <
                    tmp_val = builder->create_fcmp_lt(val_l, val_r);
                    break;
                case OP_LE: // <=
                    tmp_val = builder->create_fcmp_le(val_l, val_r);
                    break;
                case OP_GT: // >
                    tmp_val = builder->create_fcmp_gt(val_l, val_r);
                    break;
                case OP_GE: // >=
                    tmp_val = builder->create_fcmp_ge(val_l, val_r);
                    break;
            }
        }
        tmp_val = builder->create_zext(tmp_val, INT32_T); // convert the value to int
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    // additive-expression→additive-expression addop term ∣ term
    if(node.additive_expression == nullptr) node.term->accept(*this);
    
    else {
        node.additive_expression->accept(*this); // visit the left term
        auto val_l = tmp_val;
        node.term->accept(*this); // visit the right term
        auto val_r = tmp_val;

        bool isIntParam;

        // 类型转换
        if(val_l->get_type() != val_r->get_type()){
            if(val_l->get_type()->is_float_type()) {
                val_r = builder->create_sitofp(val_r, FLOAT_T);
                isIntParam = false;
            }
            else {
                val_l = builder->create_sitofp(val_l, FLOAT_T);
                isIntParam = false;
            }
        }
        else{
            isIntParam = val_l->get_type()->is_integer_type();
        }

        if(isIntParam){
            switch(node.op) {
                case OP_PLUS: // +
                    tmp_val = builder->create_iadd(val_l, val_r);
                    break;
                case OP_MINUS: // -
                    tmp_val = builder->create_isub(val_l, val_r);
                    break;
            }
        }
        else{
            switch(node.op) {
                case OP_PLUS: // +
                    tmp_val = builder->create_fadd(val_l, val_r);
                    break;
                case OP_MINUS: // -
                    tmp_val = builder->create_fsub(val_l, val_r);
                    break;
            }
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    // term→term mulop factor ∣ factor
    if(node.term == nullptr) {
        node.factor->accept(*this);
    }
    else{
        node.factor->accept(*this); // visit the term
        auto val_l = tmp_val;
        node.term->accept(*this); // visit the factor
        auto val_r = tmp_val;

        bool isIntParam;

        // 类型转换
        if(val_l->get_type() != val_r->get_type()){
            if(val_l->get_type()->is_float_type()) {
                val_r = builder->create_sitofp(val_r, FLOAT_T);
                isIntParam = false;
            }
            else {
                val_l = builder->create_sitofp(val_l, FLOAT_T);
                isIntParam = false;
            }
        }
        else{
            isIntParam = val_l->get_type()->is_integer_type();
        }

        if(isIntParam){
            switch(node.op) {
                case OP_MUL: // *
                    tmp_val = builder->create_imul(val_r, val_l);
                    break;
                case OP_DIV: // /
                    tmp_val = builder->create_isdiv(val_r, val_l);
                    break;
            }
        }
        else{
            switch(node.op) {
                case OP_MUL: // *
                    tmp_val = builder->create_fmul(val_r, val_l);
                    break;
                case OP_DIV: // /
                    tmp_val = builder->create_fdiv(val_r, val_l);
                    break;
            }
        }
    }
}

void CminusfBuilder::visit(ASTCall &node){
    // call→ID ( args )
    auto func = static_cast<Function *>(scope.find(node.id));

    auto paramType = func->get_function_type()->param_begin();
    std::vector<Value*> args;

    for (auto arg : node.args){
        arg->accept(*this);
        if (tmp_val->get_type()->is_pointer_type()){
            args.push_back(tmp_val);
        }

        else {
            if (*paramType == FLOAT_T && tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            if (*paramType==INT32_T && tmp_val->get_type()->is_float_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            args.push_back(tmp_val);
        }
        paramType++; 
    }
    tmp_val = builder->create_call(static_cast<Function*>(func), args);
}
