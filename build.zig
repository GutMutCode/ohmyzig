const std = @import("std");

// Build script for the Zig + C (Win32) sample app.
// - Builds a GUI subsystem executable (no console window)
// - Compiles C sources in src/ with headers in inc/
// - Links required Win32 libraries
pub fn build(b: *std.Build) void {
    // Target/optimize options are configurable via CLI flags
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Main Zig module (root of the application)
    const main_module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Build an executable named "ohmyzig"
    const exe = b.addExecutable(.{
        .name = "ohmyzig",
        .root_module = main_module,
    });

    // Make headers in ./inc available to Zig C imports and C compilation
    exe.addIncludePath(b.path("inc"));

    // Compile our C sources alongside Zig and pass include flag for C as well
    exe.addCSourceFiles(.{
        .files = &.{ "src/c_functions.c", "src/win32_ui.c" },
        .flags = &.{ "-Iinc" },
    });

    // Link C runtime (printf in c_functions.c) and Win32 system libraries
    exe.linkLibC();
    exe.linkSystemLibrary("user32"); // Window creation, message loop, etc.
    exe.linkSystemLibrary("gdi32");  // Basic text drawing (TextOut)
    // exe.linkSystemLibrary("kernel32"); // Not required explicitly here

    // Build as a GUI subsystem app so no console is attached
    exe.subsystem = .Windows;

    // Install artifact for `zig build`
    b.installArtifact(exe);

    // `zig build run` convenience step
    const run_exe = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);
}

