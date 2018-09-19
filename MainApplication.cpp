//
// Created by fquinn on 22/07/2018.
//

#include "MainApplication.h"
#include "subsystems/SubsystemServletRouter.h"
#include "subsystems/SubsystemOpenMama.h"
#include "OptionNames.h"
#include <Poco/Util/OptionCallback.h>
#include <iostream>
#include <Poco/Process.h>
#include <Poco/SignalHandler.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/OptionException.h>

#include <signal.h>
#include <sstream>

using namespace cascadium;
using namespace std;
using namespace Poco;
using namespace Poco::Util;

void MainApplication::initialize(Poco::Util::Application &self) {
    logger().debug("Initializing the main application container");
    Application::initialize(self);
}

void MainApplication::uninitialize() {
    logger().debug("Initializing the main application container");
    Application::uninitialize();
}

void MainApplication::reinitialize(Poco::Util::Application &self) {
    Application::reinitialize(self);
}

void MainApplication::handleOption(const std::string &name, const std::string &value) {
    if (value.length()) {
        logger().fatal("Handling option with argument in %s: '%s' = [%s]", std::string(this->name()), name, value);
    } else {
        logger().fatal("Handling boolean option in %s: '%s'", std::string(this->name()), name);
    }

    if (name == OPTION_NAME_HELP) {
        this->displayUsage(2);
    }

    try {
        // Call option handler in subsystems
        auto subsystems = this->subsystems();
        for (auto &subsystem : subsystems) {
            Subsystem *pocoSubsystem = subsystem.get();
            if (string(pocoSubsystem->name()).substr(0, 9) == "Cascadium") {
                auto *subsystemCommon = (SubsystemCommon *) pocoSubsystem;
                subsystemCommon->handleOption(name, value);
            }
        }
    } catch (...) {
        this->displayUsage(2);
    }
    Application::handleOption(name, value);
}

void MainApplication::signalHandler(int signum) {
    MainApplication::terminate();
}

void MainApplication::displayUsage(int exitCode) {
    HelpFormatter helpFormatter(this->options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setUnixStyle(true);
    helpFormatter.setHeader("OpenMAMA caching REST API bridge application.");
    helpFormatter.setWidth(80);
    helpFormatter.format(cout);
    exit(exitCode);
}

int MainApplication::main(const std::vector<std::string> &args) {
    logger().debug("Initializing subsystems...");
    waitForTerminationRequest();
    logger().information("Initializing subsystems...");
    return Application::EXIT_OK;
}

const char *MainApplication::name() const {
    return MAIN_APPLICATION_NAME;
}

MainApplication::MainApplication() : ServerApplication::ServerApplication() {
    signal(1, signalHandler);
    signal(2, signalHandler);
    signal(3, signalHandler);
    signal(15, signalHandler);
    signal(30, signalHandler);
    signal(31, signalHandler);
    logger().debug("Initializing subsystems...");
    this->addSubsystem(new SubsystemServletRouter());
    this->addSubsystem(new SubsystemOpenMama());
    logger().information("All subsystems initialized ready to receive requests");
}

void MainApplication::defineOptions(Poco::Util::OptionSet &options) {

    Option help(OPTION_NAME_HELP, "h", "Show usage information.");
    help.required(false).repeatable(true);
    options.addOption(help);

    // Only add option if not already specified elsewhere
    Option verbose(OPTION_NAME_INCREASE_VERBOSITY, "v", "Increase verbosity (may be passed up to 4 times.");
    verbose.required(false).repeatable(true);
    options.addOption(verbose);

    // Only add option if not already specified elsewhere
    Option quiet(OPTION_NAME_DECREASE_VERBOSITY, "q", "Decrease verbosity (may be passed up to 4 times.");
    quiet.required(false).repeatable(true);
    options.addOption(quiet);

    Poco::Util::ServerApplication::defineOptions(options);
}
