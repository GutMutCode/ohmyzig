//! Entry point for the Zig + C (Win32) sample.
//! - Demonstrates calling C functions from Zig using @cImport
//! - Shows a simple Win32 window via a C helper implementation
const std = @import("std");

// Import C headers so their functions are available as `c.*` in Zig.
// c_functions.h: simple arithmetic example (uses libc printf)
// win32_ui.h   : small Win32 wrapper that creates and shows a window
const c = @cImport({
    @cInclude("c_functions.h");
    @cInclude("win32_ui.h");
});

pub fn main() anyerror!void {
    // If you need to debug via console output, consider temporarily
    // switching the subsystem to .Console in build.zig.
    // const print = std.debug.print;

    // Example: call a C function from Zig.
    // const num1: c_int = 10;
    // const num2: c_int = 5;
    // const result_from_c = c.add_in_c(num1, num2);
    // print("[Zig] Result from C: {d}\n", .{result_from_c});

    // Create a simple Win32 window via C and run the message loop.
    _ = c.CreateSimpleWindow();
}

