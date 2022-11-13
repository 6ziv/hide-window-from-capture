set_xmakever("2.2.5")
add_rules("mode.release", "mode.debug")
set_languages("c17","cxx20")
rule("install-without-prefix")
	on_install(function (target)
		os.cp(target:targetfile(), target:installdir())
	end)
rule_end()
if is_mode("release") then
	set_runtimes("MT")
else
	set_runtimes("MTd")
end
proj_real_root = "$(projectdir)/../Src"

for _, arch in ipairs({{"x86","32"}, {"x64","64"}}) do
	add_requires(
		"vcpkg::termcolor~" .. arch[1],
		"vcpkg::detours~" .. arch[1],
		"vcpkg::boost-program-options~" .. arch[1],
		"vcpkg::boost-locale~" .. arch[1],
		"vcpkg::boost-scope-exit~" .. arch[1],
		"vcpkg::boost-algorithm~" .. arch[1],
		"vcpkg::boost-filesystem~" .. arch[1],
		"vcpkg::boost-lexical-cast~" .. arch[1],
		"vcpkg::boost-dll~" .. arch[1],
		"vcpkg::simde~" .. arch[1],
		{configs={shared = false},arch=arch[1]})

	target("hide_window_helper" .. arch[2])
		add_rules("install-without-prefix")
		set_targetdir("$(buildir)")
		add_installfiles("$(buildir)/hide_window_helper" .. arch[2] .. ".dll")
		set_kind("shared")
		set_optimize("fast")
		set_warnings("allextra")
		set_runtimes(vs_runtime)
		if is_mode("debug") then
			add_defines("_DEBUG=1")
		end
		set_arch(arch[1])
		set_toolchains("msvc", {plat = "windows", arch = arch[1]})
		sources = {
			proj_real_root .. "/HookDll/api_hook.c",
			proj_real_root .. "/HookDll/config.c",
			proj_real_root .. "/HookDll/dllmain.c",
			proj_real_root .. "/HookDll/hide_preview_content.c",
			proj_real_root .. "/HookDll/hiding_from_taskbar.c",
			proj_real_root .. "/HookDll/image.c",
			proj_real_root .. "/HookDll/thread_window_walker.c",
			proj_real_root .. "/HookDll/win_event_hook.c",
			proj_real_root .. "/HookDll/window_subclass.c"
		}
		headers = {
			proj_real_root .. "/HookDll/api_hook.h",
			proj_real_root .. "/HookDll/config.h",
			proj_real_root .. "/HookDll/guid.h",
			proj_real_root .. "/HookDll/hide_preview_content.h",
			proj_real_root .. "/HookDll/hiding_from_taskbar.h",
			proj_real_root .. "/HookDll/image.h",
			proj_real_root .. "/HookDll/thread_window_walker.h",
			proj_real_root .. "/HookDll/win_event_hook.h",
			proj_real_root .. "/HookDll/window_subclass.h"
		}
		add_files(sources,proj_real_root .. "/HookDll/HookDll.def")
		add_headerfiles(headers)
		add_packages("vcpkg::detours~" .. arch[1])
		add_packages("vcpkg::simde~" .. arch[1])
		add_links("user32","advapi32","gdi32")
		
	target_end()
	
	target("inject_helper" .. arch[2])
		add_rules("install-without-prefix")
		set_targetdir("$(buildir)")
		set_kind("binary")
		set_optimize("fast")
		set_warnings("more")
		set_arch(arch[1])
		set_toolchains("msvc", {plat = "windows", arch = arch[1]})
		sources = {
			proj_real_root .. "/inject_helper/inject_helper.cpp"
		}
		headers = {}
		add_files(sources)
		add_headerfiles(headers)
		add_packages("vcpkg::detours~" .. arch[1])
		add_packages("vcpkg::boost-dll~" .. arch[1])
		
	target_end()
end	
set_arch(os.arch())
add_requires("vcpkg::stb","vcpkg::nlohmann-json","vcpkg::gcem",{configs={shared = false},arch=os.arch()})
target("hidewindow_tool")
	add_rules("install-without-prefix")
	set_targetdir("$(buildir)")
	-- add_installfiles("$(buildir)/hidewindow_tool" .. arch[2] .. ".exe")
	set_kind("binary")
	set_optimize("fast")
	set_warnings("more")
	set_toolchains("msvc", {plat = "windows", arch = os.arch()})
	sources = {
		proj_real_root .. "/ConsoleApp/ConsoleApp.cpp"
	}
	headers = {
		proj_real_root .. "/ConsoleApp/config.hpp",
		proj_real_root .. "/ConsoleApp/detect_target_bitness.hpp",
		proj_real_root .. "/ConsoleApp/environment.hpp",
		proj_real_root .. "/ConsoleApp/exceptions.hpp",
		proj_real_root .. "/ConsoleApp/interactive_shell.hpp",
		proj_real_root .. "/ConsoleApp/load_image.hpp",
		proj_real_root .. "/ConsoleApp/parse_arguments.hpp",
		proj_real_root .. "/ConsoleApp/subcommands.hpp",
		proj_real_root .. "/ConsoleApp/targets.hpp"
	}
	add_files(sources)
	add_headerfiles(headers)
	add_includedirs(proj_real_root .. "/../css-color-cpp")
	add_packages(
		"vcpkg::detours~" .. os.arch(),
		"vcpkg::boost-program-options~" .. os.arch(),
		"vcpkg::boost-locale~" .. os.arch(),
		"vcpkg::boost-filesystem~" .. os.arch(),
		"vcpkg::stb",
		"vcpkg::nlohmann-json"
		)
	add_links("user32","advapi32","gdi32")
target_end()
target("conf")
	set_kind("phony")
	set_configdir("$(buildir)/conf")
	add_configfiles(proj_real_root .. "/etc/conf/*.json",{onlycopy = true})
	add_configfiles(proj_real_root .. "/etc/conf/*.png",{onlycopy = true})
	add_installfiles("$(buildir)/conf/*.*", {prefixdir = "conf"})
target_end()
target("scripts")
	set_kind("phony")
	set_configdir("$(buildir)/scripts")
	add_configfiles(proj_real_root .. "/etc/scripts/*.ps1",{onlycopy = true})
	add_installfiles(proj_real_root .. "/etc/scripts/*.ps1", {prefixdir = "scripts"})
target_end()