ChatGPT (OpenAI) Integration
===========================

The OpenAI helpers live under `src/openai/` and are exposed via the named module `openai`.

Setup
-----
- API Key: set `OPENAI_API_KEY` in your environment (e.g., `.env`).
- Optional: set `OPENAI_MODEL` (defaults to `gpt-3.5-turbo`).

Usage
-----
```
const std = @import("std");
const openai = @import("openai");

pub fn main() !void {
    const allocator = std.heap.page_allocator;
    const api_key = try std.process.getEnvVarOwned(allocator, "OPENAI_API_KEY");
    defer allocator.free(api_key);

    const resp = try openai.chatCompletionWithEnvModel(allocator, api_key, "Hello from Zig!");
    defer allocator.free(resp);
    // resp is a JSON string from the Chat Completions API
}
```

Notes
-----
- To use a specific model without an env var, call `openai.chatCompletion(alloc, api_key, "gpt-4o-mini", prompt)`.
- See also: `src/openai/models.zig` for listing available models and `docs/build-and-linking.md` for how the `openai` module is wired in `build.zig`.
