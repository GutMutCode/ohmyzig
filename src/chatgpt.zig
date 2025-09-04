const std = @import("std");

pub fn chatCompletion(allocator: std.mem.Allocator, api_key: []const u8, prompt: []const u8) ![]u8 {
    var client = std.http.Client{ .allocator = allocator };
    defer client.deinit();

    var auth_header = try std.fmt.allocPrint(allocator, "Bearer {s}", .{ api_key });
    defer allocator.free(auth_header);

    var req = try client.request(.{
        .method = .POST,
        .url = "https://api.openai.com/v1/chat/completions",
        .headers = &.{
            .{ .name = "Content-Type", .value = "application/json" },
            .{ .name = "Authorization", .value = auth_header },
        },
    });
    defer req.deinit();

    const body = try std.fmt.allocPrint(allocator,
        "{\"model\":\"gpt-3.5-turbo\",\"messages\":[{\"role\":\"user\",\"content\":\"{s}\"}]}",
        .{ prompt },
    );
    defer allocator.free(body);

    try req.writeAll(body);
    try req.finish();

    const resp_body = try req.readToEndAlloc(allocator, 1024 * 1024);
    return resp_body;
}
