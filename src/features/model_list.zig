const std = @import("std");
const ui = @import("../platform/ui.zig");
const http = @import("../platform/http.zig");
const secrets = @import("../platform/secrets.zig");

fn showMessageBox(allocator: std.mem.Allocator, title: []const u8, body: []const u8) void {
    ui.showInfoMessage(allocator, title, body);
}

// Fetches available model ids from OpenAI and displays them in a Win32 message box.
pub fn showAvailableModelsWithWin32(allocator: std.mem.Allocator) void {
    var api_key_mem: ?[]u8 = null;
    var api_key_slice: []const u8 = undefined;
    // 1) Try process env
    if (std.process.getEnvVarOwned(allocator, "OPENAI_API_KEY") catch null) |k| {
        api_key_mem = k;
        api_key_slice = k;
    } else {
        // 2) Try encrypted key in AppData
        if (secrets.loadEncrypted(allocator) catch null) |loaded| {
            api_key_mem = loaded;
            api_key_slice = loaded;
        } else {
            // 3) Prompt user for API key
            var save = false;
            const maybe_key = ui.promptApiKey(allocator, &save);
            if (maybe_key) |kz| {
                api_key_mem = kz;
                api_key_slice = kz;
                if (save) {
                    secrets.saveEncrypted(allocator, api_key_slice) catch |e| {
                        var buf: [160]u8 = undefined;
                        const msg = std.fmt.bufPrint(&buf, "Failed to save encrypted key: {s}", .{@errorName(e)}) catch "Failed to save encrypted key";
                        showMessageBox(allocator, "OpenAI Models", msg);
                    };
                }
            } else {
                showMessageBox(allocator, "OpenAI Models", "API key is required to list models.");
                return;
            }
        }
    }
    defer if (api_key_mem) |m| allocator.free(m);

    // HTTP GET /v1/models using WinINet (works reliably under Zig 0.15)
    // Build Authorization header: "Authorization: Bearer <key>\r\n"
    const header = std.fmt.allocPrint(allocator, "Authorization: Bearer {s}\r\n", .{api_key_slice}) catch |e| {
        var buf: [128]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "Header build error: {s}", .{@errorName(e)}) catch "Header build error";
        showMessageBox(allocator, "OpenAI Models", msg);
        return;
    };
    defer allocator.free(header);

    const body = http.get(allocator, "https://api.openai.com/v1/models", header) catch |e| {
        // Try fallback to cached list: just refresh the combo box silently
        if (loadModelsCache(allocator) catch null) |cached| {
            defer allocator.free(cached);
            ui.setModelOptions(allocator, cached);
            return;
        }
        var buf: [128]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "HTTP error: {s}", .{@errorName(e)}) catch "HTTP error";
        showMessageBox(allocator, "OpenAI Models", msg);
        return;
    };
    defer allocator.free(body);

    // Parse and extract model ids from JSON
    var parsed = std.json.parseFromSlice(std.json.Value, allocator, body, .{ .ignore_unknown_fields = true }) catch |e| {
        var buf: [256]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, "Parse error: {s}", .{@errorName(e)}) catch "Parse error";
        showMessageBox(allocator, "OpenAI Models", msg);
        return;
    };
    defer parsed.deinit();

    const root = parsed.value;
    const data = root.object.get("data") orelse {
        showMessageBox(allocator, "OpenAI Models", "Invalid response: missing 'data'");
        return;
    };
    const arr = data.array;
    // Join ids with newlines into a single buffer
    var list_buf: std.ArrayListUnmanaged(u8) = .{};
    defer list_buf.deinit(allocator);
    var count: usize = 0;
    for (arr.items) |item| {
        const obj = item.object;
        const id_val = obj.get("id") orelse continue;
        const id_str = id_val.string;
        if (count > 0) _ = list_buf.append(allocator, '\n') catch {};
        _ = list_buf.appendSlice(allocator, id_str) catch {};
        count += 1;
    }
    if (count == 0) {
        _ = list_buf.appendSlice(allocator, "No models returned.") catch {};
    }

    // Convert to Windows CRLF for proper line breaks in EDIT control
    const crlf_text_opt = toCRLF(allocator, list_buf.items) catch null;
    if (crlf_text_opt) |crlf_text| {
        defer allocator.free(crlf_text);
        // Save cache for offline viewing
        saveModelsCache(allocator, crlf_text) catch {};
        // Populate model selector with LF list only
        ui.setModelOptions(allocator, list_buf.items);
    } else {
        // Fallback: populate selector with LF text
        ui.setModelOptions(allocator, list_buf.items);
    }
}

// Shows cached list if present; otherwise fetches online and shows.
pub fn showModelsOfflineFirst(allocator: std.mem.Allocator) void {
    if (loadModelsCache(allocator) catch null) |cached| {
        defer allocator.free(cached);
        // Only update the model selector; do not display the list in the main area
        ui.setModelOptions(allocator, cached);
        return;
    }
    // No cache; fetch online (this will also save cache on success)
    showAvailableModelsWithWin32(allocator);
}

// Called by the button and on startup to (re)populate main window list area.
pub fn updateModelsInMain(allocator: std.mem.Allocator) void {
    // Try online first; on error, show cached
    const header_result = blk: {
        // Reuse the same flow by invoking the public function
        showAvailableModelsWithWin32(allocator);
        break :blk {};
    };
    _ = header_result;
}

fn toCRLF(allocator: std.mem.Allocator, s: []const u8) ![]u8 {
    // Worst case allocate 2x for simplicity
    var out = try allocator.alloc(u8, s.len * 2);
    var n: usize = 0;
    for (s) |ch| {
        if (ch == '\n') {
            out[n] = '\r';
            n += 1;
            out[n] = '\n';
            n += 1;
        } else {
            out[n] = ch;
            n += 1;
        }
    }
    return out[0..n];
}

fn modelsCachePath(allocator: std.mem.Allocator) ![]u8 {
    const appdata = try std.process.getEnvVarOwned(allocator, "APPDATA");
    defer allocator.free(appdata);
    const dir = try std.fs.path.join(allocator, &.{ appdata, "ohmyzig" });
    defer allocator.free(dir);
    try std.fs.cwd().makePath(dir);
    return try std.fs.path.join(allocator, &.{ dir, "models.txt" });
}

fn saveModelsCache(allocator: std.mem.Allocator, text: []const u8) !void {
    const path = try modelsCachePath(allocator);
    defer allocator.free(path);
    var file = try std.fs.cwd().createFile(path, .{ .truncate = true });
    defer file.close();
    _ = try file.writeAll(text);
}

fn loadModelsCache(allocator: std.mem.Allocator) !?[]u8 {
    const path = modelsCachePath(allocator) catch return null;
    defer allocator.free(path);
    const file = std.fs.cwd().openFile(path, .{ .mode = .read_only }) catch return null;
    defer file.close();
    const data = try file.readToEndAlloc(allocator, 1 << 20);
    return data;
}

// platform-specific helpers moved into src/platform/* adapters
