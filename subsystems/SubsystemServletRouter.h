//
// Created by fquinn on 15/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASNAPSHOT_H
#define OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASNAPSHOT_H

#define SUBSYSTEM_NAME_SERVLET_ROUTER "CascadiumSubsystemServletRouter"

#include <Poco/Util/Subsystem.h>
#include <vector>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Logger.h>
#include <iostream>
#include "SubsystemCommon.h"

namespace cascadium {
    class SubsystemServletRouter : public SubsystemCommon {
    protected:

    public:
        SubsystemServletRouter();
        ~SubsystemServletRouter() override;

        void defineOptions(Poco::Util::OptionSet &options) override;

        void handleOption(const std::string &name, const std::string &value) override;

    protected:
        void initialize(Poco::Util::Application &app) override;

        void uninitialize() override;

        void reinitialize(Poco::Util::Application &app) override;

        const char *name() const override;

    private:
        Poco::Net::HTTPServer *httpServer;

        Poco::Logger &logger;

        Poco::UInt16 httpServerPort;
    };
};

#endif //OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASNAPSHOT_H
