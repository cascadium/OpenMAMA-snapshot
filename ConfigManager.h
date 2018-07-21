//
// Created by fquinn on 26/06/2018.
//

#ifndef OPENMAMA_SNAPSHOT_CONFIGMANAGER_H
#define OPENMAMA_SNAPSHOT_CONFIGMANAGER_H

#include <vector>

class ConfigManager {

public:
    ConfigManager(int argc, char *argv[]);
    std::vector<ConfigManager*> getApplicationRunnerConfigs();
};


#endif //OPENMAMA_SNAPSHOT_CONFIGMANAGER_H
