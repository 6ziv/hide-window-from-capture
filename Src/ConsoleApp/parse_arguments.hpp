#pragma once
#include <optional>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include "exceptions.hpp"
namespace po = boost::program_options;
std::optional<std::pair<std::wstring, std::vector<std::wstring>>> parse_verb(int argc,wchar_t *argv[])
{
	po::options_description global;

	global.add_options()
		("command", po::wvalue<std::wstring>(), "command to execute")
		("subargs", po::wvalue<std::vector<std::wstring> >(), "Arguments for command");

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
		return std::nullopt;
	}
	
	std::wstring cmd = vm["command"].as<std::wstring>();
	if (std::all_of(cmd.cbegin(), cmd.cend(), [](const wchar_t& wc)->bool {return wc >= 0 && wc < 128; })) {
		std::for_each(cmd.begin(), cmd.end(), [](wchar_t& wc) {wc = towlower(wc); });
	}

	std::vector<std::wstring> opts = po::collect_unrecognized(parsed.options, po::include_positional);
	opts.erase(opts.begin());

	return std::make_pair(cmd, opts);
}