const std = @import("std");

// Calls OpenAI Chat Completions API.
// model: pass any available chat model id (e.g., "gpt-4o-mini", "gpt-3.5-turbo").
// prompt: user content. Properly JSON-escaped before sending.
pub fn chatCompletion(
    allocator: std.mem.Allocator,
    api_key: []const u8,
    model: []const u8,
    prompt: []const u8,
) ![]u8 {
    var client = std.http.Client{ .allocator = allocator };
    defer client.deinit();

    const auth_header = try std.fmt.allocPrint(allocator, "Bearer {s}", .{api_key});
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

    // Escape the prompt as a JSON string (includes quotes)
    const escaped_prompt = try std.json.stringifyAlloc(allocator, prompt, .{});
    defer allocator.free(escaped_prompt);

    // Compose request body with dynamic model and escaped prompt
    const body = try std.fmt.allocPrint(
        allocator,
        "{{\"model\":\"{s}\",\"messages\":[{{\"role\":\"user\",\"content\":{s}}}]}}",
        .{ model, escaped_prompt },
    );
    defer allocator.free(body);

    try req.writeAll(body);
    try req.finish();

    const resp_body = try req.readToEndAlloc(allocator, 1024 * 1024);
    return resp_body;
}

// Optional helper: picks model from env var OPENAI_MODEL, or defaults to gpt-3.5-turbo.
pub fn chatCompletionWithEnvModel(
    allocator: std.mem.Allocator,
    api_key: []const u8,
    prompt: []const u8,
) ![]u8 {
    var model_buf = std.ArrayList(u8).init(allocator);
    defer model_buf.deinit();

    // Try to read OPENAI_MODEL; if missing, fallback
    const env_model = std.process.getEnvVarOwned(allocator, "OPENAI_MODEL") catch null;
    if (env_model) |m| {
        defer allocator.free(m);
        try model_buf.appendSlice(m);
    } else {
        try model_buf.appendSlice("gpt-3.5-turbo");
    }

    return try chatCompletion(allocator, api_key, model_buf.items, prompt);
}
