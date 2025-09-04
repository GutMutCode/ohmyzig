const std = @import("std");

const c = @cImport({
    @cInclude("win32_ui.h");
});
const cstr = @import("../util/cstr.zig");

pub fn showInfoMessage(allocator: std.mem.Allocator, title: []const u8, body: []const u8) void {
    const t = cstr.toCString(allocator, title) catch return;
    defer allocator.free(t);
    const b = cstr.toCString(allocator, body) catch return;
    defer allocator.free(b);
    c.ShowInfoMessage(t.ptr, b.ptr);
}

pub fn showScrollableText(allocator: std.mem.Allocator, title: []const u8, body: []const u8) void {
    const t = cstr.toCString(allocator, title) catch return;
    defer allocator.free(t);
    const b = cstr.toCString(allocator, body) catch return;
    defer allocator.free(b);
    c.ShowScrollableText(t.ptr, b.ptr);
}

pub fn setMainText(allocator: std.mem.Allocator, body: []const u8) void {
    const b = cstr.toCString(allocator, body) catch return;
    defer allocator.free(b);
    c.SetMainText(b.ptr);
}

pub fn setModelOptions(allocator: std.mem.Allocator, list_newline: []const u8) void {
    const s = cstr.toCString(allocator, list_newline) catch return;
    defer allocator.free(s);
    c.SetModelOptions(s.ptr);
}

pub fn promptApiKey(allocator: std.mem.Allocator, out_save: *bool) ?[]u8 {
    var buf: [256]u8 = undefined;
    var save_flag: c_int = 0;
    if (c.PromptForApiKey(&buf, buf.len, &save_flag) == 0) return null;
    out_save.* = (save_flag != 0);

    var n: usize = 0;
    while (n < buf.len and buf[n] != 0) : (n += 1) {}
    const dup = allocator.dupe(u8, buf[0..n]) catch return null;
    return dup;
}

pub fn createSimpleWindow() void {
    _ = c.CreateSimpleWindow();
}
