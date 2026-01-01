# Implementation Plan: help2man Integration

Integrate `help2man` to automatically generate manual pages for CBSD commands from their `--help` and `--version` output.

## Proposed Changes

### Build System

#### [MODIFY] [Makefile](file:///home/dsewell/Projects/cbsd/Makefile)
- Add a check for `help2man` availability.
- Add a `doc` or `man` target to generate manual pages.
- Integrate the generation of `cbsd.1` (or update `cbsd.8`) using `help2man`.
- Use `help2man` options to customize the output (e.g., `--name`, `--section`).

### Example Integration in Makefile:

```makefile
HELP2MAN = help2man
HELP2MAN_OPTS = --no-info --section=8 --name="CBSD jail management tool"

manpages: cbsd
	${HELP2MAN} ${HELP2MAN_OPTS} ./bin/cbsdsh/cbsd -o man/cbsd.8.generated
```

> [!NOTE]
> Since `cbsd` relies on many environment variables and its `workdir`, `help2man` might need a wrapper or environment setup to run correctly during build.

## Verification Plan

### Automated Tests
- Build the project and run the new `manpages` target.
- Check if `man/cbsd.8.generated` is created and contains valid nroff content.
- Verify that `man/cbsd.8.generated` correctly captures the output of `cbsd --help`.

### Manual Verification
- Run `man ./man/cbsd.8.generated` to see how it looks in a pager.
- Compare the generated manual page with the existing `man/cbsd.8` to ensure no regression in critical information.
