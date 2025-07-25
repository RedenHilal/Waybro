# Waybro

**Waybro** is a simple status bar designed for [Wayland](https://wayland.freedesktop.org/) compositors — currently supporting **Hyprland** only. It displays common system status info such as battery level, volume, and more.

>  This project is still in early development. Expect bugs, missing features, and rough edges.

>  Contribution, bug reports, and suggestion are highly appreciated.


---

##  Features

- [x] Battery status via
- [x] Volume level
- [x] MPD Status
- [x] Workspace
- [x] And other status, either via inotify, socket ipc, or forking existing tools
- [x] Configuration file support 
- [ ] Multi-compositor support *(planned)*
- [ ] Module and Plugin support *(planned)*

---

##  Installation

### Requirements

- A Wayland compositor (currently only support for **Hyprland**)
- `cairo`
- `cmake`
- C compiler (e.g., `gcc`, `clang`)

### Build

```bash
git clone https://github.com/yourname/Waybro.git
cd Waybro/build
cmake ..
make
```

##  License
MIT License — see the [LICENSE](https://github.com/RedenHilal/Waybro?tab=MIT-1-ov-file#) file for details.
