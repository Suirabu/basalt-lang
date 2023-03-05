# Basalt Programming Language

Basalt is an experimental programming language I am working on to learn more about the
[compiler bootstrapping](https://en.wikipedia.org/wiki/Bootstrapping_(compilers)) process.

My goal is to write a self-hosted Basalt compiler as early as possible. Once the compiler has been
rewritten in Basalt I will then continue to add more features to the language.

## Building

To build the initial bootstrap compiler you must have a C compiler and GNU Make installed on your
system. The compiler can then be built by running `make` in your terminal. The resulting binary can
be found in `bin/`.

## Example

```clike
var a: int = 15

while a > 0 do
    a -= 1
end
```

## License

This project is licensed under the permissive MIT license.
