
#include "stdafx.h"
#include "qtlconfig.h"
#include "generate.h"
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char* argv[])
{
	try
	{
		po::options_description desc("options");
		string input_path, output_path, template_path;
		desc.add_options()
			("help,h", "show help message")
			("input-path,i", po::value<string>(&input_path), "input path (optional)")
			("output-path,o", po::value<string>(&output_path), "output path (optional)")
			("template-path,t", po::value<string>(&template_path), "template path (optional)")
			("config-file", po::value<vector<string>>(), "database configure file (YAML)");
		
		po::positional_options_description p;
		p.add("config-file", -1);
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
		po::notify(vm);

		if (vm.count("help") > 0)
		{
			cout << desc << endl;
			return -1;
		}

		if (vm.count("config-file") == 0)
		{
			cerr << "no database configure file" << endl;
			return -1;
		}

		if (template_path.empty())
		{
			fs::path pth = boost::dll::program_location();
			pth.remove_filename();
			pth /= "template";
			template_path = pth.string();
		}
		else
		{
			fs::path pth(template_path);
			if (pth.is_relative())
				pth = fs::absolute(pth);
			fs::create_directories(pth);
		}
		if(template_path.back()!= (char)fs::path::preferred_separator)
			template_path += (char)fs::path::preferred_separator;

		if (output_path.empty())
		{
			fs::path pth = fs::current_path();
			output_path = pth.string();
		}
		else
		{
			fs::path pth(output_path);
			if (pth.is_relative())
				pth = fs::absolute(pth);
			fs::create_directories(pth);
		}
		if (output_path.back() != (char)fs::path::preferred_separator)
			output_path += (char)fs::path::preferred_separator;

		if (input_path.empty())
		{
			fs::path pth = fs::current_path();
			input_path = pth.string();
		}
		else
		{
			fs::path pth(input_path);
			if (pth.is_relative())
				pth = fs::absolute(pth);
			fs::create_directories(pth);
		}
		if (input_path.back() != (char)fs::path::preferred_separator)
			input_path += (char)fs::path::preferred_separator;

		auto config_files =vm["config-file"].as< vector<string> >();
		fs::path in_pth(input_path);
		for (string file : config_files)
		{
			fs::path pth(file);
			if (pth.is_relative())
			{
				pth = fs::absolute(pth, in_pth);
				file = pth.string();
			}
			qtlconfig config;
			cout << file << endl;
			config.load(file.data());
			if (config.prepare())
			{
				generate(config, template_path, output_path);
			}
		}
	}
	catch (std::exception& e)
	{
		cerr << e.what() << endl;
	}

	return 0;
}

