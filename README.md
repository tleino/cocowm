# cocowm

*cocowm* is a Column Commander Window Manager for X11 Window System that
specializes in automating layout management by arranging program windows
automatically in columns. Therefore, *cocowm* is a tiling window manager
and in particular is most inspired by the Plan9 acme editor, as well as
the Plan B operating system's GUI.

A key differentiation of *cocowm* as compared to other window managers is
that *cocowm* is oriented towards receiving high-level commands from the
user, such as "move window to left", via a keyboard shortcut, which
moves a window one column to the left and resizes if space allows for
it, rather than by manually micro-managing precise placement and size
using a mouse.

(This window manager is also meant to be used together with a specialized
terminal emulator that by default uses only as little space as
necessary, and grows as needed, making it possible to have output of
shell commands in their own short-lived windows rather than accumulating
an endless scrollback.)

## Default key bindings

### Focusing windows

* **Win+Left** Left.
* **Win+Right** Right.
* **Win+Up** Up.
* **Win+Down** Down.
* **Win+Tab** Previous.

### Moving the focused window

* **Ctrl+Win+Left** Left.
* **Ctrl+Win+Right** Right.
* **Ctrl+Win+Up** Up.
* **Ctrl+Win+Down** Down.

### Actions for the focused window

* **Win+o** Toggle keep open.
* **Win+m** Toggle minimize.
* **Win+f** Toggle fullscreen.
* **Win+r** Restart command.
* **Win+q** Close.

### Managing the window manager.

* **F11** Restart.
* **F12** Quit.
