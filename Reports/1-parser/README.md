# lab1 实验报告
PB20000277 孙昊哲

## 实验要求（此部分来自实验文档）

### 3.1 目录结构

与本次实验有关的文件如下。

```
.
├── CMakeLists.txt
├── Documentations
│   └── 1-parser                       本次实验的所有文档
├── Reports
│   └── 1-parser
│       └── README.md                  在这里写实验报告 <-- 修改
├── include
│   └── syntax_tree.h
├── shell.nix
├── src
│   ├── CMakeLists.txt
│   ├── common
│   │   ├── CMakeLists.txt
│   │   └── syntax_tree.c              语法树的构造
│   └── parser
│       ├── CMakeLists.txt
│       ├── lexical_analyzer.l         词法分析器       <-- 修改
│       └── syntax_analyzer.y          语法分析器       <-- 修改
└── tests
    ├── CMakeLists.txt
    └── parser
    │   ├── CMakeLists.txt
    │   ├── easy                       简单的测试
    │   ├── normal                     中等复杂度的测试
    │   ├── hard                       复杂的测试
    │   ├── lexer.c                    用于词法分析的调试程序
    │   ├── parser.c                   语法分析
    │   ├── syntree_*_std              标准程序输出结果
    │   └── test_syntax.sh             用于进行批量测试的脚本
```

### 3.2 编译、运行和验证

项目构建使用 `cmake` 进行。

* 编译

  ```sh
  $ cd 2022fall-Compiler_CMinus
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  ```

  如果构建成功，会在该目录下看到 `lexer` 和 `parser` 两个可执行文件。
  * `lexer`用于词法分析，产生token stream；对于词法分析结果，我们不做考察
  * `parser`用于语法分析，产生语法树。

* 运行

  ```sh
  $ cd 2022fall-Compiler_CMinus
  # 词法测试
  $ ./build/lexer ./tests/parser/normal/local-decl.cminus
  Token	      Text	Line	Column (Start,End)
  280  	       int	0	(0,3)
  284  	      main	0	(4,8)
  272  	         (	0	(8,9)
  282  	      void	0	(9,13)
  273  	         )	0	(13,14)
  ...
  # 语法测试
  $ ./build/parser ./tests/parser/normal/local-decl.cminus
  >--+ program
  |  >--+ declaration-list
  |  |  >--+ declaration
  ...
  ```

* 验证
  可以使用 `diff` 与标准输出进行比较。

  ```sh
  $ cd 2022fall-Compiler_CMinus
  $ export PATH="$(realpath ./build):$PATH"
  $ cd tests/parser
  $ mkdir output.easy
  $ parser easy/expr.cminus > output.easy/expr.cminus
  $ diff output.easy/expr.cminus syntree_easy_std/expr.syntax_tree
  [输出为空，代表没有区别，该测试通过]
  ```

  我们提供了 `test_syntax.sh` 脚本进行快速批量测试。该脚本的第一个参数是 `easy` `normal` `hard` 等难度，并且有第二个可选参数，用于进行批量 `diff` 比较。脚本运行后会产生名如 `syntree_easy` 的文件夹。

  ```sh
  $ ./test_syntax.sh easy
  [info] Analyzing FAIL_id.cminus
  error at line 1 column 6: syntax error
  ...
  [info] Analyzing id.cminus
  
  $ ./test_syntax.sh easy yes
  ...
  [info] Comparing...
  [info] No difference! Congratulations!
  ```

### 3.3 提交要求和评分标准

* 提交要求
  本实验的提交要求分为两部分：实验部分的文件和报告，git提交的规范性。

  * 实验部分
    * 需要完善 `src/parser/lexical_analyzer.l` 和 `src/parser/syntax_analyzer.y`。
    * 需要在 `Reports/1-parser/README.md` 中撰写实验报告。
      * 实验报告内容包括
        * 实验要求、实验难点、实验设计、实验结果验证、实验反馈
 
  * git 提交规范
    * 不破坏目录结构（实验报告所需图片放在目录中）
    * 不上传临时文件（凡是可以自动生成的都不要上传，如 `build` 目录、测试时自动生成的输出、`.DS_Store` 等）
    * git log 言之有物

* 提交方式：
  
  * 代码提交：本次实验需要在希冀课程平台上发布的作业[Lab1-代码提交](http://cscourse.ustc.edu.cn/assignment/index.jsp?courseID=17&assignID=54)提交自己仓库的 gitlab 链接（注：由于平台限制，请提交http协议格式的仓库链接。例：学号为 PB011001 的同学，Lab1 的实验仓库地址为`http://202.38.79.174/PB011001/2022fall-compiler_cminus.git`），我们会收集最后一次提交的评测分数，作为最终代码得分。
  
  * 实验评测
    * 除已提供的 easy, normal, hard 数据集之外，平台会使用额外的隐藏测试用例进行测试。  

  * 报告提交：将 `Reports/1-parser/README.md` 导出成 pdf 文件单独提交到[Lab1-报告提交](http://cscourse.ustc.edu.cn/assignment/index.jsp?courseID=17&assignID=54)。
  
  * 提交异常：如果遇到在平台上提交异常的问题，请通过邮件联系助教，助教将收取截止日期之前，学生在 gitlab 仓库最近一次 commit 内容进行评测。

* 迟交规定

  * Soft Deadline：2022-09-30 23:59:59 (UTC+8)

  * Hard Deadline：2022-10-07 23:59:59 (UTC+8)

  * 补交请邮件提醒 TA：

    * 邮箱：`zhenghy22@mail.ustc.edu.cn` 抄送 `chen16614@mail.ustc.edu.cn`
    * 邮件主题：lab1迟交-学号
    * 内容：迟交原因、最后版本commitID、迟交时间

  * 迟交分数

    * x 为相对 Soft Deadline 迟交天数，grade 满分 100

      ```
      final_grade = grade, x = 0  
      final_grade = grade * (0.9)^x, 0 < x <= 7  
      final_grade = 0, x > 7
      ```

* 评分标准：
  实验一最终分数组成如下：
  * 平台测试得分：(70分)
  * 实验报告得分：(30分)  
  注：禁止执行恶意代码，违者0分处理。  
  
* 关于抄袭和雷同

  经过助教和老师判定属于作业抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳。

## 实验难点

1. 编写完善Flex，匹配所需要的tokens
2. 完成语法分析规则
3. 仔细阅读助教，老师提供的实验文档，读懂想干什么
4. 正确处理特殊情况

## 实验设计
1. 完成文件`src/parser/lexical_analyzer.l`，需要注意对于COMMENT的处理，应该正确维护行数和列数；
2. 完成文件`src/parser/syntax_analyzer.y`，填写与实验文档相对应的语法规则，写出语法分析树的生成规则；正确维护TOKEN
## 实验结果验证

1. 助教提供的测试已经全部通过
2. 自己的测试文件`tests/parser/test/test.cminus`，也可以通过

## 实验反馈

建议不要在国庆长假中途设置DDL，不然真的会忘
