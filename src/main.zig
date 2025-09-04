//! Entry point for the Zig + C (Win32) sample.
//! - Demonstrates calling C functions from Zig using @cImport
//! - Shows a simple Win32 window via a C helper implementation
const std = @import("std");
const models_feature = @import("features/model_list.zig");
const chat_feature = @import("features/chat.zig");
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

pub export fn OnSendChatRequest(message: [*:0]const u8) callconv(.c) void {
    // Convert C string (null-terminated) to slice
    var len: usize = 0;
    while (message[len] != 0) : (len += 1) {}
    const slice: []const u8 = message[0..len];
    chat_feature.sendChatOnce(std.heap.page_allocator, slice);
}

pub export fn OnSendChatRequestWithModel(message: [*:0]const u8, model: [*:0]const u8) callconv(.c) void {
    var msg_len: usize = 0;
    while (message[msg_len] != 0) : (msg_len += 1) {}
    var mdl_len: usize = 0;
    while (model[mdl_len] != 0) : (mdl_len += 1) {}
    const msg_slice: []const u8 = message[0..msg_len];
    const mdl_slice: []const u8 = model[0..mdl_len];
    chat_feature.sendChatOnceWithModel(std.heap.page_allocator, msg_slice, mdl_slice);
}
