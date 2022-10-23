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
  
  Type *Int32Type = Type::get_int32_type(module);  //定义int32类型
  Type *FloatType = Type::get_float_type(module);  //定义float类型

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),  //创建main函数
                                  "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  // BasicBlock的名字在生成中无所谓,但是可以方便阅读
  builder->set_insert_point(bb);  // 一个BB的开始,将当前插入指令点的位置设在bb

  auto aAlloca = builder->create_alloca(FloatType); // 申请一个float类型的变量a
  builder->create_store(CONST_FP(5.555), aAlloca);  // 将5.555存入a
  auto aLoad = builder->create_load(aAlloca); // 将a的值读出来  
  auto fcmp = builder->create_fcmp_gt(aLoad, CONST_FP(1.0)); // 判断a是否大于1.0
  auto trueBB = BasicBlock::create(module, "trueBB", mainFun); // 创建trueBB
  auto falseBB = BasicBlock::create(module, "falseBB", mainFun); // 创建falseBB
  builder->create_cond_br(fcmp, trueBB, falseBB); // 判断fcmp的值,如果为真则跳转到trueBB,否则跳转到falseBB
  builder->set_insert_point(trueBB);    // 一个BB的开始,将当前插入指令点的位置设在trueBB
  builder->create_ret(CONST_INT(233));  // return 233

  builder->set_insert_point(falseBB);   // 一个BB的开始,将当前插入指令点的位置设在falseBB
  builder->create_ret(CONST_INT(0));    // return 0
  
  std::cout << module->print();
  delete module;
  return 0;
}