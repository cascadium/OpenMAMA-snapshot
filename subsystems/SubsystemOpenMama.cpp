//
// Created by fquinn on 15/08/2018.
//

#include <OptionNames.h>
#include "SubsystemServletRouter.h"
#include <iostream>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/DynamicStruct.h>
#include <mama/mamacpp.h>
#include <mama/log.h>
#include <mamda/MamdaSubscription.h>
#include <mama/MamaTransport.h>
#include <mama/fieldcache/MamaFieldCache.h>
#include <mamda/MamdaOrderBook.h>
#include <mamda/MamdaOptionChain.h>
#include <mamda/MamdaOrderBookListener.h>
#include <mamda/MamdaErrorListener.h>
#include <mamda/MamdaQualityListener.h>
#include <mamda/MamdaQualityListener.h>
#include "SubsystemOpenMama.h"
#include "openmama/OpenMamaDictionaryRequestor.h"
#include "openmama/OpenMamaStoreMessageListener.h"
#include <map>


using namespace std;
using namespace Poco;
using namespace Poco::Util;
using namespace Wombat;
using namespace cascadium;

const char *SubsystemOpenMama::name() const {
    return SUBSYSTEM_NAME_OPEN_MAMA;
}

Poco::Logger* getSubsystemOpenMamaLogger() {
    static Poco::Logger* logger = nullptr;
    if (logger == nullptr) {
        logger = &Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA);
    }
    return logger;
}

void setUpSubsystemOpenMamaLogCb(MamaLogLevel level, const char *message) {
    if (nullptr == message) {
        return;
    }
    Poco::Logger* logger = getSubsystemOpenMamaLogger();

    // Trim trailing newline since logger will add another
    const char* lastChar = message + strlen(message) - 1;
    if ('\n' == *lastChar) {
        *(char*)lastChar = '\0';
    }
    switch(level) {
        case MAMA_LOG_LEVEL_OFF:
            logger->fatal(message);
            break;
        case MAMA_LOG_LEVEL_SEVERE:
            logger->critical(message);
            break;
        case MAMA_LOG_LEVEL_ERROR:
            logger->error(message);
            break;
        case MAMA_LOG_LEVEL_WARN:
            logger->warning(message);
            break;
        case MAMA_LOG_LEVEL_NORMAL:
            logger->notice(message);
            break;
        case MAMA_LOG_LEVEL_FINE:
            logger->information(message);
            break;
        case MAMA_LOG_LEVEL_FINER:
            logger->debug(message);
            break;
        case MAMA_LOG_LEVEL_FINEST:
            logger->trace(message);
            break;
    }
}

void SubsystemOpenMama::onStartComplete(MamaStatus status) {
    logger.debug("OpenMAMA is closing");
    ScopedLock<Mutex> scopedLock(listenerMapMutex);
    logger.debug("Removing subscriptions");
    for (auto& pair : openMamaStoreMessageListenerBySymbol) {
        const string& symbol = pair.first;
        OpenMamaStoreMessageListener* listener = pair.second;
        MamdaSubscription* subscription = listener->getSubscription();
        delete subscription;
        delete listener;
    }
    MamaTransport* transport = source->getTransport();
    delete source;
    delete transport;
    logger.debug("OpenMAMA is closed");
}

void SubsystemOpenMama::initialize(Poco::Util::Application &app) {
    logger.debug("Initializing the store subsystem");
    mama_setLogCallback2(setUpSubsystemOpenMamaLogCb);
    bridge = Mama::loadBridge("zmq");
//    Mama::setLogLevel(MAMA_LOG_LEVEL_OFF);
    logger.information("Application has loaded %s bridge version: %s",
            string("zmq"),
            string(Mama::getVersion(bridge)));
    try {
        Mama::open();
    } catch (MamaStatus& status) {
        logger.fatal("Found error: %s", string(status.toString()));
    }
    source = new MamaSource("OPENMAMA", "sub", "OPENMAMA", bridge, true);

    auto * dictionaryTransport = new MamaTransport();
    dictionaryTransport->create("sub", bridge);
    auto * dictionaryMamaSource = new MamaSource("WOMBAT", dictionaryTransport, "WOMBAT");
    OpenMamaDictionaryRequestor dictRequester (bridge);
    // Create the queue group
    queueGroup = new MamaQueueGroup(configNumQueues, bridge);
    dictionary = dictRequester.requestDictionary (dictionaryMamaSource);
    // No longer need this MAMA source after dictionary retrieval so destroy here
    delete dictionaryMamaSource;
    delete dictionaryTransport;
    // Wait for first snapshot to land
    dictionarySemaphore.set();

    Mama::startBackground(bridge, this);
}

void SubsystemOpenMama::handleOption(const string &name, const string &value) {
    if (name == OPTION_NAME_INCREASE_VERBOSITY) {
        logger.setLevel(logger.getLevel() + 1);
        int newLevel = (int) Mama::getLogLevel() + 1;
        Mama::setLogLevel(
                (MamaLogLevel) (newLevel > MamaLogLevel::MAMA_LOG_LEVEL_FINEST ? MamaLogLevel::MAMA_LOG_LEVEL_FINEST
                                                                               : newLevel));
    } else if (name == OPTION_NAME_DECREASE_VERBOSITY) {
        logger.setLevel(logger.getLevel() - 1);
        int newLevel = (int) Mama::getLogLevel() - 1;
        Mama::setLogLevel((MamaLogLevel) (newLevel < MamaLogLevel::MAMA_LOG_LEVEL_OFF ? MamaLogLevel::MAMA_LOG_LEVEL_OFF
                                                                                      : newLevel));
    } else if (name == OPTION_NAME_OPENMAMA_MIDDLEWARE) {
    } else if (name == OPTION_NAME_OPENMAMA_SOURCE) {
    } else if (name == OPTION_NAME_OPENMAMA_TRANSPORT) {
    } else if (name == OPTION_NAME_OPENMAMA_PAYLOAD) {
    }
}

void SubsystemOpenMama::uninitialize() {
    logger.debug("Stopping the OpenMAMA subsystem");
}

void SubsystemOpenMama::reinitialize(Poco::Util::Application &app) {
    logger.debug("Reinitializing the OpenMAMA subsystem");
    Subsystem::reinitialize(app);
}

void SubsystemOpenMama::defineOptions(Poco::Util::OptionSet &options) {

    Option middleware(OPTION_NAME_OPENMAMA_MIDDLEWARE, "m",
            "OpenMAMA middleware. May be passed multiple times.");
    middleware.required(false).repeatable(true).argument("STR");
    options.addOption(middleware);

    Option transport(OPTION_NAME_OPENMAMA_TRANSPORT, "t",
            "OpenMAMA configTransport. May be passed multiple times.");
    transport.required(false).repeatable(true).argument("STR");
    options.addOption(transport);

    Option source(OPTION_NAME_OPENMAMA_SOURCE, "S",
            "OpenMAMA source / prefix. May be passed multiple times.");
    source.required(false).repeatable(true).argument("STR");
    options.addOption(source);

    Option payload(OPTION_NAME_OPENMAMA_PAYLOAD, "p",
            "OpenMAMA payload bridge. May be passed multiple times.");
    payload.required(false).repeatable(true).argument("STR");
    options.addOption(payload);

    Option numQueues(OPTION_NAME_OPENMAMA_QUEUES,
            "Number of queues to create in OpenMAMA subsystem.");
    numQueues.required(false).repeatable(true).argument("NUM");
    options.addOption(numQueues);

    Subsystem::defineOptions(options);
}

SubsystemOpenMama::SubsystemOpenMama() :
logger(Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA)),
dictionarySemaphore(1),
dictionary(nullptr)
{
    dictionarySemaphore.wait();
}

OpenMamaStoreMessageListener*
SubsystemOpenMama::getOpenMamaStoreMessageListener(const std::string &symbol) {
    ScopedLock<Mutex> scopedLock(listenerMapMutex);
    auto existingIt = openMamaStoreMessageListenerBySymbol.find(symbol);
    OpenMamaStoreMessageListener* result = nullptr;
    // If there is no current record for this symbol, create it
    if (existingIt == openMamaStoreMessageListenerBySymbol.end()) {
        logger.information("Creating new listener for %s", symbol);
        result = new OpenMamaStoreMessageListener(this);
        auto * subscription = new MamdaSubscription();
        subscription->addMsgListener((MamdaMsgListener*)result);
        subscription->addErrorListener((MamdaErrorListener*)result);
        subscription->addQualityListener((MamdaQualityListener*)result);
        subscription->setRequireInitial(true);

        // Sadly the only current way to distinguish
        if(symbol.c_str()[0] == 'b') {
            subscription->setType(MAMA_SUBSC_TYPE_BOOK);
        }
        result->setSubscription(subscription);
        subscription->create(queueGroup->getNextQueue(), source, symbol.c_str(), (void*)this);
        openMamaStoreMessageListenerBySymbol[symbol] = result;
    } else {
        logger.information("Reusing existing listener for %s", symbol);
        result = openMamaStoreMessageListenerBySymbol[symbol];
    }
    return result;
}

std::string
SubsystemOpenMama::getSnapshotAsJson(const std::string &symbol) {
    if (nullptr == dictionary) {
        logger.error("Cannot acquire a snapshot - do not yet have a dictionary");
        return string();
    }

    // The below will happen in a function call rather than here
    logger.debug("Getting message listener for %s", symbol);
    auto* listener = getOpenMamaStoreMessageListener(symbol);

    logger.debug("Getting snapshot from listener for %s", symbol);
    MamaMsg* snapshot = listener->getSnapshot();

    logger.debug("Getting underlying message for %s", symbol);
    mamaMsg msg = snapshot->getUnderlyingMsg();

    logger.debug("Getting json string for %s", symbol);
    string snapshotAsJson = string(mamaMsg_toJsonStringWithDictionary(msg, dictionary->getDictC()));

    // Clean up the snapshot message - we own this memory
    delete snapshot;

    return snapshotAsJson;
}

SubsystemOpenMama::~SubsystemOpenMama() {
    logger.debug("Destroying OpenMAMA subsystem");
    delete dictionary;

    // Stop dispatching (blocks until complete)
    if (nullptr != bridge) {
        // Stop the application instance
        Mama::stop(bridge);

        // Destroy the queue group, this must be done after everything using the queues has been destroyed
        if (queueGroup != NULL)
        {
            // Destroy all the queues and wait until everything has been cleaned up
            queueGroup->stopDispatch();
            queueGroup->destroyWait();
            delete queueGroup;
            queueGroup = NULL;
        }

        // Clean up after MAMA
        Mama::close();
    }
}

Wombat::MamaDictionary *SubsystemOpenMama::getDictionary() {
    if (nullptr == this->dictionary) {
        logger.information("Waiting for dictionary...");
        dictionarySemaphore.wait();
        logger.information("Dictionary obtained");
    }
    return this->dictionary;
}
