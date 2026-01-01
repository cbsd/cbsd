# CBSD Documentation TODO List

This list is based on the codebase analysis and identifies areas for improving the CBSD documentation.

## Manual Pages
- [ ] Create/Expand man pages for commands in `jailctl/`
- [ ] Create/Expand man pages for commands in `bhyvectl/`
- [ ] Ensure all CLI tools have a corresponding `.8` man page

## Configuration Reference
- [ ] Create a centralized reference guide for files in `etc/defaults/`
- [ ] Document common variables used across different `vm-linux-*` templates
- [ ] Improve documentation for `global.conf`

## Workflow and SSI Updates
- [ ] Audit `share/docs/jail/` fragments and verify consistency with `jailctl/` scripts
- [ ] Audit `share/docs/bhyve/` fragments and verify consistency with `bhyvectl/` scripts
- [ ] Update `wf_*_ssi.md` files where logic has diverged from implementation

## Module Documentation
- [ ] Review documentation for modules in `modules/` (e.g., `pkg.d`, `zfsinstall.d`)
- [ ] Ensure new modules are added to the documentation fragments in `share/docs/`

## General Improvements
- [ ] Review and update platform-specific READMEs (`README.DragonflyBSD.md`, etc.)
- [ ] Ensure the main `README.md` reflects the latest project mission and goals
