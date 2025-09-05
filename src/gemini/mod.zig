// Public entry for the Gemini helpers.
// Re-export submodules so callers can `@import("gemini")` and access APIs.
pub const generate = @import("generate.zig");
pub const models = @import("models.zig");

// Convenience re-exports
pub const generateContent = generate.generateContent;
pub const generateContentWithEnvModel = generate.generateContentWithEnvModel;
pub const fetchModelsJson = models.fetchModelsJson;
pub const extractModelIds = models.extractModelIds;
