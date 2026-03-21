# decomment

A C17 CLI tool that safely strips comments from source files.

## What it does

decomment reads source files, removes comments, and writes the result back. It is designed with safety as the primary concern: it never truncates your original files, uses atomic writes, and skips files it does not understand.

## What it does NOT do

- It does not reformat or beautify code.
- It does not parse language grammars -- it uses lexical state machines, not ASTs.
- It does not modify string literals or other non-comment content.
- It does not handle every possible language or edge case -- when in doubt, it skips.

## Safety philosophy

decomment follows a **skip-on-doubt** policy:

- **Atomic writes**: output is written to a temporary file in the same directory, fsynced, then renamed over the original. If anything fails, the original is untouched.
- **Never truncates originals**: the original file is never opened for writing. A new file is written and atomically swapped in.
- **Skips unknown files**: if decomment cannot identify the language, it skips the file rather than guessing wrong.
- **Binary detection**: files containing NUL bytes in the first 8 KB are skipped.
- **Backup support**: optionally create a backup of the original before replacing.

## Supported languages

decomment supports 25 languages across 5 handler families:

| Handler family | Languages |
|---|---|
| C-like | C, C++, Objective-C, Java, JavaScript, TypeScript, C#, Go, Rust, Swift, Kotlin, Scala, Dart, CSS, PHP (limited) |
| Python | Python (with triple-quoted string support) |
| Shell | Shell (sh/bash/zsh) |
| Generic hash-comment | Ruby (limited), Perl (limited), R, YAML, TOML, Makefile |
| Lua | Lua (with long bracket comment/string support) |

## Unsupported languages

The following languages are **not** supported:

| Language | Reason |
|---|---|
| Haskell | Not implemented yet (nested block comments require special handling) |
| SQL | Not implemented yet (dialect-dependent comment syntax) |
| HTML, XML | Not implemented yet (markup comments interleave with content in complex ways) |
| Assembly | Not implemented yet (assembler-dependent comment syntax) |

The following edge cases in otherwise-supported languages are **dangerous** and deliberately not handled:

- **PHP heredocs/nowdocs**: complex quoting that can disguise comment-like syntax.
- **Ruby heredocs**: similar to PHP; heredoc bodies can contain comment-like lines.
- **Perl complex quoting**: arbitrary delimiters in `q{}`, `qq{}`, regex, etc.

decomment will process these languages but may produce incorrect output for files that use these constructs. Use `--check` mode to verify before committing changes.

## Building

### With Make

```sh
make
```

### With CMake

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The resulting binary is `decomment` (or `build/decomment` with CMake).

## Usage

### Single file

```sh
decomment src/main.c
```

### Recursive directory

```sh
decomment -r src/
```

### Dry-run (show what would change, don't modify files)

```sh
decomment --dry-run src/main.c
```

### Check mode (exit 3 if any file would be modified)

```sh
decomment --check -r src/
```

### Create backups before modifying

```sh
decomment --backup -r src/
```

### stdin/stdout filter mode

```sh
cat src/main.c | decomment --stdin --lang c
```

### Force language detection

```sh
decomment --lang javascript config.mjs
```

## CLI options reference

| Option | Description |
|---|---|
| `-r`, `--recursive` | Process directories recursively |
| `--dry-run` | Show changes without modifying files |
| `--check` | Exit with code 3 if any file would be modified |
| `--backup` | Create `.bak` backup before replacing each file |
| `--stdin` | Read from stdin, write to stdout |
| `--lang LANG` | Force language (skip auto-detection) |
| `--include GLOB` | Only process files matching pattern |
| `--exclude GLOB` | Skip files matching pattern |
| `--follow-symlinks` | Follow symbolic links (default: don't follow) |
| `-v`, `--verbose` | Print each file as it is processed |
| `-q`, `--quiet` | Suppress all non-error output |
| `--version` | Print version and exit |
| `-h`, `--help` | Print help and exit |

## Exit codes

| Code | Meaning |
|---|---|
| 0 | Success: all files processed without errors |
| 1 | Operational error: I/O failure, permission denied, etc. |
| 2 | Some files were skipped (unsupported language, binary, etc.) |
| 3 | Check mode: at least one file would be modified |
| 4 | Invalid usage: bad arguments, missing required options |

## Safety guarantees

- **Atomic replacement**: output goes to a temp file, then `rename()` replaces the original. No partial writes.
- **fsync**: the temp file is fsynced before rename to ensure data reaches disk.
- **Backup support**: `--backup` copies the original to `<file>.bak` before replacement.
- **Binary detection**: files with NUL bytes in the first 8 KB are skipped automatically.
- **Symlink policy**: symbolic links are not followed by default. Use `--follow-symlinks` to opt in.
- **Temp file placement**: the temp file is created in the same directory as the target to guarantee same-filesystem rename.

## Performance

decomment is single-threaded by design. This ensures deterministic output and avoids write races when processing directory trees. It is efficient enough for most codebases -- processing speed is typically limited by I/O rather than CPU.

## Known limitations

- PHP heredocs, Ruby heredocs, and Perl complex quoting are not handled safely.
- Nested block comments (e.g., Haskell `{- {- -} -}`) are not supported.
- Files larger than 256 MB are skipped.
- Raw strings in C++ (`R"delim(...)delim"`) with comment-like content may not be handled in all cases.
- No multithreading -- large monorepos with millions of files may take a while.

