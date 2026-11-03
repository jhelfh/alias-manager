# Validation

## Version 0.2.0

### Confirmed on the developer's machine

Platform: Ubuntu 26.04 LTS, amd64.

- Release build completed successfully with GCC 15.2.0 and Qt 6.10.2.
- All three CTest tests passed:
  - storage
  - shell-integration
  - terminal-input
- CPack generated alias-manager_0.2.0_amd64.deb.
- The package was installed successfully using APT.
- The user confirmed that the application works and opens in the
  standard Ubuntu terminal.
- GNOME Terminal was removed without removing Alias Manager.

### Automated test scope

- JSON storage operations and validation.
- Bash integration using an isolated shell and mock backend.
- Launcher arguments and environment handling using mock terminals.
- Cursor visibility control output, UTF-8 input, Backspace and EOF
  using a pseudoterminal.

Mock and pseudoterminal tests do not establish compatibility with
every graphical terminal emulator.

### Not yet verified

- Ubuntu 24.04 LTS.
- Clean-machine installation without development packages.
- Real graphical operation of every supported terminal adapter.
- A complete desktop acceptance checklist across both target releases.

Ubuntu 24.04 LTS remains a target, not a verified platform for this release.
