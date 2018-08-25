//
// Created by fquinn on 15/08/2018.
//

#include "SubsystemServletRouter.h"
#include <iostream>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/URI.h>
#include <OptionNames.h>
#include "../servlets/ServletOpenMamaSnapshot.h"
#include "../servlets/ServletRequestHandlerFactory.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace cascadium;

const char *SubsystemServletRouter::name() const {
    return SUBSYSTEM_NAME_SERVLET_ROUTER;
}

void SubsystemServletRouter::initialize(Poco::Util::Application &app) {
    logger.debug("Initializing the servlet router subsystem");
    httpServer = new Net::HTTPServer(new ServletRequestHandlerFactory, httpServerPort);
    httpServer->start();
    logger.information("HTTP Server started on port %hu - ready to receive requests.", httpServerPort);
}

void SubsystemServletRouter::handleOption(const string &name, const string &value) {
    if (name == OPTION_NAME_HTTP_PORT) {
        httpServerPort = (UInt16) strtol(value.c_str(), nullptr, 0);
        logger.notice("Will launch http server on port %hu", httpServerPort);
    } else if (name == OPTION_NAME_INCREASE_VERBOSITY) {
        logger.setLevel(logger.getLevel() + 1);
    } else if (name == OPTION_NAME_DECREASE_VERBOSITY) {
        logger.setLevel(logger.getLevel() - 1);
    }
}

void SubsystemServletRouter::uninitialize() {
    cerr << "Stopping the servlet subsystem" << endl;
    httpServer->stop();
}

void SubsystemServletRouter::reinitialize(Poco::Util::Application &app) {
    Subsystem::reinitialize(app);
}

void SubsystemServletRouter::defineOptions(Poco::Util::OptionSet &options) {
    auto callback = Poco::Util::OptionCallback<SubsystemServletRouter>(
            this, &SubsystemServletRouter::handleOption);

    // Only add option if not already specified elsewhere
    Option optionPort(OPTION_NAME_HTTP_PORT, "l", "Specify port to listen for HTTP requests on.");
    optionPort.required(false).repeatable(false).argument("NUM");
    options.addOption(optionPort);

    Subsystem::defineOptions(options);
}

SubsystemServletRouter::SubsystemServletRouter() :
logger(Poco::Logger::get(SUBSYSTEM_NAME_SERVLET_ROUTER)),
httpServer(nullptr),
httpServerPort(8080)
{

}

SubsystemServletRouter::~SubsystemServletRouter() {
    cerr << "In destructor for subsystem servlet router" << endl;
}
