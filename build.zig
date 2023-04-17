const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    // Allows the person running `zig build` to build local documentation using
    // zig autodoc with the `-Demit-docs` flag.
    const emit_docs = b.option(bool, "emit-docs", "Generate local documentation") orelse false;

    const exe = b.addExecutable("basalt", "src/main.zig");
    exe.setTarget(target);
    exe.setBuildMode(mode);

    // Link LibC and declare C source files
    exe.linkLibC();
    exe.addIncludePath("src");
    exe.addCSourceFiles(&.{
        "src/codegen.c",
        "src/expr.c",
        "src/global.c",
        "src/lexer.c",
        "src/parser.c",
        "src/sema.c",
        "src/symbol.c",
        "src/token.c",
        "src/typecheck.c",
        "src/value.c",
    }, &.{});

    // Emit local documentation if the `-Demit-docs` flag is enabled
    exe.emit_docs = if (emit_docs) .emit else .default;

    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
