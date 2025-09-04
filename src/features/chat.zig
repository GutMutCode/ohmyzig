const std = @import("std");
const ui = @import("../platform/ui.zig");
const http = @import("../platform/http.zig");
const secrets = @import("../platform/secrets.zig");

fn getApiKey(allocator: std.mem.Allocator) ?[]u8 {
    // Try env
    if (std.process.getEnvVarOwned(allocator, "OPENAI_API_KEY") catch null) |k| return k;
    // Try encrypted cache
    if (secrets.loadEncrypted(allocator) catch null) |loaded| return loaded;
    // Prompt user
    var save = false;
    const maybe_key = ui.promptApiKey(allocator, &save);
    if (maybe_key) |kz| {
        if (save) {
            secrets.saveEncrypted(allocator, kz) catch {};
        }
        return kz;
    }
    return null;
}

fn buildAuthHeader(allocator: std.mem.Allocator, api_key: []const u8) ![]u8 {
    return std.fmt.allocPrint(allocator, "Authorization: Bearer {s}\r\nContent-Type: application/json\r\n", .{api_key});
}

fn jsonEscape(allocator: std.mem.Allocator, s: []const u8) ![]u8 {
    var out = try allocator.alloc(u8, s.len * 2);
    var n: usize = 0;
    for (s) |ch| {
        switch (ch) {
            '"' => { out[n] = '\\'; n += 1; out[n] = '"'; n += 1; },
            '\\' => { out[n] = '\\'; n += 1; out[n] = '\\'; n += 1; },
            '\n' => { out[n] = '\\'; n += 1; out[n] = 'n'; n += 1; },
            '\r' => { out[n] = '\\'; n += 1; out[n] = 'r'; n += 1; },
            '\t' => { out[n] = '\\'; n += 1; out[n] = 't'; n += 1; },
            else => |c| {
                if (c < 0x20) {
                    const hex = "0123456789abcdef";
                    out[n] = '\\'; n += 1;
                    out[n] = 'u'; n += 1;
                    out[n] = '0'; n += 1;
                    out[n] = '0'; n += 1;
                    out[n] = hex[(c >> 4) & 0xF]; n += 1;
                    out[n] = hex[c & 0xF]; n += 1;
                } else {
                    out[n] = c; n += 1;
                }
            },
        }
    }
    return out[0..n];
}

fn buildRequestBody(allocator: std.mem.Allocator, model: []const u8, user_text: []const u8) ![]u8 {
    const esc = try jsonEscape(allocator, user_text);
    defer allocator.free(esc);
    return std.fmt.allocPrint(allocator,
        "{{\"model\":\"{s}\",\"input\":\"{s}\",\"reasoning\":{{\"effort\":\"medium\"}}}}",
        .{ model, esc },
    );
}

fn extractText(allocator: std.mem.Allocator, body: []const u8) ![]u8 {
    // Try to parse flexible shapes: prefer `output_text`, else `choices[0].message.content`
    var parsed = try std.json.parseFromSlice(std.json.Value, allocator, body, .{ .ignore_unknown_fields = true });
    defer parsed.deinit();
    const root = parsed.value;
    if (root.object.get("output_text")) |ot| {
        if (ot.string.len > 0) return try allocator.dupe(u8, ot.string);
    }
    if (root.object.get("choices")) |choices| {
        const arr = choices.array.items;
        if (arr.len > 0) {
            const first = arr[0];
            if (first.object.get("message")) |msg| {
                if (msg.object.get("content")) |content| {
                    if (content.string.len > 0) return try allocator.dupe(u8, content.string);
                }
            }
            if (first.object.get("text")) |txt| {
                if (txt.string.len > 0) return try allocator.dupe(u8, txt.string);
            }
        }
    }
    // Fallback: return original body slice if nothing parseable
    return try allocator.dupe(u8, body);
}

fn chooseModelOrDefault(model_opt: ?[]const u8) []const u8 {
    if (model_opt) |m| if (m.len > 0) return m;
    return "gpt-5.1-mini";
}

pub fn sendChatOnce(allocator: std.mem.Allocator, user_text: []const u8) void {
    sendChatOnceWithModel(allocator, user_text, null);
}

pub fn sendChatOnceWithModel(allocator: std.mem.Allocator, user_text: []const u8, model_opt: ?[]const u8) void {
    const api_key_opt = getApiKey(allocator);
    if (api_key_opt == null) {
        ui.showInfoMessage(allocator, "Chat", "API key required.");
        return;
    }
    const api_key = api_key_opt.?;
    defer allocator.free(api_key);

    const header = buildAuthHeader(allocator, api_key) catch |e| {
        var buf: [128]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "Header error: {s}", .{@errorName(e)}) catch "Header error";
        ui.showInfoMessage(allocator, "Chat", msg);
        return;
    };
    defer allocator.free(header);

    const body = buildRequestBody(allocator, chooseModelOrDefault(model_opt), user_text) catch |e| {
        var buf: [128]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "Body build error: {s}", .{@errorName(e)}) catch "Body build error";
        ui.showInfoMessage(allocator, "Chat", msg);
        return;
    };
    defer allocator.free(body);

    const resp = http.postJson(allocator, "https://api.openai.com/v1/responses", header, body) catch |e| {
        var buf: [160]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "HTTP error: {s}", .{@errorName(e)}) catch "HTTP error";
        ui.showInfoMessage(allocator, "Chat", msg);
        return;
    };
    defer allocator.free(resp);

    const text = extractText(allocator, resp) catch |e| blk: {
        var buf: [160]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "Parse error: {s}", .{@errorName(e)}) catch "Parse error";
        ui.showInfoMessage(allocator, "Chat", msg);
        break :blk null;
    };
    if (text) |t| {
        defer allocator.free(t);
        ui.setMainText(allocator, t);
    }
}
