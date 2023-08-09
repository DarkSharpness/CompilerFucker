; ModuleID = 'mx.c'
source_filename = "mx.c"
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "riscv32-unknown-unknown-elf"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__String_substring__(ptr noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca ptr, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca ptr, align 4
  store ptr %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  %9 = load i32, ptr %6, align 4
  %10 = load i32, ptr %5, align 4
  %11 = sub nsw i32 %9, %10
  store i32 %11, ptr %7, align 4
  %12 = load i32, ptr %7, align 4
  %13 = add nsw i32 %12, 1
  %14 = call ptr @malloc(i32 noundef %13) #4
  store ptr %14, ptr %8, align 4
  %15 = load ptr, ptr %8, align 4
  %16 = load ptr, ptr %4, align 4
  %17 = load i32, ptr %5, align 4
  %18 = getelementptr inbounds i8, ptr %16, i32 %17
  %19 = load i32, ptr %7, align 4
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %15, ptr align 1 %18, i32 %19, i1 false)
  %20 = load ptr, ptr %8, align 4
  %21 = load i32, ptr %7, align 4
  %22 = getelementptr inbounds i8, ptr %20, i32 %21
  store i8 0, ptr %22, align 1
  %23 = load ptr, ptr %8, align 4
  ret ptr %23
}

; Function Attrs: allocsize(0)
declare dso_local ptr @malloc(i32 noundef) #1

; Function Attrs: argmemonly nocallback nofree nounwind willreturn
declare void @llvm.memcpy.p0.p0.i32(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i32, i1 immarg) #2

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__String_parseInt__(ptr noundef %0) #0 {
  %2 = alloca ptr, align 4
  %3 = alloca i32, align 4
  store ptr %0, ptr %2, align 4
  %4 = load ptr, ptr %2, align 4
  %5 = call i32 (ptr, ptr, ...) @sscanf(ptr noundef %4, ptr noundef @.str, ptr noundef %3)
  %6 = load i32, ptr %3, align 4
  ret i32 %6
}

declare dso_local i32 @sscanf(ptr noundef, ptr noundef, ...) #3

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__String_ord__(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load i32, ptr %4, align 4
  %7 = getelementptr inbounds i8, ptr %5, i32 %6
  %8 = load i8, ptr %7, align 1
  %9 = zext i8 %8 to i32
  ret i32 %9
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__print__(ptr noundef %0) #0 {
  %2 = alloca ptr, align 4
  store ptr %0, ptr %2, align 4
  %3 = load ptr, ptr %2, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.1, ptr noundef %3)
  ret void
}

declare dso_local i32 @printf(ptr noundef, ...) #3

; Function Attrs: noinline nounwind optnone
define dso_local void @__println__(ptr noundef %0) #0 {
  %2 = alloca ptr, align 4
  store ptr %0, ptr %2, align 4
  %3 = load ptr, ptr %2, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.2, ptr noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__printInt__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__printlnInt__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, i32 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__getString__() #0 {
  %1 = alloca ptr, align 4
  %2 = call ptr @malloc(i32 noundef 4096) #4
  store ptr %2, ptr %1, align 4
  %3 = load ptr, ptr %1, align 4
  %4 = call i32 (ptr, ...) @scanf(ptr noundef @.str.1, ptr noundef %3)
  %5 = load ptr, ptr %1, align 4
  ret ptr %5
}

declare dso_local i32 @scanf(ptr noundef, ...) #3

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__getInt__() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 (ptr, ...) @scanf(ptr noundef @.str, ptr noundef %1)
  %3 = load i32, ptr %1, align 4
  ret i32 %3
}

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__toString__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca ptr, align 4
  store i32 %0, ptr %2, align 4
  %4 = call ptr @malloc(i32 noundef 16) #4
  store ptr %4, ptr %3, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load i32, ptr %2, align 4
  %7 = call i32 (ptr, ptr, ...) @sprintf(ptr noundef %5, ptr noundef @.str, i32 noundef %6)
  %8 = load ptr, ptr %3, align 4
  ret ptr %8
}

declare dso_local i32 @sprintf(ptr noundef, ptr noundef, ...) #3

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__string_add__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %8 = load ptr, ptr %3, align 4
  %9 = call i32 @strlen(ptr noundef %8)
  store i32 %9, ptr %5, align 4
  %10 = load ptr, ptr %4, align 4
  %11 = call i32 @strlen(ptr noundef %10)
  store i32 %11, ptr %6, align 4
  %12 = load i32, ptr %5, align 4
  %13 = load i32, ptr %6, align 4
  %14 = add nsw i32 %12, %13
  %15 = add nsw i32 %14, 1
  %16 = call ptr @malloc(i32 noundef %15) #4
  store ptr %16, ptr %7, align 4
  %17 = load ptr, ptr %7, align 4
  %18 = load ptr, ptr %3, align 4
  %19 = load i32, ptr %5, align 4
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %17, ptr align 1 %18, i32 %19, i1 false)
  %20 = load ptr, ptr %7, align 4
  %21 = load i32, ptr %5, align 4
  %22 = getelementptr inbounds i8, ptr %20, i32 %21
  %23 = load ptr, ptr %4, align 4
  %24 = load i32, ptr %6, align 4
  %25 = add nsw i32 %24, 1
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %22, ptr align 1 %23, i32 %25, i1 false)
  %26 = load ptr, ptr %7, align 4
  ret ptr %26
}

declare dso_local i32 @strlen(ptr noundef) #3

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_eq__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp eq i32 %7, 0
  ret i1 %8
}

declare dso_local i32 @strcmp(ptr noundef, ptr noundef) #3

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_ne__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp ne i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_gt__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp sgt i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_ge__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp sge i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_lt__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp slt i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_le__(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca ptr, align 4
  store ptr %0, ptr %3, align 4
  store ptr %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 4
  %6 = load ptr, ptr %4, align 4
  %7 = call i32 @strcmp(ptr noundef %5, ptr noundef %6)
  %8 = icmp sle i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__Array_size(ptr noundef %0) #0 {
  %2 = alloca ptr, align 4
  store ptr %0, ptr %2, align 4
  %3 = load ptr, ptr %2, align 4
  %4 = getelementptr inbounds i32, ptr %3, i32 -1
  %5 = load i32, ptr %4, align 4
  ret i32 %5
}

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__new_array4__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca ptr, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = add nsw i32 %4, 1
  %6 = shl i32 %5, 2
  %7 = call ptr @malloc(i32 noundef %6) #4
  store ptr %7, ptr %3, align 4
  %8 = load i32, ptr %2, align 4
  %9 = load ptr, ptr %3, align 4
  store i32 %8, ptr %9, align 4
  %10 = load ptr, ptr %3, align 4
  %11 = getelementptr inbounds i32, ptr %10, i32 1
  ret ptr %11
}

; Function Attrs: noinline nounwind optnone
define dso_local ptr @__new_array1__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca ptr, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = add nsw i32 %4, 4
  %6 = call ptr @malloc(i32 noundef %5) #4
  store ptr %6, ptr %3, align 4
  %7 = load i32, ptr %2, align 4
  %8 = load ptr, ptr %3, align 4
  store i32 %7, ptr %8, align 4
  %9 = load ptr, ptr %3, align 4
  %10 = getelementptr inbounds i32, ptr %9, i32 1
  ret ptr %10
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+a,+c,+m,+relax,-save-restore" }
attributes #1 = { allocsize(0) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+a,+c,+m,+relax,-save-restore" }
attributes #2 = { argmemonly nocallback nofree nounwind willreturn }
attributes #3 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+a,+c,+m,+relax,-save-restore" }
attributes #4 = { allocsize(0) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"ilp32"}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{i32 1, !"SmallDataLimit", i32 8}
!4 = !{!"Ubuntu clang version 15.0.7"}
