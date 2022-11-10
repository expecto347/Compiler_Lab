#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) ConstantInt::get(num, module)

#define CONST_FP(num) ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
    auto module = new Module("Cminus code");  // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    // BasicBlock的名字在生成中无所谓,但是可以方便阅读
    builder->set_insert_point(bb);
    auto val1 = CONST_FP(1.11);
    auto val2 = CONST_FP(2.22);

    builder->create_fsub(val1, val2);
    builder->ret(CONST_INT(0));
  // 尽管已经有很多注释,但可能还是会遇到很多bug
  // 所以强烈建议配置AutoComplete,效率会大大提高!
  // 如果猜不到某个IR指令对应的C++的函数,建议把指令翻译成英语然后在method列表中搜索一下。
  // 最后,这个例子只涉及到了一些基本的指令生成,
  // 对于额外的指令,包括数组,在之后的实验中可能需要大家自己搜索一下思考一下,
  // 还有涉及到的C++语法,可以及时提问或者向大家提供指导哦!
  // 对于这个例子里的代码风格/用法,如果有好的建议也欢迎提出!
  std::cout << module->print();
  delete module;
  return 0;
}
