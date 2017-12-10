/***---NATIVE NAMES---***/

// This is used for natives whose names are not allowed in C code per C syntax
// For example, +, -, *, /
// The C code uses an "internal name", here we define translations to actual names
NATIVE_NAME("!=","ne");
NATIVE_NAME("*","mul");
NATIVE_NAME("+","add");
NATIVE_NAME("-","sub");
NATIVE_NAME("/","div");
NATIVE_NAME("<","lt");
NATIVE_NAME("<=","lte");
NATIVE_NAME("=","eq");
NATIVE_NAME(">","gt");
NATIVE_NAME(">=","gte");
