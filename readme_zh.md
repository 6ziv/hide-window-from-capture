# Hide Window From Capture

For English version, go [here](readme_en.md)

从截图或推流中、任务栏中，或者任务切换窗口(alt-tab)中，隐藏某些窗口。

### 功能

* 通过 `--hide-preview` 选项，从截图或推流中隐藏窗口

* 通过 `--hide-taskbar` 选项，从任务栏或者任务切换窗口中隐藏窗口，用静态图片来替换缩略图或预览图，或者之后通过命令控制窗口图标是否在任务栏上出现。

* Hook了CreateProcess 函数族，因此在程序创建子进程时，子进程也会以和父进程同样的配置启动。

### 使用帮助

##### 注入运行中的进程

* `hidewindow_tool inject --pid <target process id>`  (`--pid` = `-p`)

* `hidewindow_tool inject --pname <target process name>`

* `hidewindow_tool inject --grub`

前面两个指令注入所有满足约束的进程，而最后一个指令则允许用户交互式地选择一个窗口对应的进程。

可以同时使用多组选项来选取多个目标，例如：

`hidewindow_tool inject -p 1111 -p 2222 --pname a.exe --pname b.exe --grub`

会注入到PID为1111或2222，进程名为a.exe或b.exe的所有进程，以及用户选择的窗口对应的进程中。

##### 创建新进程

`hidewindow_tool run [--executable=...] [--cmdline=...] [--working-dir=...]`

上面命令中的三个选项会被传递给CreateProcess(Ex)，如果其中有未设置或者为空的选项，则被传递的为NULL.

可以参阅MSDN上相关内容来设置这三个选项。例如：

`hidewindow_tool run --cmdline cmd` 会启动一个新的命令行窗口。

（cmd一类的命令行程序无法被detour，但是由cmd打开的程序会被注入。）

上述命令(`inject`和`run`)有一些公用的选项

* --hide-preview [on|off]: 是否用静态图片替换窗口缩略图和预览图。
  
  * --preview <path-to-an-image>: 替换预览图的图片。
  
  * --preview-scaling [none|repeat|stretch|keepratio]: 预览图的缩放模式。
  
  * --preview-background: 预览图的背景色。理论上支持CSS-4草案中提到的color functions，但是我个人建议使用这个程序开发过程中测试过的named colors（比如“red”）或者 hex notation（形如：“#FFFFFFFF”）
  
  * --thumbnail <path-to-an-image>: 类似 `--preview`, 替换缩略图的图片。
  
  * --thumbnail-scaling: [repeat|stretch|keepratio]: 缩略图的缩放模式。因为缩略图有尺寸要求，所以不支持`none` 。
  
  * --thumbnail-background: 缩略图背景色。建议用完全不透明的颜色，因为缩略图时常不支持透明或半透明，而系统会忽略我们提供的混叠后的bitmap的透明度通道。

* --hide-taskbar [on|off]: 是否从任务栏上隐藏窗口。如果设置为`on`，我们稍后仍然可以通过命令进行调整。
  
  ###### 缩放模式:
  
  * none: 不缩放。只能用于预览图。
  
  * repeat: 如果原图片比目标区域大，则裁剪；否则，重复原图片直到填满目标区域。
  
  * stretch: 将原图片缩放至目标区域大小。
  
  * keepratio: 和 `stretch` 类似，但是缩放过程中保持图片纵横比。如果不能匹配，则在目标区域两边留有透明。

##### 控制任务栏图片是否显示

- `hidewindow_tool taskbar list`: 列出可以控制的窗口。

- `hidewindow_tool taskbar show|hide ...`:显示或者隐藏匹配条件的窗口
  
  用来匹配窗口的参数可以是:
  
  - `--pname,--pid` 按进程名或进程id匹配窗口。
  
  - `--title` 按窗口标题匹配窗口。
  
  - `--hwnd` 按窗口HWND匹配窗口。
  
  - `--all` 所有可以控制的窗口。如果设置了这个选项，则上面的四个选项不能被设置。

- `hidewindow_tool taskbar interactive`: 交互式调整任务栏图标可见性。

此外，还有一个位于`scripts/createShortcutForQQ.ps1`的PowerShell脚本，演示了利用PowerShell提供的功能，如何创建一个快捷方式，以用来通过此程序运行系统中安装的程序。

### 构建

建议使用Visual Studio进行构建。如果需要从脚本调用，可以参考仓库中包含的Github Workflow。

如果系统上安装了vcpkg，可以执行`vcpkg install --triplet=x64-windows-static @.vcpkg_deps.txt` 来安装进行64位构建所需的库；如果要构建32位程序，则用`x86-windows-static`替代`x64-windows-static`。

一部分库总是会在32位和64位，以静态库、静态CRT的配置安装。如果需要调整这部分配置，请手动安装依赖或者修改`.vcpkg_deps.txt`，之后再通过Visual Studio更新项目设置。（而且可能还要费不少功夫来调整Detours库）。

代码中也包含用来构建的xmake.lua，位于`xmake/xmake.lua`。在`xmake`目录下运行`xmake`以进行构建。即使是使用xmake进行构建，也同样只支持msvc。

### 许可协议

MIT
