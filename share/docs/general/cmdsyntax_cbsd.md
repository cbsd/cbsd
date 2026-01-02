# CBSD Command Syntax

All CBSD commands follow a consistent prefix and argument structure. Unless you are using the [CBSD CLI](https://www.bsdstore.ru/en/cbsdsh.html), every command must begin with the `cbsd` prefix and be executed with **root** privileges.

### Basic Examples
```sh
cbsd jls              # List jails
cbsd jstop myjail     # Stop a jail by name
cbsd jstart myjail    # Start a jail by name
cbsd node mode=list   # List cluster nodes
```

## Command Arguments

CBSD commands use a `param=value` format.
- Arguments can be **required** or **optional**.
- The **order** of arguments does not matter.
- Complex values containing spaces should be quoted.

---

## Getting Help

To discover the available arguments and requirements for any command, use the `--help` flag:

```sh
cbsd <command> --help
```

### Understanding Help Output
When you run `cbsd jls --help`, you will see output similar to this:

```text
[jail] List jail and status
require:
opt: alljails shownode display node header
alljails=1 - get jaillist from remote node
shownode=1 - show nodename for jails
node= only for current node
header=0 don't print header
display= list by comma for column. Default: jid,jname,ip4_addr,host_hostname,path,status
External help: https://www.bsdstore.ru/en/wf_jls_en.html
```

- **Module Name**: Shown in square brackets (e.g., `[jail]`).
- **require**: Lists arguments that must be provided for the command to function.
- **opt**: Lists optional parameters that modify the command's behavior.
- **External Help**: Provides a link to the full online documentation for that specific subcommand.

---

## Global Parameters & Environment Overrides

### Scripting & Automation (`inter=0`)
By default, some CBSD commands are interactive (e.g., asking to download a base image if it's missing). To use CBSD in non-interactive scripts, append `inter=0` to ensure the command uses default decisions instead of prompting the user:

```sh
cbsd jstart inter=0 jname=myjail
cbsd jcreate inter=0 jconf=/path/to/jail.conf
```

### Visual Feedback (`NOCOLOR=1`)
CBSD produces colorized console output by default. To disable color for easier parsing in your own scripts or logs, use the `NOCOLOR` environment variable:

```sh
env NOCOLOR=1 cbsd jls
```

### Troubleshooting (`CBSD_DEBUG=1`)
If a command isn't behaving as expected, you can enable full execution tracing (via `sh xtrace`) to see every operation performed by the script:

```sh
env CBSD_DEBUG=1 cbsd jls
```

---

## Machine-Readable Output

For advanced automation and integration with other tools, most CBSD listing commands can output data in machine-parsable formats. Use the `display` parameter to choose your preferred format:

```sh
cbsd jls display=json
cbsd jls display=xml
cbsd jls display=html
```

*Note: Use these formats when building GUIs or custom dashboards that need to scrape CBSD state.*

---
Copyright © 2013—2025 CBSD Team.
