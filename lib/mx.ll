; ModuleID = 'mx.c'
source_filename = "mx.c"
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "riscv32-unknown-unknown-elf"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__String_length__(i8* noundef %0) #0 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  %3 = load i8*, i8** %2, align 4
  %4 = call i32 @strlen(i8* noundef %3)
  ret i32 %4
}

declare dso_local i32 @strlen(i8* noundef) #1

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__String_substring__(i8* noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i8*, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i8*, align 4
  store i8* %0, i8** %4, align 4
  store i32 %1, i32* %5, align 4
  store i32 %2, i32* %6, align 4
  %9 = load i32, i32* %6, align 4
  %10 = load i32, i32* %5, align 4
  %11 = sub nsw i32 %9, %10
  store i32 %11, i32* %7, align 4
  %12 = load i32, i32* %7, align 4
  %13 = add nsw i32 %12, 1
  %14 = call i8* @malloc(i32 noundef %13)
  store i8* %14, i8** %8, align 4
  %15 = load i8*, i8** %8, align 4
  %16 = load i8*, i8** %4, align 4
  %17 = load i32, i32* %5, align 4
  %18 = getelementptr inbounds i8, i8* %16, i32 %17
  %19 = load i32, i32* %7, align 4
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 1 %15, i8* align 1 %18, i32 %19, i1 false)
  %20 = load i8*, i8** %8, align 4
  %21 = load i32, i32* %7, align 4
  %22 = getelementptr inbounds i8, i8* %20, i32 %21
  store i8 0, i8* %22, align 1
  %23 = load i8*, i8** %8, align 4
  ret i8* %23
}

declare dso_local i8* @malloc(i32 noundef) #1

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i32(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i32, i1 immarg) #2

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__String_parseInt__(i8* noundef %0) #0 {
  %2 = alloca i8*, align 4
  %3 = alloca i32, align 4
  store i8* %0, i8** %2, align 4
  %4 = load i8*, i8** %2, align 4
  %5 = call i32 (i8*, i8*, ...) @sscanf(i8* noundef %4, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32* noundef %3)
  %6 = load i32, i32* %3, align 4
  ret i32 %6
}

declare dso_local i32 @sscanf(i8* noundef, i8* noundef, ...) #1

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__String_ord__(i8* noundef %0, i32 noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i32, align 4
  store i8* %0, i8** %3, align 4
  store i32 %1, i32* %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i32, i32* %4, align 4
  %7 = getelementptr inbounds i8, i8* %5, i32 %6
  %8 = load i8, i8* %7, align 1
  %9 = zext i8 %8 to i32
  ret i32 %9
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__print__(i8* noundef %0) #0 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  %3 = load i8*, i8** %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), i8* noundef %3)
  ret void
}

declare dso_local i32 @printf(i8* noundef, ...) #1

; Function Attrs: noinline nounwind optnone
define dso_local void @__println__(i8* noundef %0) #0 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  %3 = load i8*, i8** %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i32 0, i32 0), i8* noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__printInt__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local void @__printlnInt__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([4 x i8], [4 x i8]* @.str.3, i32 0, i32 0), i32 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__getString__() #0 {
  %1 = alloca i8*, align 4
  %2 = call i8* @malloc(i32 noundef 4096)
  store i8* %2, i8** %1, align 4
  %3 = load i8*, i8** %1, align 4
  %4 = call i32 (i8*, ...) @scanf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), i8* noundef %3)
  %5 = load i8*, i8** %1, align 4
  ret i8* %5
}

declare dso_local i32 @scanf(i8* noundef, ...) #1

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__getInt__() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 (i8*, ...) @scanf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32* noundef %1)
  %3 = load i32, i32* %1, align 4
  ret i32 %3
}

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__toString__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 4
  store i32 %0, i32* %2, align 4
  %4 = call i8* @malloc(i32 noundef 16)
  store i8* %4, i8** %3, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i32, i32* %2, align 4
  %7 = call i32 (i8*, i8*, ...) @sprintf(i8* noundef %5, i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32 noundef %6)
  %8 = load i8*, i8** %3, align 4
  ret i8* %8
}

declare dso_local i32 @sprintf(i8* noundef, i8* noundef, ...) #1

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__string_add__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %8 = load i8*, i8** %3, align 4
  %9 = call i32 @strlen(i8* noundef %8)
  store i32 %9, i32* %5, align 4
  %10 = load i8*, i8** %4, align 4
  %11 = call i32 @strlen(i8* noundef %10)
  store i32 %11, i32* %6, align 4
  %12 = load i32, i32* %5, align 4
  %13 = load i32, i32* %6, align 4
  %14 = add nsw i32 %12, %13
  %15 = add nsw i32 %14, 1
  %16 = call i8* @malloc(i32 noundef %15)
  store i8* %16, i8** %7, align 4
  %17 = load i8*, i8** %7, align 4
  %18 = load i8*, i8** %3, align 4
  %19 = load i32, i32* %5, align 4
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 1 %17, i8* align 1 %18, i32 %19, i1 false)
  %20 = load i8*, i8** %7, align 4
  %21 = load i32, i32* %5, align 4
  %22 = getelementptr inbounds i8, i8* %20, i32 %21
  %23 = load i8*, i8** %4, align 4
  %24 = load i32, i32* %6, align 4
  %25 = add nsw i32 %24, 1
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* align 1 %22, i8* align 1 %23, i32 %25, i1 false)
  %26 = load i8*, i8** %7, align 4
  ret i8* %26
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_eq__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp eq i32 %7, 0
  ret i1 %8
}

declare dso_local i32 @strcmp(i8* noundef, i8* noundef) #1

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_ne__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp ne i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_gt__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp sgt i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_ge__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp sge i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_lt__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp slt i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local zeroext i1 @__string_le__(i8* noundef %0, i8* noundef %1) #0 {
  %3 = alloca i8*, align 4
  %4 = alloca i8*, align 4
  store i8* %0, i8** %3, align 4
  store i8* %1, i8** %4, align 4
  %5 = load i8*, i8** %3, align 4
  %6 = load i8*, i8** %4, align 4
  %7 = call i32 @strcmp(i8* noundef %5, i8* noundef %6)
  %8 = icmp sle i32 %7, 0
  ret i1 %8
}

; Function Attrs: noinline nounwind optnone
define dso_local i32 @__Array_size(i8* noundef %0) #0 {
  %2 = alloca i8*, align 4
  store i8* %0, i8** %2, align 4
  %3 = load i8*, i8** %2, align 4
  %4 = bitcast i8* %3 to i32*
  %5 = getelementptr inbounds i32, i32* %4, i32 -1
  %6 = load i32, i32* %5, align 4
  ret i32 %6
}

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__new_array4__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32*, align 4
  store i32 %0, i32* %2, align 4
  %4 = load i32, i32* %2, align 4
  %5 = add nsw i32 %4, 1
  %6 = shl i32 %5, 2
  %7 = call i8* @malloc(i32 noundef %6)
  %8 = bitcast i8* %7 to i32*
  store i32* %8, i32** %3, align 4
  %9 = load i32, i32* %2, align 4
  %10 = load i32*, i32** %3, align 4
  store i32 %9, i32* %10, align 4
  %11 = load i32*, i32** %3, align 4
  %12 = getelementptr inbounds i32, i32* %11, i32 1
  %13 = bitcast i32* %12 to i8*
  ret i8* %13
}

; Function Attrs: noinline nounwind optnone
define dso_local i8* @__new_array1__(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32*, align 4
  store i32 %0, i32* %2, align 4
  %4 = load i32, i32* %2, align 4
  %5 = add nsw i32 %4, 4
  %6 = call i8* @malloc(i32 noundef %5)
  %7 = bitcast i8* %6 to i32*
  store i32* %7, i32** %3, align 4
  %8 = load i32, i32* %2, align 4
  %9 = load i32*, i32** %3, align 4
  store i32 %8, i32* %9, align 4
  %10 = load i32*, i32** %3, align 4
  %11 = getelementptr inbounds i32, i32* %10, i32 1
  %12 = bitcast i32* %11 to i8*
  ret i8* %12
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+a,+c,+m" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+a,+c,+m" }
attributes #2 = { argmemonly nofree nounwind willreturn }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"ilp32"}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{i32 1, !"SmallDataLimit", i32 8}
!4 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
