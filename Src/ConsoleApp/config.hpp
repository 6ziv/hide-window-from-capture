#pragma once
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include "css_color.hpp"
//#include <css-color-parser-cpp/csscolorparser.hpp>

#include "../HookDll/image.h"
#include "../HookDll/config.h"
#include "exceptions.hpp"
#include "environment.hpp"
namespace po = boost::program_options;
template<> inline std::string boost::lexical_cast(const std::wstring& wstr) {
	return boost::locale::conv::from_utf<wchar_t>(wstr, std::locale());
}
inline std::wistream& operator>>(std::wistream& is, ImageResizeMode& mode) {
	std::wstring wstr;
	is >> wstr;
	boost::algorithm::to_lower(wstr);
	if (wstr == L"none" || wstr == L"noresize")mode = MY_STRETCH_NORESIZE;
	else if (wstr == L"repeat")mode = MY_STRETCH_REPEAT;
	else if (wstr == L"stretch")mode = MY_STRETCH_RESIZE;
	else if (wstr == L"keepratio")mode = MY_STRETCH_KEEPRATIO;
	else {
		std::ostringstream oss;
		oss << "unknown resize mode:" << std::quoted(boost::lexical_cast<std::string>(wstr));
		my_assert(false, oss.str());
	}
	return is;
}
inline nlohmann::json checked_parse(std::ifstream&& ifs) {
	try {
		return nlohmann::json::parse(ifs);
	}
	catch (const nlohmann::json::parse_error& e) {
		wcwarn() << L"Error parsing global defaults." << std::endl;
		cwarn() << e.what() << std::endl;
	}
	return nlohmann::json::parse("{}");
}
nlohmann::json& get_default() {
	static nlohmann::json default_conf = checked_parse(std::ifstream(expand_env("%HIDE_PROGRAM_ROOT%\\conf\\default.json")));
	return default_conf;
}
inline std::ostream& operator<<(std::ostream& os, const ImageResizeMode& mode) {
	switch (mode) {
	case MY_STRETCH_NORESIZE:
		return os << "none";
	case MY_STRETCH_REPEAT:
		return os << "repeat";
	case MY_STRETCH_RESIZE:
		return os << "stretch";
	case MY_STRETCH_KEEPRATIO:
		return os << "keepratio";
	default:
		my_assert(false, "Unknown resize mode val:" + std::to_string(mode));
	}
	return os;
}

inline void add_conf_options(po::options_description& desc) {
	po::options_description preview_options("Image used to hide window preview");
	preview_options.add_options()
		("preview", po::wvalue<std::wstring>()->default_value(boost::locale::conv::utf_to_utf<wchar_t, char>(get_default().value("preview", std::string())))->value_name("path_to_image"), "image used to replace window preview")
		("thumbnail", po::wvalue<std::wstring>()->default_value(boost::locale::conv::utf_to_utf<wchar_t, char>(get_default().value("thumbnail", std::string())))->value_name("path_to_image"), "image used to replace window thumbnail")
		("preview-scaling", po::wvalue<ImageResizeMode>()->default_value(boost::lexical_cast<ImageResizeMode>(get_default().value("preview-scaling", "keepratio").c_str()))->value_name("[none|repeat|stretch|keepratio]"), "policy for resizing preview")
		("thumbnail-scaling", po::wvalue<ImageResizeMode>()->default_value(boost::lexical_cast<ImageResizeMode>(get_default().value("thumbnail-scaling", "stretch").c_str()))->value_name("[repeat|stretch|keepratio]"), "policy for resizing thumbnail")
		("preview-background", po::value<std::string>()->default_value(get_default().value("preview-background", "transparent"))->value_name("color"), "background color for preview")
		("thumbnail-background", po::value<std::string>()->default_value(get_default().value("thumbnail-background", "white"))->value_name("color"), "background color for thumbnail")
		;
	//thumbnail is always in "stretch" mode.
	po::options_description switches("Choose what to hide");
	switches.add_options()
		("hide-preview", po::value<bool>()->default_value(get_default().value("hide-preview", true), get_default().value("hide-preview", true) ? "on" : "off")->value_name("[on|off]"), "hide window preview in Task Switcher")
		("hide-taskbar", po::value<bool>()->default_value(get_default().value("hide-taskbar", false), get_default().value("hide-taskbar", false) ? "on" : "off")->value_name("[on|off]"), "hide window from taskbar")
		;

	desc.add(switches).add(preview_options);
}
inline void load_img_default_confs(po::variables_map& vm) {
	if (vm["preview-scaling"].defaulted() || vm["preview-background"].defaulted())
	{
		std::ifstream preview_def_conf_file(std::filesystem::path(expand_env(vm["preview"].as<std::wstring>()) + L".json"));
		if (preview_def_conf_file.is_open()) {
			nlohmann::json preview_def_conf;
			try {
				preview_def_conf = nlohmann::json::parse(preview_def_conf_file);
			}
			catch (const nlohmann::json::parse_error& e) {
				wcwarn() << L"Error parsing img configuration: " << expand_env(vm["preview"].as<std::wstring>()) + L".json" << std::endl;
				cwarn() << e.what() << std::endl;
			}

			if (preview_def_conf.contains("scaling") && vm["preview-scaling"].defaulted()) {
				auto scale = boost::lexical_cast<ImageResizeMode>(preview_def_conf["scaling"].get<std::string>().c_str());
				if (vm["preview-scaling"].as<ImageResizeMode>() != scale) {
					cinfo() << "Overriding global preview scaling defaults with image-specific default: ( " << vm["preview-scaling"].as<ImageResizeMode>() << " -> " << scale << " )" << std::endl;
					vm.insert_or_assign("preview-scaling", po::variable_value(scale, true));
				}
			}
			if (preview_def_conf.contains("background") && vm["preview-background"].defaulted()) {
				auto color = preview_def_conf["background"].get<std::string>();
				if (vm["preview-background"].as<std::string>() != color) {
					cinfo() << "Overriding global preview background-color defaults with image-specific default: ( " << std::quoted(vm["preview-background"].as<std::string>()) << " -> " << std::quoted(color) << " )" << std::endl;
					vm.insert_or_assign("preview-background", po::variable_value(color, true));
				}
			}
		}
	}

	if (vm["thumbnail-scaling"].defaulted() || vm["thumbnail-background"].defaulted())
	{
		std::ifstream thumbnail_def_conf_file(std::filesystem::path(expand_env(vm["thumbnail"].as<std::wstring>()) + L".json"));
		if (thumbnail_def_conf_file.is_open()) {
			nlohmann::json thumbnail_def_conf;
			try {
				thumbnail_def_conf = nlohmann::json::parse(thumbnail_def_conf_file);
			}
			catch (const nlohmann::json::parse_error& e) {
				wcwarn() << L"Error parsing img configuration: " << expand_env(vm["thumbnail"].as<std::wstring>()) + L".json" << std::endl;
				cwarn() << e.what() << std::endl;
			}
			if (thumbnail_def_conf.contains("scaling") && vm["thumbnail-scaling"].defaulted()) {
				auto scale = boost::lexical_cast<ImageResizeMode>(thumbnail_def_conf["scaling"].get<std::string>().c_str());
				if (vm["thumbnail-scaling"].as<ImageResizeMode>() != scale) {
					cinfo() << "Overriding global thumbnail scaling defaults with image-specific default: ( " << vm["thumbnail-scaling"].as<ImageResizeMode>() << " -> " << scale << " )" << std::endl;
					vm.insert_or_assign("thumbnail-scaling", po::variable_value(scale, true));
				}
			}
			if (thumbnail_def_conf.contains("background") && vm["thumbnail-background"].defaulted()) {
				auto color = thumbnail_def_conf["background"].get<std::string>();
				if (vm["thumbnail-background"].as<std::string>() != color) {
					cinfo() << "Overriding global thumbnail background-color defaults with image-specific default: ( " << std::quoted(vm["thumbnail-background"].as<std::string>()) << " -> " << std::quoted(color) << " )" << std::endl;
					vm.insert_or_assign("thumbnail-background", po::variable_value(color, true));
				}
			}
		}
	}

}
inline void get_config(po::variables_map& vm, CONFIG* pConfig) {
	CONFIG& config = *pConfig;
	config.HidePreview = vm["hide-preview"].as<bool>();
	config.HideTaskbar = vm["hide-taskbar"].as<bool>();
	if (!config.HidePreview) {
		if (vm.count("preview") || vm.count("thumbnail") || vm.count("preview-scaling") || vm.count("thumbnail-scaling") || vm.count("preview-background") || vm.count("thumbnail-background")) {
			cwarn() << "Preview/Thumbnail will not be hidden, but related configuration arguments are set" << std::endl;
			cwarn() << "They will be ignored." << std::endl;
		}
		config.icon_mapping = config.preview_mapping = NULL;
	}
	else {
		my_assert(vm["thumbnail-scaling"].as<ImageResizeMode>() != MY_STRETCH_NORESIZE, "thumbnail resize mode cannot be 'none'");

		load_img_default_confs(vm);
		auto to_hex = [](const auto& c)->auto {
			auto arr = css_colors::to_dword(c, "argb").value_or(std::array<uint8_t, 4>{255, 255, 255, 255});
			return (static_cast<uint32_t>(arr[0]) << 24) | (static_cast<uint32_t>(arr[1]) << 16) | (static_cast<uint32_t>(arr[2]) << 8) | (static_cast<uint32_t>(arr[3]));
		};
		//if(vm["preview-scaling"].defaulted())
		config.preview_mapping = load_img(expand_env(vm["preview"].as<std::wstring>()), vm["preview-scaling"].as<ImageResizeMode>(), to_hex(css_colors::parse(vm["preview-background"].as<std::string>().c_str())));
		config.icon_mapping = load_img(expand_env(vm["thumbnail"].as<std::wstring>()), vm["thumbnail-scaling"].as<ImageResizeMode>(), to_hex(css_colors::parse(vm["thumbnail-background"].as<std::string>().c_str())));
	}
}