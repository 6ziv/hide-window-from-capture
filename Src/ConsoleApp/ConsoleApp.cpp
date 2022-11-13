// ConsoleApp.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
//#include <css-color-parser-cpp/csscolorparser.hpp>
#include <windows.h>
#include <sdkddkver.h>
#include <random>
#include <detours/detours.h>
#include "targets.hpp"
#include "load_image.hpp"
#include "detect_target_bitness.hpp"
#include "parse_arguments.hpp"
#include "subcommands.hpp"
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include "../HookDll/image.h"
namespace po = boost::program_options;
LONG WINAPI UnhandledExceptionFilterFunc(
	_EXCEPTION_POINTERS* ExceptionInfo
){
	plain_cout() << "some exception." << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
	
	return EXCEPTION_EXECUTE_HANDLER;
}

int wmain(int argc, wchar_t* argv[]) try
{
	SetUnhandledExceptionFilter(UnhandledExceptionFilterFunc);
	std::ios::sync_with_stdio(true);
	std::locale::global(boost::locale::generator()(""));

	wcinfo() << "Setting HIDE_PROGRAM_ROOT to:" << boost::dll::program_location().parent_path() << std::endl;
	SetEnvironmentVariableW(L"HIDE_PROGRAM_ROOT", boost::dll::program_location().parent_path().c_str());

	std::wstring verb;
	std::vector<std::wstring> args;
	auto parse_verb_result = parse_verb(argc, argv);
	if (!parse_verb_result) {
		cwarn() << "No subcommand found" << std::endl;
		return subcommand_help(argv[0]);
	}
	std::tie(verb, args) = parse_verb_result.value();
	if (!actions.contains(verb)) {
		wcwarn() << L"Unknown subcommand:" << std::quoted(verb)  << std::endl;
		return subcommand_help(argv[0], args);
	}
	return std::invoke(actions[verb], argv[0], args);
	/*
	po::options_description global;

	global.add_options()
		("command", po::value<std::string>(), "command to execute")
		("subargs", po::value<std::vector<std::string> >(), "Arguments for command");

	po::positional_options_description pos;
	pos.add("command", 1).
		add("subargs", -1);

	po::wparsed_options parsed = po::wcommand_line_parser(argc, argv).
		options(global).
		positional(pos).
		allow_unregistered().
		run();

	po::variables_map vm;
	po::store(parsed, vm);

	if (!vm.count("command")) {
		std::cerr << termcolor::red << "No subcommand found" << termcolor::reset << std::endl;
		return subcommand_help(argv[0]);
	}

	std::string cmd = vm["command"].as<std::string>();
	std::vector<std::wstring> opts = po::collect_unrecognized(parsed.options, po::include_positional);
	opts.erase(opts.cbegin());
	if (cmd == "help") {
		return subcommand_help(argv[0]);
	}
	else if (cmd == "inject") {
		return subcommand_inject(opts);
	}
	else {
		std::cerr << termcolor::red << "Unrecognized subcommand " << std::quoted(cmd) << termcolor::reset << std::endl;
		return subcommand_help(argv[0]);
	}

	std::cout << "FLAG3" << std::endl;
	std::cout << cmd << std::endl;
	for (auto opt : opts)std::wcout << opt << "  ";*/
}
catch (const std::exception& e) {
	std::cerr << termcolor::red << "Exception:" << e.what() << termcolor::reset << std::endl;
}
catch (...) {
	std::cerr << termcolor::red << "An unknown exception was raised." << termcolor::reset << std::endl;
}