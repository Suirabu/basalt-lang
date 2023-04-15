const std = @import("std");
const debug = std.debug;

const c = @import("c.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    var args = try std.process.argsAlloc(gpa.allocator());
    defer gpa.allocator().free(args);

    if (args.len != 2) {
        std.log.info("Usage: {s} <file>", .{args[0]});
        fatal("invalid usage", .{});
    }

    const source_path = args[1];
    // Read source from disk
    const source = blk: {
        var source_file = try std.fs.cwd().openFile(source_path, .{});
        defer source_file.close();
        const stat = try source_file.stat();

        var source_buffer = try gpa.allocator().allocSentinel(u8, stat.size, 0);
        debug.assert(try source_file.readAll(source_buffer) == stat.size);
        break :blk source_buffer;
    };
    defer gpa.allocator().free(source);

    // Collect tokens
    var token_list = std.ArrayList(c.Token).init(gpa.allocator());
    defer token_list.deinit();
    {
        var lexer = c.Lexer{
            .source = source.ptr,
            .sp = 0,
            .line = 0,
            .column = 0,
            .source_path = source_path.ptr,
        };
        var current_token: c.Token = undefined;
        var has_error = false;

        while (true) {
            if (!c.lexer_collect_token(&lexer, &current_token)) {
                has_error = true;
            } else {
                try token_list.append(current_token);
                if (current_token.@"type" == c.TOK_EOF)
                    break;
            }
        }

        if (has_error)
            return error.LexerError;
    }

    // Parse expressions
    var parser = c.Parser{
        .tokens = token_list.items.ptr,
        .tp = 0,
    };
    var expr_list = std.ArrayList([*c]c.Expr).init(gpa.allocator());
    defer expr_list.deinit();
    {
        var has_error = false;

        while (!c.parser_reached_end(&parser)) {
            const expr_or_null = c.parser_collect_expr(&parser);
            if (expr_or_null) |expr| {
                try expr_list.append(expr);
            } else {
                has_error = true;
            }
        }

        if (has_error)
            return error.ParserError;
    }

    // Ensure `main` function exists and is properly defined
    if (!c.symbol_exists("main")) {
        std.log.err("function `main` is not defined", .{});
        return error.MissingMainDefinition;
    } else {
        const main_symbol = c.symbol_get("main").*;
        if (main_symbol.stype != c.SYM_FN or main_symbol.n_params != 0 or main_symbol.return_type != c.VAL_INT) {
            std.log.err("symbol `main` must be a function with no parameters and a return type of `int`", .{});
            return error.IllegalMainDefinition;
        }
    }

    // Typechecking
    if (!c.typecheck_exprs(expr_list.items.ptr, expr_list.items.len))
        return error.TypecheckError;

    // Semantic analysis
    if (!c.sema_analyze(expr_list.items.ptr, expr_list.items.len))
        return error.SemanticError;

    // Assemble and link executable
    {
        const source_path_stem = stem(source_path);

        var obj_path = try std.fmt.allocPrint(gpa.allocator(), "{s}.o{c}", .{ source_path_stem, 0 });
        defer gpa.allocator().free(obj_path);

        // Assemble generated output
        {
            const asm_path = try std.fmt.allocPrint(gpa.allocator(), "{s}.asm{c}", .{ source_path_stem, 0 });
            defer gpa.allocator().free(asm_path);

            if (!c.generate_assembly(expr_list.items.ptr, expr_list.items.len, asm_path.ptr))
                return error.CodegenError;

            const result = try std.ChildProcess.exec(.{
                .allocator = gpa.allocator(),
                .argv = &.{ "yasm", "-f", "elf64", asm_path, "-o", obj_path },
            });
            debug.assert(result.term.Exited == 0);
        }

        // Link executable
        {
            const result = try std.ChildProcess.exec(.{
                .allocator = gpa.allocator(),
                .argv = &.{ "ld", obj_path, "-o", source_path_stem },
            });
            debug.assert(result.term.Exited == 0);
            obj_path.len -= 1; // HACK: Ignore null terminator
            try std.fs.cwd().deleteFile(obj_path); // Delete object file
        }
    }

    c.global_free_all();
}

// Defined in Zig 0.11.0's standard library at `std.fs.path.stem`
// Redefined here since we're using Zig 0.10.1
pub fn stem(path: []const u8) []const u8 {
    const filename = std.fs.path.basename(path);
    const index = std.mem.lastIndexOfScalar(u8, filename, '.') orelse return filename[0..];
    if (index == 0) return path;
    return filename[0..index];
}

pub fn fatal(comptime fmt: []const u8, args: anytype) noreturn {
    std.log.err(fmt, args);
    std.process.exit(1);
}
