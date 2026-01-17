# Waybro

**Waybro** is a simple status bar designed for [Wayland](https://wayland.freedesktop.org/) compositors. This bar is made with module-based architecture, so you can create you own module using defined interface.

>  This project is still in early development. Expect bugs, missing features, and undefined behaviours.

>  Contribution, bug reports, and suggestion are highly appreciated.


---

##  Features

-  Customizable module 
-  Prebuild modules, such as pulseaudio, libnl, and more
-  Simple interfaces for developing module
-  Flexible module
-  Configuration file support 

---

##  Installation

### Requirements

- A Wayland compositor (currently only support for **Hyprland**)
- `pango-cairo`
- `cairo`
- `cmake`
- `gcc`
- `libnl`
- `libpulse`

### Build

```bash
git clone https://github.com/RedenHilal/Waybro.git
cd Waybro
chmod +x install.sh
./install.sh release
```

## Developing

### Module

Developing a module can be done by creating a your module directory in src/module, put source source file and put CMakeLists.txt there—with it linked to waybro-api interface. For more information, see include/module.h and existing prebuilt module for reference :3

##  License
MIT License — see the [LICENSE](https://github.com/RedenHilal/Waybro?tab=MIT-1-ov-file#) file for details.
