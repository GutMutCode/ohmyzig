const std = @import("std");

/// Calls Gemini's text generation API.
/// model: pass any available model id (e.g., "gemini-1.5-flash").
/// prompt: user content. Properly JSON-escaped before sending.
pub fn generateContent(
    allocator: std.mem.Allocator,
    api_key: []const u8,
    model: []const u8,
    prompt: []const u8,
) ![]u8 {
    var client = std.http.Client{ .allocator = allocator };
    defer client.deinit();

    const url = try std.fmt.allocPrint(
        allocator,
        "https://generativelanguage.googleapis.com/v1beta/models/{s}:generateContent?key={s}",
        .{ model, api_key },
    );
    defer allocator.free(url);

    var req = try client.request(.{
        .method = .POST,
        .url = url,
        .headers = &.{
            .{ .name = "Content-Type", .value = "application/json" },
        },
    });
    defer req.deinit();

    // Escape the prompt as a JSON string (includes quotes)
    const escaped_prompt = try std.json.stringifyAlloc(allocator, prompt, .{});
    defer allocator.free(escaped_prompt);

    const body = try std.fmt.allocPrint(
        allocator,
        "{{\"contents\":[{{\"parts\":[{{\"text\":{s}}}]}}]}}",
        .{ escaped_prompt },
    );
    defer allocator.free(body);

    try req.writeAll(body);
    try req.finish();

    const resp_body = try req.readToEndAlloc(allocator, 1024 * 1024);
    return resp_body;
}

/// Helper that chooses model from GEMINI_MODEL env var or defaults to gemini-1.5-flash.
pub fn generateContentWithEnvModel(
    allocator: std.mem.Allocator,
    api_key: []const u8,
    prompt: []const u8,
) ![]u8 {
    var model_buf = std.ArrayList(u8).init(allocator);
    defer model_buf.deinit();

    const env_model = std.process.getEnvVarOwned(allocator, "GEMINI_MODEL") catch null;
    if (env_model) |m| {
        defer allocator.free(m);
        try model_buf.appendSlice(m);
    } else {
        try model_buf.appendSlice("gemini-1.5-flash");
    }

    return try generateContent(allocator, api_key, model_buf.items, prompt);
}
