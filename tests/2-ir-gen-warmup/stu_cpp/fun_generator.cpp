#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) ConstantInt::get(num, module)

#define CONST_FP(num) ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
  auto module = new Module("Cminus code");  // module name是什么无关紧要
  auto builder = new IRBuilder(nullptr, module);
  
  Type *Int32Type = Type::get_int32_type(module); // 定义int32类型
  std::vector<Type *> Ints(1, Int32Type); // 定义一个int32类型的数组
  auto calleeFun = Function::create(FunctionType::get(Int32Type, Ints), "callee", module); // 创建callee函数，并且定义为Int32类型

  auto bb = BasicBlock::create(module, "entry", calleeFun); // BasicBlock的名字在生成中无所谓,但是可以方便阅读

  builder->set_insert_point(bb);        // 一个BB的开始,将当前插入指令点的位置设在bb
  auto aAlloca = builder->create_alloca(Int32Type); // 在BB的开始,创建一个alloca指令,用于分配变量a
  std::vector<Value *> args;  // 用于存放函数参数
  for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++) {
    args.push_back(*arg);   // 将函数参数放入args中
  }

  builder->create_store(args[0], aAlloca); 
  auto aLoad = builder->create_load(aAlloca);
  auto mul = builder->create_imul(aLoad, CONST_INT(2));
  builder->create_ret(mul);

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  bb = BasicBlock::create(module, "entry", mainFun);
  // BasicBlock的名字在生成中无所谓,但是可以方便阅读
  builder->set_insert_point(bb); 
  auto call = builder->create_call(calleeFun, {CONST_INT(110)}); // 调用callee函数

  builder->create_ret(call); // 返回callee函数的返回值
  std::cout << module->print(); // 打印module
  delete module;
  return 0;
}