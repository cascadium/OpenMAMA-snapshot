//
// Created by fquinn on 22/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_SUBSYSTEMCOMMON_H
#define OPENMAMA_SNAPSHOT_SUBSYSTEMCOMMON_H


#include <Poco/Util/Subsystem.h>
#include <Poco/Util/Application.h>

class SubsystemCommon: public Poco::Util::Subsystem {
public:
    // Alternative way to trigger handleOption which will call handleOption to avoid protection issue
    virtual void handleOption(const std::string &name, const std::string &value) = 0;
};


#endif //OPENMAMA_SNAPSHOT_SUBSYSTEMCOMMON_H
