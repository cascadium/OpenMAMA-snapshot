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
#include "SubsystemCommon.h"
#include <mama/MamaDictionary.h>

namespace cascadium {

    // Forward declarations to avoid circular reference
    class OpenMamaStoreMessageListener;

    class SubsystemOpenMama : public SubsystemCommon {
    public:
        SubsystemOpenMama();
        Wombat::MamaDictionary *getDictionary();

        // Blocking call to acquire a snapshot of a symbol. Will raise a TimeoutException on timeout.
        std::string
        getSnapshotAsJson(const std::string &symbol);

        OpenMamaStoreMessageListener*
        getOpenMamaStoreMessageListener(const std::string &symbol);

        void defineOptions(Poco::Util::OptionSet &options) override;

        void handleOption(const std::string &name, const std::string &value) override;

    protected:

        void initialize(Poco::Util::Application &app) override;

        void uninitialize() override;

        void reinitialize(Poco::Util::Application &app) override;

        const char *name() const override;

        ~SubsystemOpenMama() override;

        ///void setUpSubsystemOpenMamaLogCn(MamaLogLevel level, const char *message);

    private:

        Poco::Semaphore dictionarySemaphore;

        Poco::Logger &logger;

        Wombat::MamaDictionary *dictionary;

        mamaBridge bridge;

        Wombat::MamaSource* source;

        std::string transport;

        std::string sourceName;

        std::string dictionarySource;

        std::string dictionaryTransport;

        std::string dictionaryMiddleware;

        std::vector<std::string> middlewares;

        std::vector<std::string> payloads;

        std::map<std::string, OpenMamaStoreMessageListener*> openMamaStoreMessageListenerBySymbol;
    };
};

#endif //OPENMAMA_SNAPSHOT_SUBSYSTEMOPENMAMASTORE_H
