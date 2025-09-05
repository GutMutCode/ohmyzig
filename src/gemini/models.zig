const std = @import("std");

pub const ModelsResponse = struct {
    status: std.http.Status,
    body: []u8,
};

/// Fetches the raw models list JSON from Gemini API.
/// Caller owns the returned body and must free it.
pub fn fetchModelsJson(allocator: std.mem.Allocator, api_key: []const u8) !ModelsResponse {
    var client = std.http.Client{ .allocator = allocator };
    defer client.deinit();

    const url = try std.fmt.allocPrint(
        allocator,
        "https://generativelanguage.googleapis.com/v1beta/models?key={s}",
        .{ api_key },
    );
    defer allocator.free(url);

    var req = try client.request(.{
        .method = .GET,
        .url = url,
    });
    defer req.deinit();

    try req.finish();
    const status = req.response.status;
    const body = try req.readToEndAlloc(allocator, 1024 * 1024);
    return .{ .status = status, .body = body };
}

/// Extracts model ids from Gemini models JSON response.
/// Returns a heap-allocated array of heap-allocated strings (caller frees all).
pub fn extractModelIds(allocator: std.mem.Allocator, json_bytes: []const u8) ![][]u8 {
    var parsed = try std.json.parseFromSlice(std.json.Value, allocator, json_bytes, .{ .ignore_unknown_fields = true });
    defer parsed.deinit();

    const root = parsed.value;
    const models = root.object.get("models") orelse return error.InvalidResponse;
    const arr = models.array orelse return error.InvalidResponse;

    var list = std.ArrayList([]u8).init(allocator);
    defer list.deinit();

    for (arr.items) |item| {
        const obj = item.object orelse continue;
        const name_val = obj.get("name") orelse continue;
        const name_str = name_val.string orelse continue;
        const dup = try allocator.dupe(u8, name_str);
        try list.append(dup);
    }

    return try list.toOwnedSlice();
}
