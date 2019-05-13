//
// Created by fquinn on 15/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASTORE_H
#define OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASTORE_H

#define SUBSYSTEM_NAME_OPEN_MAMA "CascadiumSubsystemOpenMama"

#include <Poco/Util/Subsystem.h>
#include <Poco/Logger.h>
#include <Poco/Semaphore.h>
#include <map>
#include <set>
#include "SubsystemCommon.h"
#include <mama/MamaDictionary.h>

namespace cascadium {

    // Forward declarations to avoid circular reference
    class OpenMamaStoreMessageListener;

    class SubsystemOpenMama : public SubsystemCommon, Wombat::MamaStartCallback {
        public:

            SubsystemOpenMama();

            Wombat::MamaDictionary *getDictionary();

            // Blocking call to acquire a snapshot of a symbol. Will raise a TimeoutException on timeout.
            std::string getSnapshotAsJson(const std::string &symbol, long& eventCount);

            std::string removeOpenMamaStoreMessageListener(const std::string& symbol);

            OpenMamaStoreMessageListener* getOpenMamaStoreMessageListener(const std::string &symbol);

            void defineOptions(Poco::Util::OptionSet &options) override;

            void handleOption(const std::string &name, const std::string &value) override;

        protected:

            void initialize(Poco::Util::Application &app) override;

            void uninitialize() override;

            void reinitialize(Poco::Util::Application &app) override;

            const char *name() const override;

            ~SubsystemOpenMama() override;

            void onStartComplete(Wombat::MamaStatus status) override;

        private:
            // Internal state
            Poco::Semaphore dictionarySemaphore;
            Poco::Logger &logger;
            Wombat::MamaDictionary * dictionary;
            mamaBridge bridge;
            std::set<mamaPayloadBridge> payloadBridges;
            Wombat::MamaSource * source;
            std::map<std::string, OpenMamaStoreMessageListener *> openMamaStoreMessageListenerBySymbol;
            Wombat::MamaQueueGroup * queueGroup;
            Poco::Mutex listenerMapMutex;
            mamaPayloadBridge defaultPayloadBridge;

            // Configuration parameters
            unsigned int configNumQueues;
            std::string configTransport;
            std::string configSourceName;
            std::string configDictionarySource;
            std::string configDictionaryTransport;
            std::string configDictionaryMiddleware;
            std::string configMiddleware;
            std::set<std::string> configPayloads;
            int configVerbosity;
    };
};

#endif //OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASTORE_H
