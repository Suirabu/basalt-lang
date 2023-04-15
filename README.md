# Basalt Programming Language

Basalt is an experimental programming language I am working on to learn more about the
[compiler bootstrapping](https://en.wikipedia.org/wiki/Bootstrapping_(compilers)) process.

My goal is to write a self-hosted Basalt compiler as early as possible. Once the compiler has been
rewritten in Basalt I will then continue to add more features to the language.

## Usage

To build the initial bootstrap compiler you must have the latest stable release of the 
[Zig compiler](https://ziglang.org/download/) (currently version 0.10.1) installed on your system.
You can then build the bootstrap compiler with the following command:

```
$ zig build
```

The resulting binary can then be found in `zig-out/bin/`.

## Example

```clike
var a: int = 15

while a > 0 do
    a -= 1
end
```

## License

This project is licensed under the permissive MIT license.
