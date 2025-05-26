# Waybro

**Waybro** is a simple status bar designed for [Wayland](https://wayland.freedesktop.org/) compositors — currently supporting **Hyprland** only. It displays common system status info such as battery level, volume, and more.

The name "Waybro" comes from combining "Wayland" and "bar", with a dash of personality.

Waybro uses **Cairo** for rendering and is intended to be minimal, lightweight, and eventually self-configurable.

> ⚠️ This project is still in early development. Expect bugs, missing features, and rough edges.

---

## ✨ Features

- [x] Battery status via
- [x] Volume level
- [x] Support for Hyprland
- [x] MPD Status
- [x] Workspace
- [x] And other status, either via inotify, unix socket, or forking existing tools
- [ ] Configuration file support *(planned)*
- [ ] Multi-compositor support *(planned)*
- [ ] Module and Plugin support *(planned)*

---

## 🔧 Installation

### Requirements

- A Wayland compositor (currently only tested on **Hyprland**)
- `cairo`
- `cmake`
- C compiler (e.g., `gcc`, `clang`)

### Build

```bash
git clone https://github.com/yourname/Waybro.git
cd Waybro
cmake -B build
cmake --build build
```

## 📜 License
MIT License — see the [LICENSE](https://github.com/RedenHilal/Waybro?tab=MIT-1-ov-file#) file for details.
