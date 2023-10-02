# Basalt Programming Language

Basalt is an experimental programming language I am working on to learn more about the
[compiler bootstrapping](https://en.wikipedia.org/wiki/Bootstrapping_(compilers)) process.

My goal is to write a self-hosted Basalt compiler as early as possible. Once the compiler has been
rewritten in Basalt I will then continue to add more features to the language.

## Building

Use GNU Make to compile the initial bootstrap compiler (currently the only compiler).

```sh
$ make
```

The resulting binary can then be found in the `bin` directory.

## License

This project is licensed under the permissive MIT license.
