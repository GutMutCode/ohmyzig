//! Entry point for the Zig + C (Win32) sample.
//! - Demonstrates calling C functions from Zig using @cImport
//! - Shows a simple Win32 window via a C helper implementation
const std = @import("std");
const models_feature = @import("features/model_list.zig");
const ui = @import("platform/ui.zig");

// Import C headers so their functions are available as `c.*` in Zig.
// c_functions.h: simple arithmetic example (uses libc printf)
// win32_ui.h   : small Win32 wrapper that creates and shows a window
const c = @cImport({
    @cInclude("c_functions.h");
    @cInclude("win32_ui.h");
});

pub fn main() anyerror!void {
    // Start the Win32 UI and run the message loop.
    // Create a simple Win32 window and run the message loop.
    ui.createSimpleWindow();
}

pub export fn OnShowModelsRequest() callconv(.c) void {
    models_feature.showModelsOfflineFirst(std.heap.page_allocator);
}
