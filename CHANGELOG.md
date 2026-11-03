# Changelog

## 0.2.0

- Remove the GNOME Terminal package dependency.
- Prefer the configured XDG terminal; adapt known terminals to request a separate window.
- Fall back to the system alternative and supported installed terminals.
- Add Alacritty, Kitty, Konsole, Ptyxis, WezTerm, Foot, Xterm, Tilix and Xfce adapters.
- Keep the terminal's background and use green foreground text.
- Implement a 500 ms software cursor visibility toggle while waiting for input.
- Isolate native terminal input from the Qt interface and JSON storage.
- Sanitize known Snap-derived GUI environment settings when detected.
- Add --here for existing-terminal/SSH use.
- Preserve aliases.json and setup-v1 compatibility with 0.1.0.
