// Public entry for the OpenAI helpers.
// Re-export submodules so callers can `@import("openai")` and access APIs.
pub const chatgpt = @import("chatgpt.zig");
pub const models = @import("models.zig");

// Convenience re-exports
pub const chatCompletion = chatgpt.chatCompletion;
pub const chatCompletionWithEnvModel = chatgpt.chatCompletionWithEnvModel;
pub const fetchModelsJson = models.fetchModelsJson;
pub const extractModelIds = models.extractModelIds;
