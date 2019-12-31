# Simple shell
## Compilation
Prerequisite: clang, cmake

```
export CC=/usr/bin/clang
cmake .
make
```

## Supported functions

This shell supports following functions.
- Single command
- Command with arguments
- Redirection, e.g. `echo 123 > 1.out`
- Pipes, e.g. `cat < 1.txt 2.txt | grep 1 > 3.txt`
- Ctrl D
- Build-in commands, e.g. `cd`, `pwd`
- Ctrl C
- Quotes
- Error handling
- Background commands and `jobs`




## Sample IO

```
sh $ echo 123
123
sh $ cat 123
cat: 123: No such file or directory
sh $ echo 123 > 1.out | cat
error: duplicated output redirection
sh $ echo 123 | cat
123
sh $ exit
exit
```
