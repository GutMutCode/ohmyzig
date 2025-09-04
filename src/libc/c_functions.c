// Simple C function implementation used from Zig.
// Demonstrates linking libc and calling into C from Zig code.
#include "c_functions.h"
#include <stdio.h>

int add_in_c(int a, int b) {
    // Note: In a GUI subsystem app, stdout is typically not visible.
    // This printf is mainly useful when running under a debugger
    // or if the subsystem is switched to Console.
    printf("[C] Adding %d and %d\n", a, b);
    return a + b;
}

