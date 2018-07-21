#include <iostream>
#include <boost/program_options.hpp>
#include <iostream>
#include "ConfigManager.h"
#include "ApplicationRunner.h"

int main(int argc, char *argv[]) {

    // Create new config manager for everything this application requires
    auto* cm = new ConfigManager(argc, argv);

    // Create application runner based on the parsed configuration
    for (ConfigManager* applicationRunnerConfig : cm->getApplicationRunnerConfigs()) {
        auto* runner = new ApplicationRunner(applicationRunnerConfig);
    }

    return 0;
}