; ModuleID = 'cminus'
source_filename = "/labs/MyTest/test.cminus"

declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define i32 @main() {
label_entry:
  %op0 = alloca [100 x i32]
  %op1 = alloca i32
  %op2 = load i32, i32* %op1
  %op3 = icmp slt i32 %op2, 0
  br i1 %op3, label %label6, label %label4
label4:                                                ; preds = %label_entry
  %op5 = getelementptr [100 x i32], [100 x i32]* %op0, i32 0, i32 %op2
  store i32 1, i32* %op5
  ret i32 0
label6:                                                ; preds = %label_entry
  call void @neg_idx_except()
  ret i32 0
}
