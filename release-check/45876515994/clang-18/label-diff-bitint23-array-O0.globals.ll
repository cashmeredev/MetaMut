@main.narrow = internal global [2 x i23] [i23 trunc (i64 sub (i64 ptrtoint (ptr blockaddress(@main, %4) to i64), i64 ptrtoint (ptr blockaddress(@main, %2) to i64)) to i23), i23 1], align 4
@main.reference = internal global [2 x i32] [i32 trunc (i64 sub (i64 ptrtoint (ptr blockaddress(@main, %4) to i64), i64 ptrtoint (ptr blockaddress(@main, %2) to i64)) to i32), i32 1], align 4
@.str = private unnamed_addr constant [19 x i8] c"between the labels\00", align 1
@.str.1 = private unnamed_addr constant [24 x i8] c"bitint=%d,%d int=%d,%d\0A\00", align 1
