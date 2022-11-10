; ModuleID = 'cminus'
source_filename = "/labs/MyTest/param.cminus"

declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define i32 @main() {
label_entry:
  %op0 = alloca i32
  %op1 = alloca [100 x float]
  %op2 = icmp slt i32 1, 0
  br i1 %op2, label %label8, label %label3
label3:                                                ; preds = %label_entry
  %op4 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op5 = sitofp i32 1 to float
  store float %op5, float* %op4
  %op6 = load float, float* %op4
  %op7 = icmp slt i32 1, 0
  br i1 %op7, label %label18, label %label9
label8:                                                ; preds = %label_entry
  call void @neg_idx_except()
  ret i32 0
label9:                                                ; preds = %label3
  %op10 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op11 = load float, float* %op10
  %op12 = mul i32 2, 1
  %op13 = sitofp i32 %op12 to float
  %op14 = fmul float %op11, %op13
  %op15 = fptosi float %op14 to i32
  store i32 %op15, i32* %op0
  %op16 = load i32, i32* %op0
  %op17 = icmp slt i32 1, 0
  br i1 %op17, label %label27, label %label19
label18:                                                ; preds = %label3
  call void @neg_idx_except()
  ret i32 0
label19:                                                ; preds = %label9
  %op20 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op21 = load i32, i32* %op0
  %op22 = mul i32 %op21, 100
  %op23 = sitofp i32 %op22 to float
  store float %op23, float* %op20
  %op24 = load float, float* %op20
  %op25 = load i32, i32* %op0
  %op26 = icmp ne i32 %op25, 0
  br i1 %op26, label %label28, label %label31
label27:                                                ; preds = %label9
  call void @neg_idx_except()
  ret i32 0
label28:                                                ; preds = %label19
  %op29 = load i32, i32* %op0
  %op30 = add i32 %op29, 1
  store i32 %op30, i32* %op0
  br label %label33
label31:                                                ; preds = %label19
  %op32 = icmp slt i32 1, 0
  br i1 %op32, label %label38, label %label35
label33:                                                ; preds = %label28, %label39
  %op34 = icmp slt i32 1, 0
  br i1 %op34, label %label49, label %label45
label35:                                                ; preds = %label31
  %op36 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op37 = icmp slt i32 1, 0
  br i1 %op37, label %label44, label %label39
label38:                                                ; preds = %label31
  call void @neg_idx_except()
  ret i32 0
label39:                                                ; preds = %label35
  %op40 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op41 = load float, float* %op40
  %op42 = sitofp i32 1 to float
  %op43 = fadd float %op41, %op42
  store float %op43, float* %op36
  br label %label33
label44:                                                ; preds = %label35
  call void @neg_idx_except()
  ret i32 0
label45:                                                ; preds = %label33
  %op46 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op47 = load float, float* %op46
  %op48 = fcmp une float %op47,0x0
  br i1 %op48, label %label50, label %label52
label49:                                                ; preds = %label33
  call void @neg_idx_except()
  ret i32 0
label50:                                                ; preds = %label45
  %op51 = icmp slt i32 1, 0
  br i1 %op51, label %label59, label %label55
label52:                                                ; preds = %label45
  %op53 = icmp slt i32 1, 0
  br i1 %op53, label %label64, label %label60
label54:                                                ; preds = %label55, %label60
  ret i32 0
label55:                                                ; preds = %label50
  %op56 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op57 = load float, float* %op56
  %op58 = fptosi float %op57 to i32
  ret i32 %op58
  br label %label54
label59:                                                ; preds = %label50
  call void @neg_idx_except()
  ret i32 0
label60:                                                ; preds = %label52
  %op61 = getelementptr [100 x float], [100 x float]* %op1, i32 0, i32 1
  %op62 = load float, float* %op61
  %op63 = fptosi float %op62 to i32
  ret i32 %op63
  br label %label54
label64:                                                ; preds = %label52
  call void @neg_idx_except()
  ret i32 0
}
