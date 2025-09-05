Gemini API Integration
======================

The Gemini helpers live under `src/gemini/` and are exposed via the named module `gemini`.

Setup
-----
- API Key: set `GEMINI_API_KEY` in your environment or provide it directly to the helper functions.
- Optional: set `GEMINI_MODEL` (defaults to `gemini-1.5-flash`).

Usage
-----
```
const std = @import("std");
const gemini = @import("gemini");

pub fn main() !void {
    const allocator = std.heap.page_allocator;
    const api_key = try std.process.getEnvVarOwned(allocator, "GEMINI_API_KEY");
    defer allocator.free(api_key);

    const resp = try gemini.generateContentWithEnvModel(allocator, api_key, "Hello from Zig!");
    defer allocator.free(resp);
    // resp is a JSON string from the Gemini API
}
```

Notes
-----
- To use a specific model without an env var, call `gemini.generateContent(alloc, api_key, "gemini-1.5-flash", prompt)`.
- See also: `src/gemini/models.zig` for listing available models and `docs/build-and-linking.md` for how the `gemini` module is wired in `build.zig`.
