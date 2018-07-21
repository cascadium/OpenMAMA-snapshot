//
// Created by fquinn on 26/06/2018.
//

#include <boost/program_options.hpp>
#include <iostream>
#include "ConfigManager.h"

namespace po = boost::program_options;

std::vector<ConfigManager*> ConfigManager::getApplicationRunnerConfigs() {
    std::vector<ConfigManager*> applicationRunnerConfigs;
    return applicationRunnerConfigs;
}

ConfigManager::ConfigManager(int argc, char *argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("compression", po::value<int>(), "set compression level")
            ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
    }
    std::cout << "Hello, World!" << std::endl;
}
