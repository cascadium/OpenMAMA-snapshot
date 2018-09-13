//
// Created by fquinn on 17/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_OPENMAMADICTIONARYREQUESTOR_H
#define OPENMAMA_SNAPSHOT_OPENMAMADICTIONARYREQUESTOR_H

#include <iostream>
#include <mama/mamacpp.h>
#include "../SubsystemOpenMama.h"
#include <mamda/MamdaOrderBookFields.h>


namespace cascadium {
    class OpenMamaDictionaryRequestor : public Wombat::MamaDictionaryCallback {
    public:
        explicit OpenMamaDictionaryRequestor(mamaBridge bridge)
                : bridge(bridge), hasDictionary(false),
                  logger(Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA)) {

        }

        Wombat::MamaDictionary *requestDictionary(Wombat::MamaSource *source) {
            auto * dictionary = new Wombat::MamaDictionary;
            dictionary->create(Wombat::Mama::getDefaultEventQueue(bridge), this, source);

            // This blocks until success or failure
            Wombat::Mama::start(bridge);
            if (!hasDictionary) {
                logger.error("Failed to acquire a dictionary");
                delete dictionary;
                dictionary = nullptr;
            } else {
                logger.information("Returning acquired dictionary");
                Wombat::MamdaOrderBookFields::setDictionary(*dictionary);
            }
            return dictionary;
        }

    private:
        /*Callbacks from the MamaDictionaryCallback class*/
        void onTimeout() override {
            logger.error("Timed out waiting for data dictionary");
            Wombat::Mama::stop(bridge);
        }

        void onError(const char *errMsg) override {
            logger.error("Error getting dictionary: %s", std::string(errMsg));
            Wombat::Mama::stop(bridge);
        }

        void onComplete() override {
            logger.error("Now have a dictionary!");
            hasDictionary = true;
            Wombat::Mama::stop(bridge);
        }

        mamaBridge bridge;
        bool hasDictionary;
        Poco::Logger &logger;
    };
};


#endif //OPENMAMA_SNAPSHOT_OPENMAMADICTIONARYREQUESTOR_H
