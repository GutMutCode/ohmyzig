const std = @import("std");

const c = @cImport({
    @cInclude("win_secret.h");
});
const cstr = @import("../util/cstr.zig");

fn appDataKeyPath(allocator: std.mem.Allocator) ![]u8 {
    const appdata = try std.process.getEnvVarOwned(allocator, "APPDATA");
    defer allocator.free(appdata);
    const dir = try std.fs.path.join(allocator, &.{ appdata, "ohmyzig" });
    defer allocator.free(dir);
    try std.fs.cwd().makePath(dir);
    return std.fs.path.join(allocator, &.{ dir, "openai.key" });
}

pub fn saveEncrypted(allocator: std.mem.Allocator, api_key: []const u8) !void {
    const path = try appDataKeyPath(allocator);
    defer allocator.free(path);
    const c_path = try cstr.toCString(allocator, path);
    defer allocator.free(c_path);
    const ok = c.dpapi_encrypt_and_save(c_path.ptr, api_key.ptr, @as(c_ulong, @intCast(api_key.len)));
    if (ok == 0) return error.AccessDenied;
}

pub fn loadEncrypted(allocator: std.mem.Allocator) !?[]u8 {
    const path = appDataKeyPath(allocator) catch return null;
    defer allocator.free(path);
    const c_path = cstr.toCString(allocator, path) catch return null;
    defer allocator.free(c_path);
    var out_ptr: [*c]u8 = @as([*c]u8, @ptrFromInt(0));
    var out_len: c_ulong = 0;
    const ok = c.dpapi_load_and_decrypt(c_path.ptr, &out_ptr, &out_len);
    if (ok == 0 or out_ptr == @as([*c]u8, @ptrFromInt(0)) or out_len == 0) return null;
    defer c.secret_free(out_ptr);
    const bytes = @as([*]u8, @ptrCast(out_ptr))[0..out_len];
    return try allocator.dupe(u8, bytes);
}
