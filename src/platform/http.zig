const std = @import("std");

const c = @cImport({
    @cInclude("win_http.h");
});
const cstr = @import("../util/cstr.zig");

// Performs HTTP GET with an optional extra header block.
// Returns a Zig-allocated body buffer the caller must free.
pub fn get(allocator: std.mem.Allocator, url: []const u8, extra_header: []const u8) ![]u8 {
    const url_c = try cstr.toCString(allocator, url);
    defer allocator.free(url_c);
    const hdr_c = try cstr.toCString(allocator, extra_header);
    defer allocator.free(hdr_c);

    var out_ptr: [*c]u8 = @as([*c]u8, @ptrFromInt(0));
    var out_len: c_ulong = 0;
    const ok = c.http_get_with_header(url_c.ptr, hdr_c.ptr, &out_ptr, &out_len);
    if (ok == 0 or out_ptr == @as([*c]u8, @ptrFromInt(0)) or out_len == 0) return error.NetworkError;
    defer c.http_free(out_ptr);

    const bytes = @as([*]u8, @ptrCast(out_ptr))[0..out_len];
    return allocator.dupe(u8, bytes);
}

// Performs HTTP POST with JSON body and extra headers (e.g. Authorization).
pub fn postJson(allocator: std.mem.Allocator, url: []const u8, extra_header: []const u8, body: []const u8) ![]u8 {
    const url_c = try cstr.toCString(allocator, url);
    defer allocator.free(url_c);
    const hdr_c = try cstr.toCString(allocator, extra_header);
    defer allocator.free(hdr_c);
    const body_c = try cstr.toCString(allocator, body);
    defer allocator.free(body_c);

    var out_ptr: [*c]u8 = @as([*c]u8, @ptrFromInt(0));
    var out_len: c_ulong = 0;
    const ok = c.http_post_json_with_header(url_c.ptr, hdr_c.ptr, body_c.ptr, &out_ptr, &out_len);
    if (ok == 0 or out_ptr == @as([*c]u8, @ptrFromInt(0)) or out_len == 0) return error.NetworkError;
    defer c.http_free(out_ptr);
    const bytes = @as([*]u8, @ptrCast(out_ptr))[0..out_len];
    return allocator.dupe(u8, bytes);
}
