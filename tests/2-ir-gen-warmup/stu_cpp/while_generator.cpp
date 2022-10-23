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

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),  "main", module); //创建main函数
  auto bb = BasicBlock::create(module, "entry", mainFun);
  // BasicBlock的名字在生成中无所谓,但是可以方便阅读
  builder->set_insert_point(bb);  // 一个BB的开始,将当前插入指令点的位置设在bb

  auto aAlloca = builder->create_alloca(Int32Type); //分配一个int32类型的空间给a
  auto iAlloca = builder->create_alloca(Int32Type); //分配一个int32类型的空间给i
  builder->create_store(CONST_INT(10), aAlloca);  //将10存入a
  builder->create_store(CONST_INT(0), iAlloca);   //将0存入i

  auto loopBB = BasicBlock::create(module, "loopBB", mainFun); //创建循环的BB
  builder->create_br(loopBB);         //跳转到循环的BB
  builder->set_insert_point(loopBB);  //一个BB的开始,将当前插入指令点的位置设在loopBB

  auto aLoad = builder->create_load(aAlloca);
  auto iLoad = builder->create_load(iAlloca); //将a和i的值取出来
  auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10)); //比较i和10的大小

  auto trueBB = BasicBlock::create(module, "trueBB", mainFun);
  auto falseBB = BasicBlock::create(module, "falseBB", mainFun);
  builder->create_cond_br(icmp, trueBB, falseBB);  //根据比较结果跳转到trueBB或falseBB
  builder->set_insert_point(trueBB);  //一个BB的开始,将当前插入指令点的位置设在trueBB

  auto i_Load = builder->create_iadd(iLoad, CONST_INT(1));  // i = i + 1
  auto a_Load = builder->create_iadd(aLoad, i_Load);        // a = a + i
  builder->create_store(a_Load, aAlloca);
  builder->create_store(i_Load, iAlloca);
  builder->create_br(loopBB);          //跳转到循环的BB
  builder->set_insert_point(falseBB);  //一个BB的开始,将当前插入指令点的位置设在falseBB
  builder->create_ret(aLoad);          //返回a的值
  std::cout << module->print();
  delete module;
  return 0;
}