const std = @import("std");

pub const ModelsResponse = struct {
    status: std.http.Status,
    body: []u8,
};

// Fetches the raw Models list JSON from OpenAI.
// Caller owns the returned body and must free it.
pub fn fetchModelsJson(allocator: std.mem.Allocator, api_key: []const u8) !ModelsResponse {
    var client = std.http.Client{ .allocator = allocator };
    defer client.deinit();

    const auth_header = try std.fmt.allocPrint(allocator, "Bearer {s}", .{api_key});
    defer allocator.free(auth_header);

    var req = try client.request(.{
        .method = .GET,
        .url = "https://api.openai.com/v1/models",
        .headers = &.{
            .{ .name = "Authorization", .value = auth_header },
        },
    });
    defer req.deinit();

    try req.finish();
    const status = req.response.status;
    const body = try req.readToEndAlloc(allocator, 1024 * 1024);
    return .{ .status = status, .body = body };
}

// Basic parser that extracts model ids from the models JSON response.
// Returns a heap-allocated array of heap-allocated strings (caller frees all).
pub fn extractModelIds(allocator: std.mem.Allocator, json_bytes: []const u8) ![][]u8 {
    var parsed = try std.json.parseFromSlice(std.json.Value, allocator, json_bytes, .{ .ignore_unknown_fields = true });
    defer parsed.deinit();

    const root = parsed.value;
    const data = root.object.get("data") orelse return error.InvalidResponse;
    var list = std.ArrayList([]u8).init(allocator);
    defer list.deinit();

    const arr = data.array orelse return error.InvalidResponse;
    for (arr.items) |item| {
        const obj = item.object orelse continue;
        const id_val = obj.get("id") orelse continue;
        const id_str = id_val.string orelse continue;
        const dup = try allocator.dupe(u8, id_str);
        try list.append(dup);
    }

    return try list.toOwnedSlice();
}
