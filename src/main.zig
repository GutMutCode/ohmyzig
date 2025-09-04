//! Entry point for the Zig + C (Win32) sample.
//! - Demonstrates calling C functions from Zig using @cImport
//! - Shows a simple Win32 window via a C helper implementation
const std = @import("std");
const chatgpt = @import("chatgpt.zig");

// Import C headers so their functions are available as `c.*` in Zig.
// c_functions.h: simple arithmetic example (uses libc printf)
// win32_ui.h   : small Win32 wrapper that creates and shows a window
const c = @cImport({
    @cInclude("c_functions.h");
    @cInclude("win32_ui.h");
});

pub fn main() anyerror!void {
    const allocator = std.heap.page_allocator;
    const stdout = std.io.getStdOut().writer();

    const api_key = std.process.getEnvVarOwned(allocator, "OPENAI_API_KEY") catch {
        try stdout.print("OPENAI_API_KEY not set\n", .{});
        return;
    };
    defer allocator.free(api_key);

    const response = try chatgpt.chatCompletion(allocator, api_key, "Hello from Zig!");
    defer allocator.free(response);

    try stdout.print("ChatGPT raw response: {s}\n", .{response});

    // Create a simple Win32 window via C and run the message loop.
    _ = c.CreateSimpleWindow();
}

