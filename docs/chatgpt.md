ChatGPT Integration
===================

`src/chatgpt.zig` demonstrates how to invoke the OpenAI ChatGPT API from Zig. It reads the API key from the `OPENAI_API_KEY` environment variable and sends a simple message.

```
const api_key = std.process.getEnvVarOwned(allocator, "OPENAI_API_KEY") catch return;
const response = try chatgpt.chatCompletion(allocator, api_key, "Hello from Zig!");
```

The raw JSON response from the API is printed to stdout.
