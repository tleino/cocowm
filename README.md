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

## Controls

*cocowm* is fully controllable using keyboard and partially controllable
using mouse.

Edit keyboard.c to modify default key bindings.

*cocowm* is used best when windows are explicitly created by the user
using *cocowm command line* via **Win+Enter** or **Win+Space**.

So, for example, don't ask your browser to create a new window,
but ask *cocowm* to do it.

*cocowm* tries to help in selecting the correct program using the
following magic:

### Running commands

Open the prompt via **Win+Enter** or **Win+Space**. Exit the prompt
using **Esc** (cancel) or **Enter** (launch).

* **Run URLs:** If no spaces but has at least one '.', prefix the line
with "firefox-esr ".
* **Run shell commands:** If the line begins with '!', prefix the line
with "xterm -hold -e "

Edit action.c to modify these.

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
* **Win+h** Toggle hide others.
* **Win+f** Toggle fullscreen.
* **Win+r** Restart command.
* **Win+Space** Replace with new command.
* **Win+Enter** Run new command.
* **Win+q** Close.

_Minimize_ means window is explicitly hidden and needs to be reopened manually.

_Hide_ means window is temporarily hidden and is shown automatically
when hiding is toggled off.

### Managing the window manager.

* **F11** Restart.
* **F12** Quit.

## Dependencies

* No dependencies if you're compiling for a normal/sane Unix-like system that
has X11 Window System.

## Compiling

	$ ./configure ~
	$ make
	$ make install

## Customizing

* Edit Makefile.in and e.g. remove or add compile-time options using -D in
the CFLAGS. E.g. see 'WANT_ONE_PER_COLUMN' option.
* Edit files such as keyboard.c

## Running

Running with 3 columns:

	$ cocowm 3

## See also

* [mxswm](https://github.com/tleino/mxswm) another window manager by the same
author.
* [cocovt](https://github.com/tleino/cocovt) terminal emulator by the same
author that is friends with *cocowm*.
