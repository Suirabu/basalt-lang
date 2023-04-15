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

    const exe = b.addExecutable("basalt", null);
    exe.setTarget(target);
    exe.setBuildMode(mode);

    // Link LibC and declare C source files
    exe.linkLibC();
    exe.addCSourceFiles(&.{
        "src/codegen.c",
        "src/expr.c",
        "src/global.c",
        "src/lexer.c",
        "src/main.c",
        "src/parser.c",
        "src/sema.c",
        "src/symbol.c",
        "src/token.c",
        "src/typecheck.c",
        "src/value.c",
    }, &.{});
    exe.install();
}
