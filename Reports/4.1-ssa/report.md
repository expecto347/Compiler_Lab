# Lab4.1 实验报告

## 实验要求

阅读实验文档，理解优化的基本流程，并回答思考题

## 思考题
### Mem2reg
1. 请简述概念：支配性、严格支配性、直接支配性、支配边界。

   * 支配性：如果每一条从流图的入口结点到结点 n 的路径都经过结点 d, 我们就说 d 支配（dominate）n，记为 d dom n。请注意，在这个定义下每个结点都支配它自己。

   * 严格支配性：如果 d != n 且 d dom n，那么 d sdom n。

   * 直接支配性：从入口处节点到达节点n的所有路径上，结点n的最后一个支配节点 称为 直接支配节点。

   * 支配边界：将相对于 n 具有如下性质的结点 m 的集合称为 n 的支配边界，记为 `DF(n)`：

     （1）n 支配 m 的一个前驱；

     （2）n 并不严格支配 m。

     非正式的，`DF(n)` 包含：在离开 n 的每条 CFG 路径上，从结点 n 可达但不支配的第一个结点。

2. `phi`节点是SSA的关键特征，请简述`phi`节点的概念，以及引入`phi`节点的理由。

   * 概念：`phi`节点根据当前块的前导选择值的指令，决定后续使用的正确选择分支中的值

   * `phi`节点会根据程序运行的路径来选择不同的新变量

   * 例如来自一个维基百科的例子：

     * y在底层区块的使用，可以被指定为y1亦或是y2，这得根据它流程的来源来决定，所以我们该如何知道要使用哪一个？

       答案是，我们增加一个特别的描述，称之为*Φ （Phi）函数*，作为最后一个区块的起始，这个描述将会产生一个新的定义y3，会根据程序运作的路径来选择y1或y2。

       ![](https://raw.githubusercontent.com/expecto347/Img/main/202211270317422.png)

3. 观察下面给出的`cminus`程序对应的 LLVM IR，与开启`Mem2Reg`生成的LLVM IR对比，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

   * 对于传参的变量来说，去掉了对于传入参数的`load`和`store`过程，直接从传入的寄存器中取用函数参数
   * 去掉了对于函数中返回函数的`load`和`store`过程，直接从返回寄存器中取用寄存器中的值
   * 在`func`函数中的`if`语句，使用了`phi`函数来判断到底是哪一个分支，在分支中作出选择

4. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。（需要给出代码中的成员变量或成员函数名称）
   ```C++
   void Mem2Reg::generate_phi() {
       // global_live_var_name 是全局名字集合，以 alloca 出的局部变量来统计。
       // 步骤一：找到活跃在多个 block 的全局名字集合，以及它们所属的 bb 块
       std::set<Value *> global_live_var_name;
       std::map<Value *, std::set<BasicBlock *>> live_var_2blocks;
       for (auto &bb1 : func_->get_basic_blocks()) {
           auto bb = &bb1;
           std::set<Value *> var_is_killed;
           for (auto &instr1 : bb->get_instructions()) {
               auto instr = &instr1;
               if (instr->is_store()) {
                   // store i32 a, i32 *b
                   // a is r_val, b is l_val
                   auto r_val = static_cast<StoreInst *>(instr)->get_rval();
                   auto l_val = static_cast<StoreInst *>(instr)->get_lval();
   
                   if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val)) {
                       global_live_var_name.insert(l_val);
                       live_var_2blocks[l_val].insert(bb);
                   }
               }
           }
       }
   
       // 步骤二：从支配树获取支配边界信息，并在对应位置插入 phi 指令
       std::map<std::pair<BasicBlock *, Value *>, bool> bb_has_var_phi; // bb has phi for var
       for (auto var : global_live_var_name) {
           std::vector<BasicBlock *> work_list;
           work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
           for (int i = 0; i < work_list.size(); i++) {
               auto bb = work_list[i];
               for (auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb)) {
                   if (bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end()) {
                       // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                       auto phi =
                           PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                       phi->set_lval(var);
                       bb_dominance_frontier_bb->add_instr_begin(phi);
                       work_list.push_back(bb_dominance_frontier_bb);
                       bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                   }
               }
           }
       }
   }
   ```

   * 对于存储在`global_live_var_name`中的每一个跨block的全局变量，利用函数`get_dominance_frontier`找到他的支配边界
   * 使用`if(bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end())`判断每一个属于该支配边界的节点
   * 最后使用`add_instr_begin`插入该全局变量所需要的`phi`函数

5. 算法是如何选择`value`(变量最新的值)来替换`load`指令的？（描述清楚对应变量与维护该变量的位置）

   * 首先维护一个名称为`std::map<Value *, std::vector<Value *>> var_val_stack`的map栈，在栈中存储多个程序块的活动变量名以及同一个活动变量名在不同路径上的重命名变量的集合：`vector<Value *>`
   * 接着对于每一个`BlockBasic`块首先执行语句块中的`Phi`函数，将得到的`Value`值存入`var_val_stack[l_val]`末尾
   * 最后对于`bb`中的`load`指令，判断`get_lval()`的值是否出现在了寄存器当中，如果是，那么就用`value`值来替代`load`指令，该`load`指令冗余

### 代码阅读总结

本次实验阅读任务中，了解了`SSA`生成算法的基本过程，通过标准的`SSA`构造算法来将原始IR转换成`minimal-SSA`并最终转换成`prune-SSA`

实现了对代码中多余的`load`和`store`指令的删除和优化

深入了对于代码优化的理解，为下一阶段的任务打下良好的基础

### 实验反馈 （可选 不会评分）

无