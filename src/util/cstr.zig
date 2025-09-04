const std = @import("std");

pub fn toCString(allocator: std.mem.Allocator, s: []const u8) ![]u8 {
    var buf = try allocator.alloc(u8, s.len + 1);
    @memcpy(buf[0..s.len], s);
    buf[s.len] = 0;
    return buf;
}

