# Alias Manager

A green, terminal-based Bash alias manager built with C++17 and Qt 6 Core.
Version 0.2.0. License: MIT.

## Platform scope and status

Targets: Ubuntu Desktop 24.04 LTS and 26.04 LTS, amd64.
These are target platforms, not a claim that this source archive has completed
release validation on both systems. Fedora, Arch, ARM and other shells are future work.
See VALIDATION.md for checks actually performed.

## Features

- List, add, edit, rename and delete Bash aliases.
- Green text, with the background supplied by your terminal profile.
- A vertical cursor whose visibility toggles every 500 ms while waiting for input.
- An existing terminal emulator, not an additional mandatory terminal package.
- A separate terminal window for supported launch adapters.
- Opt-in automatic Bash integration, a .bashrc backup and no manual source command.
- Atomic JSON replacement, update locking, duplicate validation and safe Bash quoting.
- No network access or command execution by the menu.

## Install a binary release

Download the amd64 .deb for your Ubuntu release and install it with APT:

```bash
sudo apt install ./alias-manager_0.2.0_amd64.deb
```

Runtime libraries are resolved by APT. No compiler is needed to use a binary release.
A terminal emulator must already be installed; the package does not install one.

## Launch and setup

```bash
alias-manager
```

Do not run the manager with sudo. A separate terminal window opens. On first use,
accept automatic Bash integration if desired. Setup backs up ~/.bashrc, inserts
one marked, guarded source block and writes a setup marker.
Reopen existing Bash terminals once after setup. Future changes are synchronized
before the next Bash prompt, with no manual source command.

Declining leaves integration disabled and asks again next time. Records can still
be managed. Symlinked .bashrc files are not automatically rewritten.

For SSH, recovery or a terminal that has no supported launcher adapter:

```bash
alias-manager --here
```

This deliberately uses the current terminal rather than opening another window.

## Terminal selection

1. An optional ALIAS_MANAGER_TERMINAL override, if set to a supported executable.
2. The configured XDG terminal, discovered with xdg-terminal-exec if available.
3. The system x-terminal-emulator alternative if it resolves to a supported adapter.
4. Installed supported terminals, in the launcher's documented source order.

Adapters: Ptyxis, GNOME Terminal, Alacritty, Kitty, Konsole, Xfce Terminal,
Tilix, Foot, WezTerm, Xterm and UXTerm. Known XDG terminal choices are routed
through their adapter to request a separate window/process. Other XDG terminal
entries are delegated to xdg-terminal-exec, whose entry controls window behavior.
Not every installed/custom terminal is automatically supported.

Your terminal preference is not changed. If no supported terminal is found, the
launcher explains the problem and offers --here; it does not install anything.
For a one-off explicit choice, for example:

```bash
ALIAS_MANAGER_TERMINAL=alacritty alias-manager
```

The override is an executable name/path, not a shell command with arguments.
The launcher does not parse it with eval.

The command in the new window is the manager session, not a permanent interactive
shell. Normal completion closes the window in normal terminal configurations;
a user-configured hold-on-exit behavior can keep it open. Session errors pause
for acknowledgement so their messages are readable.

## Cursor and colors

The application emits green foreground colors only and does not set the terminal
background or change its theme. The palette controls the exact green shade.
The cursor uses a steady-bar request plus application-driven hide/show toggling,
instead of relying on the emulator's native cursor blink preference.
Terminals must honor cursor-shape/visibility control sequences for this to work.
No terminal profile or system accessibility setting is modified.

## Menu

| Input | Action |
| --- | --- |
| 1 | Add an alias |
| 2 | Edit or rename, selected by list number |
| 3 | Delete, with confirmation |
| 4 | Reload |
| 0 or q | Exit |

Press Enter after each choice. Empty name cancels adding; empty values during
editing retain current values. Ctrl+D ends input.

Example:

```text
Alias   > la
Command > ls -al
```

Do not wrap the complete command in extra shell quotes. Quotes that are part of
the command itself should remain, e.g. `printf '%s\n' "hello"`.
Names start with an ASCII letter or underscore and contain only ASCII letters,
digits, underscores and hyphens. Invalid or duplicate names are rejected.
Commands are stored as text and execute only when their aliases are invoked.

## Integration behavior

The hook requests an export before each Bash prompt and reapplies it only when
the exported definitions change. Already idle terminals refresh after Enter or
the next completed command, not asynchronously. A terminal waiting for the
manager process may refresh only after the manager exits.
The manager takes priority for conflicting names during a synchronization.
Unrelated aliases are preserved. Pre-existing replaced aliases are restored when
possible when their managed replacements are removed. Manual shell changes do
not edit JSON and may be overwritten by a subsequent managed update.

Existing PROMPT_COMMAND handlers are retained. Export errors leave shell aliases
unchanged. The hook currently runs a small backend process before every prompt;
more efficient change detection is planned.

## Snap VS Code launch environment

When known Snap-derived GUI paths are detected, the launcher removes selected
Snap GTK/library overrides for its child process and uses standard Ubuntu data
and configuration search directories, including Snap/Flatpak desktop entries.
It does not change the parent terminal, .bashrc, VS Code or system schemas.
This is a targeted compatibility workaround, not universal Snap compatibility.
Launching from a normal desktop terminal is a useful diagnostic comparison.

## Storage and migration

Data directory: `${XDG_CONFIG_HOME:-$HOME/.config}/alias-manager`.
Files: aliases.json, aliases.lock while updating, and setup-v1 after consent.
The 0.1.0 JSON format and setup marker are preserved. Upgrading does not delete records.
Backups: `~/.bashrc.alias-manager-backup.<timestamp>.<pid>`.
Corrupt or invalid JSON is rejected rather than overwritten.

For prototype users, check `type -a alias-manager`: an old executable under
~/.local/bin may shadow /usr/bin/alias-manager. Preserve a backup, then rename
that old executable if necessary. Marked prototype integration blocks are
replaced only during consented setup. Restart existing shells after migration.

## Build from source

Use a clean folder and build on the Ubuntu release being targeted. A binary built
on 26.04 is not assumed compatible with 24.04.

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev dpkg-dev file python3
bash scripts/build-deb.sh
```

The script configures Release mode, builds, runs CTest, then creates a package
with CPack and dpkg-shlibdeps. The resulting package is in build-release.
When Python is available, CTest also runs isolated shell checks and PTY input checks.

```bash
dpkg-deb --info build-release/alias-manager_0.2.0_amd64.deb
dpkg-deb --contents build-release/alias-manager_0.2.0_amd64.deb
sudo apt install ./build-release/alias-manager_0.2.0_amd64.deb
/usr/bin/alias-manager
```

The package installs a launcher in /usr/bin, a backend/session in
/usr/lib/alias-manager, and the hook in /usr/share/alias-manager.
Package installation does not itself edit any user's home directory.

## Validation before publication

On both clean target desktops, verify installation/dependencies, launch from
the system terminal and Snap VS Code, separate-window behavior, default
terminal preference, blinking while waiting/typing, system background, setup
backup/idempotence, add/edit/delete, quoting, cross-terminal refresh, conflict
restoration, restart persistence, invalid/corrupt input, and removal.
Test each terminal adapter on a real installed emulator before advertising it
as verified; mock argument tests are not a substitute for desktop testing.

## Clean up after upgrading from 0.1.0

First install 0.2.0 and verify that the terminal you want opens correctly.
Only if the extra GNOME Terminal installed for 0.1.0 is not wanted:

```bash
sudo apt remove --dry-run gnome-terminal gnome-terminal-data
```

Review the proposed removal. If it contains only the expected packages:

```bash
sudo apt remove gnome-terminal gnome-terminal-data
```

Then review unused dependencies before removing them:

```bash
sudo apt autoremove --dry-run
sudo apt autoremove
```

Do not accept unexpected removals. Optional package cache cleanup:

```bash
sudo apt clean
```

This does not delete your project's local .deb files or personal alias records.
Keep a working source backup; remove old downloaded ZIPs/build directories only
when no longer needed. Do not delete all of ~/.cache or remove Snap core20 to
work around a launch-environment conflict.

## Remove the manager

```bash
sudo apt remove alias-manager
```

Records and Bash backups are deliberately retained. The guarded .bashrc block is
inert while the hook is absent. Aliases already loaded into a running shell may
remain until that shell closes. Back up records before deliberately removing
the configuration directory yourself.

## Roadmap

Fedora/Arch packages, additional terminal validation, more architectures,
more shells and cheaper Bash synchronization.
