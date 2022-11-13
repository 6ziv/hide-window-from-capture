#pragma once
#include <map>
#include <functional>
#include <iostream>
#include "config.hpp"
#include "interactive_shell.hpp"
#include <variant>
#include <sstream>
#include <boost/dll/runtime_symbol_info.hpp>
#define HIDE_WINDOW_DEFINE_GUID
#include "../HookDll/guid.h"
extern std::map<std::wstring, std::function<int(const std::wstring&, const std::vector<std::wstring>&)>> actions;
int subcommand_help(const std::wstring& program_name, const std::vector<std::wstring>& verb_arguments = std::vector<std::wstring>())
{
	plain_wcout() << L"Usage:"<< std::endl;
	wcinfo() << program_name << L" <subcommand> [args ...]"<<std::endl<<std::endl;
	plain_wcout() << L"Supported subcommands:" << std::endl;
	for (const auto& action : actions) {
		wcinfo() << action.first << L" , ";
	}
	wcinfo() << std::endl << std::endl;

	plain_wcout() << L"Call " << termcolor::green << std::quoted(program_name + L" <subcommand> --help") << termcolor::reset << "for more detailed help message";
	return 0;
}
HANDLE dup_handle(HANDLE target_process, HANDLE handle_to_dup) {
	HANDLE ret;
	if (handle_to_dup == NULL || handle_to_dup == INVALID_HANDLE_VALUE)return NULL;
	expect_win32(DuplicateHandle(GetCurrentProcess(), handle_to_dup, target_process, &ret, NULL, FALSE, DUPLICATE_SAME_ACCESS));
	return ret;
}
void inject(DWORD pid, const CONFIG* cfg) {
	const std::wstring injector_32 = (boost::dll::program_location().parent_path() / "inject_helper32.exe").wstring();
	const std::wstring injector_64 = (boost::dll::program_location().parent_path() / "inject_helper64.exe").wstring();


	const std::wstring buffer_32 = (boost::dll::program_location().parent_path() / "hide_window_helper32.dll").wstring();
	const std::wstring buffer_64 = (boost::dll::program_location().parent_path() / "hide_window_helper64.dll").wstring();
	
	
	/*
	HMODULE hMod = GetModuleHandle(TEXT("kernel32.dll"));
	expect_win32(hMod);

	FARPROC hLoadLib = GetProcAddress(hMod, "LoadLibraryA");
	expect_win32(hLoadLib);*/
	
	
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	expect_win32(hProcess);

	auto bitness = detect_bitness(hProcess);
	my_assert(bitness == PROCESS_X86 || bitness == PROCESS_X64, "Unknown process bitness");

	const std::wstring buffer = bitness == PROCESS_X64 ? buffer_64 : buffer_32;
	LPVOID rBuffer = VirtualAllocEx(hProcess, NULL, (buffer.length() + 1) * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
	expect_win32(rBuffer);
	WriteProcessMemory(hProcess, rBuffer, buffer.c_str(), (buffer.length() + 1) * sizeof(wchar_t), NULL);

	CONFIG copyed_cfg;
	copyed_cfg.HidePreview = cfg->HidePreview;
	copyed_cfg.HideTaskbar = cfg->HideTaskbar;
	copyed_cfg.icon_mapping = dup_handle(hProcess, cfg->icon_mapping);
	copyed_cfg.preview_mapping = dup_handle(hProcess, cfg->preview_mapping);
	expect_win32(DetourCopyPayloadToProcess(hProcess, HIDE_WINDOW_CONFIG_GUID, reinterpret_cast<PVOID>(&copyed_cfg), sizeof(CONFIG)));
	
	ProcessBitness my_bitness = (sizeof(void*) == 8) ? PROCESS_X64 : PROCESS_X86;
	DWORD retCode;
	if (my_bitness == bitness) {
		HANDLE hThread = CreateRemoteThreadEx(hProcess, NULL, NULL, (PTHREAD_START_ROUTINE)LoadLibraryW, rBuffer, NULL, NULL, NULL);
		expect_win32(hThread);
		WaitForSingleObject(hThread, INFINITE);
		DWORD ec;
		GetExitCodeThread(hThread, &ec);
		retCode = (ec != 0) ? ERROR_SUCCESS : ERROR_DLL_INIT_FAILED;
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, rBuffer, 0, MEM_RELEASE);
	}
	else {
		const std::wstring injector = bitness == PROCESS_X64 ? injector_64 : injector_32;
		
		std::wostringstream oss;
		oss << L"injector" << " " << std::dec << pid << " " << std::hex << reinterpret_cast<uintptr_t>(rBuffer);
		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		PROCESS_INFORMATION pi;
		DWORD t;
		WriteConsoleA(si.hStdOutput, "\033[33m", 5, &t, NULL);
		expect_win32(CreateProcessW(injector.c_str(), oss.str().data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
		//require c++11
		CloseHandle(pi.hThread);
		DWORD wait_result = WaitForSingleObject(pi.hProcess, 5000);
		if (wait_result != WAIT_OBJECT_0) {
			TerminateProcess(pi.hProcess, ERROR_TIMEOUT);
			CloseHandle(pi.hProcess);
			expect_win32(false);
		}
		WriteConsoleA(si.hStdOutput, "\033[00m", 5, &t, NULL);
		expect_win32(GetExitCodeProcess(pi.hProcess, &retCode));
		
	}
	CloseHandle(hProcess);
	expect_win32(retCode == ERROR_SUCCESS, retCode);
}

void create_process(const std::wstring& app, std::wstring cmdline, const std::wstring& current_dir, const CONFIG* cfg)
{
	const std::string buffer_32 = (boost::dll::program_location().parent_path() / "hide_window_helper32.dll").string();
	const std::string buffer_64 = (boost::dll::program_location().parent_path() / "hide_window_helper64.dll").string();
	const std::string buffer = (sizeof(void*) == 8) ? buffer_64 : buffer_32;

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };
	expect_win32(DetourCreateProcessWithDllExW(app.empty() ? NULL : app.c_str(), cmdline.empty() ? NULL : cmdline.data(), NULL, NULL, FALSE, CREATE_SUSPENDED|CREATE_NEW_CONSOLE, NULL, current_dir.empty() ? NULL : current_dir.c_str(), &si, &pi, buffer.c_str(), NULL));
	BOOST_SCOPE_EXIT(pi) { ResumeThread(pi.hThread); CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }BOOST_SCOPE_EXIT_END;

	CONFIG copyed_cfg;
	copyed_cfg.HidePreview = cfg->HidePreview;
	copyed_cfg.HideTaskbar = cfg->HideTaskbar;
	copyed_cfg.icon_mapping = dup_handle(pi.hProcess, cfg->icon_mapping);
	copyed_cfg.preview_mapping = dup_handle(pi.hProcess, cfg->preview_mapping);
	expect_win32(DetourCopyPayloadToProcess(pi.hProcess, HIDE_WINDOW_CONFIG_GUID, reinterpret_cast<PVOID>(&copyed_cfg), sizeof(CONFIG)));

	cinfo() << "Successfully created process, pid:" << pi.dwProcessId << std::endl;
}

int subcommand_inject(const std::wstring& program_name, const std::vector<std::wstring>& opts) {
	po::options_description inject_opts;

	po::options_description target_opts("Selecting the target process");
	target_opts.add_options()
		("pid,p", po::value<std::vector<int>>(), "target pid")
		("pname,pn", po::wvalue<std::vector<std::wstring>>(), "target process name")
		("grub,g", "grub window from user interaction");

	inject_opts.add_options()("help,h", "display usage for this subcommand");
	inject_opts.add(target_opts);
	add_conf_options(inject_opts);

	po::wparsed_options parsed = po::wcommand_line_parser(opts).
		options(inject_opts).
		run();
	po::variables_map vm;
	po::store(parsed, vm);

	if (vm.count("help")) {
		plain_cout() << "Inject into a running process" << std::endl;
		wcinfo() << program_name << L" inject [args]" << std::endl;
		plain_cout() << inject_opts << std::endl;
		return 0;
	}
	CONFIG cfg;
	get_config(vm, &cfg);
	//if (!cfg)throw(std::runtime_error("cannot get config"));

	std::set<DWORD> collect_pids;
	if (vm.count("pid")) {
		auto pids = vm["pid"].as<std::vector<int>>();
		for (const auto& element : pids) {
			collect_pids.insert(element);
		}
	}
	if (vm.count("pname")) {
		auto names = vm["pname"].as<std::vector<std::wstring>>();
		get_pids_from_pname(names, collect_pids);
	}
	if (vm.count("grub")) {
		DWORD pid = get_pid_from_cursor();
		if (pid != 0)collect_pids.insert(pid);
	}
	my_assert(!collect_pids.empty(), "cannot get target process");

	for (const auto& pid : collect_pids) {
		inject(pid, &cfg);
	}
	return 0;
}
int subcommand_run(const std::wstring& program_name, const std::vector<std::wstring>& opts) {
	po::options_description run_opts;

	po::options_description createprocess_opts("Arguments for creating new process");
	createprocess_opts.add_options()
		("executable", po::wvalue<std::wstring>()->default_value(std::wstring()), "image file to execute")
		("cmdline", po::wvalue<std::wstring>()->default_value(std::wstring()), "command line to execute")
		("working-dir", po::wvalue<std::wstring>()->default_value(std::wstring()), "working directory for the new process");
	run_opts.add_options()("help,h", "display usage for this subcommand");
	run_opts.add(createprocess_opts);
	add_conf_options(run_opts);
	po::positional_options_description pos;
	pos.add("executable", -1);
	po::wparsed_options parsed = po::wcommand_line_parser(opts).
		options(run_opts).
		positional(pos).
		run();
	po::variables_map vm;
	po::store(parsed, vm);

	if (vm.count("help")) {
		plain_cout() << "Run executable with injected dll." << std::endl;
		wcinfo() << program_name << L" run [arguments] [executable image file]" << std::endl;
		plain_cout() << run_opts << std::endl;
		return 0;
	}
	CONFIG cfg;
	get_config(vm, &cfg);
	create_process(vm["executable"].as<std::wstring>(), vm["cmdline"].as<std::wstring>(), vm["working-dir"].as<std::wstring>(), &cfg);
	return 0;
}

BOOL CALLBACK EnumWindowsProc(
	HWND   hwnd,
	LPARAM lParam
) {
	std::vector<HWND>& ret = *reinterpret_cast<std::vector<HWND>*>(lParam);
	//ret.clear();
	UINT msg_id = RegisterWindowMessageA("SetWindowOwnerToDesktop");
	if (IsWindowVisible(hwnd) && ((GetWindowLongPtrA(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) == 0))
	{
		HWND h = (HWND)SendMessageA(hwnd, msg_id, 2, 233);
		if (h == hwnd) {
			ret.push_back(h);
		}
	}
	return TRUE;
}
std::vector<std::pair<HWND,bool>> get_taskbar_windows() {
	std::vector<std::pair<HWND, bool>> ret;
	std::vector<HWND> hwnds;
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&hwnds));
	for (const auto& hwnd : hwnds) {
		ret.emplace_back(hwnd, NULL == GetWindow(hwnd, GW_OWNER));
	}
	return ret;
};
std::wstring window_info_to_string(std::pair<HWND,bool> window_info)
{
	const auto& [hwnd, shown] = window_info;
	if (hwnd == NULL)
		return L"[Closed Window]";
	std::wostringstream woss;

	if (shown)
		woss << "S "; else woss << "H ";

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);

	auto process_name = get_name_from_pid(pid);
	auto window_name = get_window_title(hwnd, 32);


	woss << std::hex << static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hwnd)) << std::dec << "  " << window_name << "  " << process_name << "(" << pid << ")";
	return woss.str();
};

int subcommand_taskbar(const std::wstring& program_name, const std::vector<std::wstring>& opts) {
	po::options_description taskbar_opts;

	taskbar_opts.add_options()
		("command", po::wvalue<std::wstring>(), "command to execute")
		("help,h", "display usage for this subcommand");

	po::positional_options_description pos;
	pos.add("command", 1);

	po::options_description sh_target("Selecting target window");
	sh_target.add_options()
		("pname", po::wvalue<std::vector<std::wstring>>(), "target process name")
		("pid", po::value<std::vector<DWORD>>(), "target pid")
		("title", po::wvalue<std::vector<std::wstring>>(), "target window title")
		("hwnd", po::value<std::vector<std::string>>(), "target window hwnd")
		("all", "show/hide every injected window")
		;
	taskbar_opts.add(sh_target);

	po::wparsed_options parsed = po::wcommand_line_parser(opts).
		options(taskbar_opts).
		positional(pos).
		run();

	po::variables_map vm;
	po::store(parsed, vm);
	if (vm.count("help")) {
		plain_wcout() << L"Show/Hide window from taskbar." << std::endl;
		wcinfo() << program_name << L" taskbar show [args]" << std::endl;
		wcinfo() << program_name << L" taskbar hide [args]" << std::endl;
		plain_cout() << sh_target << std::endl << std::endl;

		plain_wcout() << L"list injected windows." << std::endl;
		wcinfo() << program_name << L" taskbar list" << std::endl << std::endl;

		plain_wcout() << L"list injected windows and control their states." << std::endl;
		wcinfo() << program_name << L" taskbar interactive" << std::endl << std::endl;
		return 0;
	}

	my_assert(vm.count("command"), "no verb found under subcommand 'taskbar'");
	std::wstring cmd = vm["command"].as<std::wstring>();
	if (std::all_of(cmd.cbegin(), cmd.cend(), [](const wchar_t& wc)->bool {return wc >= 0 && wc < 128; })) {
		std::for_each(cmd.begin(), cmd.end(), [](wchar_t& wc) {wc = towlower(wc); });
	}
	UINT msg_id = RegisterWindowMessageA("SetWindowOwnerToDesktop");
	bool has_all = vm.count("all");
	bool has_filter = vm.count("pname") + vm.count("pid") + vm.count("title") + vm.count("hwnd");

	if (cmd == L"show" || cmd == L"hide") {
		//cinfo() << "ShowHide" << std::endl;
		my_assert(has_all || has_filter, "No window specified");
		if (has_all && has_filter) {
			cwarn() << "'--all' specified. Other filters will be ignored." << std::endl;
		}

		std::set<HWND> matching_hwnds;
		if (vm.count("all"))matching_hwnds.insert(HWND_BROADCAST);
		else {
			std::set<HWND> hwnd_vals;
			std::set<std::wstring> window_titles;
			std::set<DWORD> window_pids;
			size_t title_max_len = 0;
			if (vm.count("hwnd")) {
				auto hwnd_strs = vm["hwnd"].as<std::vector<std::string>>();
				for (auto hwnd_str : hwnd_strs) {
					if (hwnd_str.substr(0, 2) == "0x" || hwnd_str.substr(0, 2) == "0X")hwnd_str[1] = 'x';
					else hwnd_str = std::string("0x") + hwnd_str;
					HWND hwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(std::stoull(hwnd_str, nullptr, 16)));
					hwnd_vals.insert(hwnd);
				}
			}
			if (vm.count("title")) {
				auto titles = vm["title"].as<std::vector<std::wstring>>();
				for (auto title : titles) {
					window_titles.insert(title);
					if (title_max_len < title.length())title_max_len = title.length();
				}
			}
			if (vm.count("pname")) {
				auto process_names = vm["pname"].as<std::vector<std::wstring>>();
				get_pids_from_pname(process_names, window_pids);
			}
			if (vm.count("pid")) {
				auto pids = vm["pid"].as<std::vector<DWORD>>();
				for (auto pid : pids) {
					window_pids.insert(pid);
				}
			}
			auto window_hwnds = get_taskbar_windows();
			for (const auto& [h, s] : window_hwnds) {
				if (hwnd_vals.count(h)) {
					matching_hwnds.insert(h);
					continue;
				}

				auto title = get_window_title(h, title_max_len + 1);
				for (const auto& t : window_titles) {
					bool eq =
						!boost::locale::comparator<wchar_t, boost::locale::collator_base::primary>()(title, t) &&
						!boost::locale::comparator<wchar_t, boost::locale::collator_base::primary>()(t, title);
					if (eq) {
						matching_hwnds.insert(h);
						continue;
					}
				}

				DWORD pid;
				GetWindowThreadProcessId(h, &pid);
				if (window_pids.count(pid)) {
					matching_hwnds.insert(h);
					continue;
				}
			}
		}
		my_assert(!matching_hwnds.empty(), "No window matches any of the given filters");
		for (const auto& h : matching_hwnds) {
			SendMessageA(h, msg_id, (cmd == L"show") ? 0 : 1, 0);
		}
	}
	else if (cmd == L"list") {
		auto wins = get_taskbar_windows();
		for (const auto &win_info : wins) {
			plain_wcout() << window_info_to_string(win_info) << std::endl;
		}
	}
	else if (cmd == L"interactive") {
		InteractiveShell shell;

		plain_wcout() << L"\u001b[?1049h";
		BOOST_SCOPE_EXIT(void) { plain_wcout() << L"\u001b[?1049l"; }BOOST_SCOPE_EXIT_END;

		
		const auto draw_screen = [&](const std::vector<std::pair<HWND, bool>>& window_infos, size_t selected) {

			cinfo() << "\u001b[0G\u001b[0d";
			cinfo() << "Use <W>/<S> or arrow keys <Up>/<Down> to go through the list" << "\u001b[0K" << std::endl;
			cinfo() << "Press <Tab> to show or hide window" << "\u001b[0K" << std::endl;
			cinfo() << "Press <ESC> or <Ctrl + C> to quit." << "\u001b[0K" << std::endl;

			plain_wcout() << L"\u001b[0G\u001b[5d";
			for (size_t i = 0; i < window_infos.size(); i++) {
				if (i == selected) {
					colored_stream(termcolor::reverse, std::wcout) << window_info_to_string(window_infos[i]) << L"\u001b[0K" << std::endl;
				}
				else {
					plain_wcout() << window_info_to_string(window_infos[i]) << L"\u001b[0K" << std::endl;
				}
			}
			plain_wcout() << L"\u001b[0J";
		};


		std::vector<std::pair<HWND,bool>> saved_window_handles;
		
		size_t current_selected = 0;

		while (true) {
			auto new_window_infos = get_taskbar_windows();
			std::map<HWND,bool> new_infos_map;
			for (const auto& [h,s] : new_window_infos) {
				new_infos_map[h] = s;
			}
			size_t current_size = 0;
			size_t new_selected = current_selected;
			for (size_t i = 0; i < saved_window_handles.size(); i++) {
				if (i == current_selected)
					new_selected = current_size;
				auto h = saved_window_handles[i];
				if (new_infos_map.contains(h.first)) {
					saved_window_handles[current_size++] = std::make_pair(h.first, new_infos_map[h.first]);
					new_infos_map.erase(h.first);
				}
				else if (i == current_selected) {
					saved_window_handles[current_size++].first = NULL;
				}
			}
			saved_window_handles.resize(current_size);
			for (const auto& [h,s] : new_infos_map) {
				saved_window_handles.emplace_back(h, s);
			}
			current_selected = new_selected;
			draw_screen(saved_window_handles, current_selected);

			//redraw is expensive.

			Sleep(10);

			INPUT_RECORD record;
			bool break_on = false;
			while (true) {
				DWORD s;
				if (
					!ReadConsoleInputExW(shell.hStdin, &record, 1, &s, 0x2) ||
					0 == s || 0 == (record.EventType & KEY_EVENT) ||
					!record.Event.KeyEvent.bKeyDown
					)break;
				auto cas = record.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | RIGHT_CTRL_PRESSED | SHIFT_PRESSED);
				if ((record.Event.KeyEvent.wVirtualKeyCode == VK_UP || record.Event.KeyEvent.wVirtualKeyCode == 'W') && 0 == cas)
				{
					if (current_selected != 0)current_selected--;
				}
				if ((record.Event.KeyEvent.wVirtualKeyCode == VK_DOWN || record.Event.KeyEvent.wVirtualKeyCode == 'S') && 0 == cas)
				{
					if (current_selected != saved_window_handles.size() - 1)current_selected++;
				}
				if ((record.Event.KeyEvent.wVirtualKeyCode == VK_TAB) && 0 == cas)
				{
					if (current_selected >= saved_window_handles.size() || saved_window_handles[current_selected].first == NULL) continue;
					const auto& [hwnd, current_state] = saved_window_handles[current_selected];
					SendMessageA(hwnd, msg_id, current_state ? 1 : 0, 0);
					
				}
				if (
					(record.Event.KeyEvent.wVirtualKeyCode == 'C' && (LEFT_CTRL_PRESSED == cas || RIGHT_CTRL_PRESSED == cas)) ||
					(record.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && 0 == cas)
					)
				{
					//std::cout << "<BREAK>" << std::endl;
					break_on = true;
					break;
				}
			}
			if (break_on)break;
		}
	}
	else {
		wcwarn() << "Unknown subcommand under 'taskbar': " << std::quoted(cmd) << std::endl;
		my_assert(false, "unknown verb");
	}

	return 0;
}
inline static std::map<std::wstring, std::function<int(const std::wstring&, const std::vector<std::wstring>&)>> actions =
{
	{L"help",subcommand_help},
	{L"inject",subcommand_inject},
	{L"run",subcommand_run},
	{L"taskbar",subcommand_taskbar}
};