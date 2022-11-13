# Hide Window From Capture

如果需要中文版帮助文档，请移步[此处](readme_zh.md)

To hide a window from screen capture, taskbar and task-switcher.

### Features

* Hide a window from screen capture with `--hide-preview`

* Hide a window from taskbar and task-switch with `--hide-taskbar`. We can protect thumbnail and preview with a static image, or control whether the window appears on the taskbar later.

* CreateProcess api family is detoured, so child processes are automatically started with the same configuration as the parent.

### Usage

##### Inject into running processes

* `hidewindow_tool inject --pid <target process id>`  (`--pid` = `-p`)

* `hidewindow_tool inject --pname <target process name>`

* `hidewindow_tool inject --grub`

The first two syntaxes injects into all processes matching given restrictions.

And the last one allows user to select a running process by window interactively.

We may use multiple options to select multiple targets. For example:

`hidewindow_tool inject -p 1111 -p 2222 --pname a.exe --pname b.exe --grub`

will inject into processes with pid 1111 or 2222, **or** with pname a.exe or b.exe, adding the process selected with user interaction.

##### Start new processes

`hidewindow_tool run [--executable=...] [--cmdline=...] [--working-dir=...]`

These three options will be passed as parameters of `CreateProcess(Ex)`, and if any of these options is empty or not specified, NULL will be used.

Follow the Microsoft Doc on how to set them. For example:

`hidewindow_tool run --cmdline cmd` will start a new console for you.

(Well, of course we cannot 'protect' a console application like 'cmd.exe', but its children process will be injected.)

For above two subcommands (`inject` and `run`), they share some common options:

* --hide-preview [on|off]: whether to protect window preview with static images (in fact it contains 'thumbnail' and 'preview').
  
  * --preview <path-to-an-image>: specify the image to use for preview.
  
  * --preview-scaling [none|repeat|stretch|keepratio]: specify scaling mode for preview. 
  
  * --preview-background: specify a background color for preview. In theory color functions in css-4 draft should be supported, but personally I suggest using named colors (like 'red') or hex notations (like `#FFFFFFFF`), as they are better tested during development.
  
  * --thumbnail <path-to-an-image>: similar to `--preview`, but used for thumbnails
  
  * --thumbnail-scaling: [repeat|stretch|keepratio]: scaling mode for thumbnails. `none` is not supported, as thumbnails are fixed sized.
  
  * --thumbnail-background: background color for thumbnails. Fully opaque colors are suggested, as thumbnails usually cannot be transparent, and the system seems to ignore alpha channel in the mixed bitmap.

* --hide-taskbar [on|off]: whether to hide window from taskbar. When this option is set to `on`, we can later disable or enable it from command line.
  
  ###### Scaling modes:
  
  * none: do not resize. Can only be used with preview images.
  
  * repeat: if source image is larger than target region, crop to target; otherwise, repeat the source image to fill the target region.
  
  * stretch: resize source image to fit target region. 
  
  * keepratio: similar to `stretch`, but leaving transparent borders in target region and keep image aspect ratio.

##### Control taskbar visibilities

- `hidewindow_tool taskbar list`: list all controllable windows.

- `hidewindow_tool taskbar show|hide ...`:show or hide the matching windows.
  
  Arguments may contain:
  
  - `--pname,--pid` select windows in processes with given names or pids.
  
  - `--title` select windows with given titles.
  
  - `--hwnd` select windows with given HWNDs.
  
  - `--all` All controllable windows. When this is set, the above four options cannot be set.

- `hidewindow_tool taskbar interactive`: start an interactive console for controling taskbar icon visibility.

Also, there is a powershell script file  `scripts/createShortcutForQQ.ps1` which demonstrates how to create a shortcut for starting installed programs with this application.

### Building

The recommended way of building this project is with Visual Studio. To integrate the build process in scripts, refer to the automated github workflow.

If you have vcpkg installed, running `vcpkg install --triplet=x64-windows-static @.vcpkg_deps.txt`  will install the dependent libraries (to build a 32-bit binary, change `x64-windows-static`to`x86-windows-static`).

Some libraries will be installed in both 32-bit and 64-bit, and always as static libraries with static CRT. To build the program in other configurations, manually install the dependencies or modify `.vcpkg_deps.txt`, and then change the project setting in Visual Studio. (But I'm afraid it will be a lot of work to make Detours work.)

A xmake.lua is also included in the source tree as `xmake/xmake.lua`. Build it with `xmake` as working directory. Only msvc is supported.

### License

MIT
