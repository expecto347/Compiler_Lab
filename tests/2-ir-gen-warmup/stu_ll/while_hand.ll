; ModuleID = 'while.c'
source_filename = "while.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
; main函数入口
define dso_local i32 @main() #0 {
  %1 = alloca i32                 ;为a分配32字节的空间
  %2 = alloca i32                 ;为i分配32字节的空间
  store i32 10, i32* %1           ;a = 10
  store i32 0, i32* %2            ;i = 0
  br label %3                     ;跳转到loop
3:
  %4 = load i32, i32* %1          ;将a的值保存到%4中
  %5 = load i32, i32* %2          ;将i的值保存到寄存器%5中
  %6 = icmp slt i32 %5, 10        ;判断i < 10
  br i1 %6, label %7, label %10
7:                                ;TRUE
  %8 = add nsw i32 %5, 1          ;i=i+1
  %9 = add nsw i32 %4, %8         ;a=a+i
  store i32 %8, i32* %2
  store i32 %9, i32* %1
  br label %3                     ;继续循环
10:                               ;FALSE
  ret i32 %4                      ;为假时返回a的值
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}