# MiniShell

MiniShell is a school project that implements a simplified Unix-like shell in C. It reads commands from the user, parses them, supports pipelines, redirections, background jobs, and executes programs using the system process API.

## Project goals

The project is designed to explore core shell concepts such as:

- command parsing
- process creation with fork/exec
- standard input/output redirection
- pipelines between commands
- signal handling
- background execution

## Repository structure

The source code is located in the `minishell/` folder:

- `minishell/minishell.c` — main shell loop and command execution logic
- `minishell/readcmd.c` — command-line parsing logic
- `minishell/readcmd.h` — command structure definitions
- `minishell/Makefile` — build configuration
- `minishell/test_readcmd.c` — parsing test utility
- `minishell/input.txt` — sample input file

## Features

This shell supports:

- basic command execution (`execvp`)
- pipelines using `|`
- input redirection using `<`
- output redirection using `>`
- background jobs using `&`
- exit command to quit the shell
- handling of `SIGINT`, `SIGTSTP`, and `SIGCHLD`

## Build

From the repository root:

```bash
cd minishell
make
```

This builds the executable:

```bash
./minishell
```

## Example usage

```bash
> ls -la
> echo hello world
> ls | grep MiniShell
> cat < input.txt > output.txt
> sleep 5 &
> exit
```

## Clean build artifacts

```bash
make clean
```

## Notes

This is a minimal educational shell and is not intended to fully reproduce every behavior of a production shell such as Bash or Zsh. It focuses on understanding how command interpretation and process management work at a lower level.

## License

This project is provided as a learning exercise and does not appear to include a formal license file.
