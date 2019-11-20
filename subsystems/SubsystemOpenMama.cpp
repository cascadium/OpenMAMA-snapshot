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
#include <Poco/NumberParser.h>
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
#include "../MainApplication.h"


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

    // Clear state tracking containers
    payloadBridges.clear();
    openMamaStoreMessageListenerBySymbol.clear();
    defaultPayloadBridge = nullptr;

    // Initialize log levels
    switch(configVerbosity) {
        case -5:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_OFF);
            logger.setLevel("none");
            break;
        case -4:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_OFF);
            logger.setLevel("fatal");
            break;
        case -3:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_OFF);
            logger.setLevel("critical");
            break;
        case -2:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_SEVERE);
            logger.setLevel("error");
            break;
        case -1:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_ERROR);
            logger.setLevel("warning");
            break;
        case 0:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_WARN);
            logger.setLevel("notice");
            break;
        case 1:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_NORMAL);
            logger.setLevel("information");
            break;
        case 2:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_FINE);
            logger.setLevel("debug");
            break;
        case 3:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_FINER);
            logger.setLevel("trace");
            break;
        case 4:
            Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_FINEST);
            logger.setLevel("trace");
            break;
        default:
            if (configVerbosity > 0) {
                Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_FINEST);
                logger.setLevel("trace");
            } else {
                Mama::setLogLevel(MamaLogLevel::MAMA_LOG_LEVEL_OFF);
                logger.setLevel("none");
            }
            break;
    }

    // Load the OpenMAMA middleware bridge
    try {
        logger.information("Loading OpenMAMA middleware bridge '%s'", configMiddleware);
        bridge = Mama::loadBridge(configMiddleware.c_str());
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot load '%s' middleware bridge (%s)",
                     configMiddleware, string(status.toString()));
        return;
    }

    // Load the OpenMAMA payload bridges
    for (auto& payload: configPayloads) {
        try {
            logger.information("Loading %s payload bridge", payload);
            mamaPayloadBridge payloadBridge = Mama::loadPayloadBridge(payload.c_str());
            payloadBridges.insert(payloadBridge);
            if (nullptr == defaultPayloadBridge) {
                defaultPayloadBridge = payloadBridge;
            }

        } catch (MamaStatus &status) {
            logger.information("Failed to load '%s' payload bridge", payload);
        }
    }

    // If no payload bridges loaded successfully, this is a fatal error
    if (payloadBridges.empty()) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot find or load any payload bridges");
        return;
    }

    // Call OpenMAMA open
    try {
        logger.information("Opening main OpenMAMA resources");
        Mama::open();
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot open OpenMAMA resources (%s)",
                string(status.toString()));
        return;
    }

    // Fetch the dictionary
    try {
        logger.information("Initializing the OpenMAMA dictionary transport %s", configDictionaryTransport);
        auto * dictionaryTransport = new MamaTransport();
        dictionaryTransport->create(configDictionaryTransport.c_str(), bridge);

        logger.information("Initializing the OpenMAMA dictionary source %s", configDictionarySource);
        auto * dictionaryMamaSource = new MamaSource(configDictionarySource.c_str(), dictionaryTransport,
                configDictionarySource.c_str());
        OpenMamaDictionaryRequestor dictRequester (bridge);

        // Go fetch the dictionary
        logger.information("Fetching the OpenMAMA dictionary");
        dictionary = dictRequester.requestDictionary (dictionaryMamaSource);

        // Clean up after retrieving dictionary
        logger.information("Cleaning up temporary OpenMAMA dictionary resources");
        delete dictionaryMamaSource;
        delete dictionaryTransport;

        // Throw exception if request timed out without a response
        if (nullptr == dictionary) {
            throw Wombat::MamaStatus(MAMA_STATUS_NOT_FOUND);
        }

        // Getting this far means success - mark the semaphore as non-zero to signify dictionary is ready
        dictionarySemaphore.set();
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot find dictionary (%s)", string(status.toString()));
        return;
    }

    // Create the OpenMAMA source
    try {
        logger.information("Creating the OpenMAMA subscription source transport: %s, source: %s",
                configTransport, configSourceName);
        source = new MamaSource(configSourceName.c_str(), configTransport.c_str(), configSourceName.c_str(), bridge,
                true);
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot initialize requested OpenMAMA data source (%s)",
                string(status.toString()));
        return;
    }

    // Create the queue group
    try {
        logger.information("Creating an OpenMAMA queue group with %u queues", configNumQueues);
        queueGroup = new MamaQueueGroup(configNumQueues, bridge);
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot initialize OpenMAMA Queue group (%s)",
                     string(status.toString()));
        return;
    }

    // Fire up OpenMAMA
    try {
        logger.information("Starting dispatch for OpenMAMA on %u queues", configNumQueues);
        Mama::startBackground(bridge, this);
        logger.notice("OpenMAMA initialized - ready for requests", configNumQueues);
    } catch (MamaStatus& status) {
        logger.fatal("Failed to initialize SubsystemOpenMama: Cannot start OpenMAMA (%s)",
                     string(status.toString()));
        return;
    }

}

void SubsystemOpenMama::handleOption(const string &name, const string &value) {
    if (name == OPTION_NAME_INCREASE_VERBOSITY) {
        configVerbosity++;
    } else if (name == OPTION_NAME_DECREASE_VERBOSITY) {
        configVerbosity--;
    } else if (name == OPTION_NAME_OPENMAMA_MIDDLEWARE) {
        configMiddleware = value;
    } else if (name == OPTION_NAME_OPENMAMA_SOURCE) {
        configSourceName = value;
    } else if (name == OPTION_NAME_OPENMAMA_TRANSPORT) {
        configTransport = value;
    } else if (name == OPTION_NAME_OPENMAMA_PAYLOAD) {
        configPayloads.insert(value);
    } else if (name == OPTION_NAME_OPENMAMA_QUEUES) {
        configNumQueues = NumberParser::parseUnsigned(value);
    } else if (name == OPTION_NAME_OPENMAMA_DICT_MIDDLEWARE) {
        configDictionaryMiddleware = value;
    } else if (name == OPTION_NAME_OPENMAMA_DICT_SOURCE) {
        configDictionarySource = value;
    } else if (name == OPTION_NAME_OPENMAMA_DICT_TRANSPORT) {
        configDictionaryTransport = value;
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
            "OpenMAMA middleware to use.");
    middleware.required(false).repeatable(false).argument("STR");
    options.addOption(middleware);

    Option transport(OPTION_NAME_OPENMAMA_TRANSPORT, "t",
            "OpenMAMA transport name to use.");
    transport.required(false).repeatable(false).argument("STR");
    options.addOption(transport);

    Option source(OPTION_NAME_OPENMAMA_SOURCE, "S",
            "OpenMAMA source / prefix to use.");
    source.required(false).repeatable(false).argument("STR");
    options.addOption(source);

    Option payload(OPTION_NAME_OPENMAMA_PAYLOAD, "p",
            "OpenMAMA payload bridge. May be passed multiple times.");
    payload.required(false).repeatable(true).argument("STR");
    options.addOption(payload);

    Option numQueues(OPTION_NAME_OPENMAMA_QUEUES, "Q",
            "Number of queues (processing threads) to create in OpenMAMA subsystem.");
    numQueues.required(false).repeatable(true).argument("NUM");
    options.addOption(numQueues);

    Option dictMiddleware(OPTION_NAME_OPENMAMA_DICT_MIDDLEWARE, "dm",
             "OpenMAMA middleware to use for dictionary if different from application's middleware.");
    dictMiddleware.required(false).repeatable(true).argument("STR");
    options.addOption(dictMiddleware);

    Option dictSource(OPTION_NAME_OPENMAMA_DICT_SOURCE, "ds",
             "OpenMAMA source / prefix to use for dictionary (Default: WOMBAT).");
    dictSource.required(false).repeatable(true).argument("STR");
    options.addOption(dictSource);

    Option dictTransport(OPTION_NAME_OPENMAMA_DICT_TRANSPORT, "dt",
             "OpenMAMA transport to use for dictionary if different from application's transport.");
    dictTransport.required(false).repeatable(true).argument("STR");
    options.addOption(dictTransport);

    Subsystem::defineOptions(options);
}

SubsystemOpenMama::SubsystemOpenMama() :
logger(Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA)),
dictionarySemaphore(1),
dictionary(nullptr),
defaultPayloadBridge(nullptr)
{
    // Immediately decrement the semaphore. Must be constructed with non zero so to make zero, need to do here
    dictionarySemaphore.wait();

    // Set default configurable values
    configNumQueues = 1;
    configMiddleware = "zmq";
    configTransport = "sub";
    configSourceName = "OPENMAMA";
    configDictionarySource = "WOMBAT";
    configDictionaryTransport = configTransport;
    configDictionaryMiddleware = configMiddleware;
    configPayloads = {"qpidmsg", "omnmmsg"};
    configVerbosity = 0;

    // Set up MAMA log callbacks now
    mama_setLogCallback2(setUpSubsystemOpenMamaLogCb);
}

OpenMamaStoreMessageListener*
SubsystemOpenMama::getOpenMamaStoreMessageListener(const std::string &symbol) {
    ScopedLock<Mutex> scopedLock(listenerMapMutex);
    auto existingIt = openMamaStoreMessageListenerBySymbol.find(symbol);
    OpenMamaStoreMessageListener* result = nullptr;
    // If there is no current record for this symbol, create it
    if (existingIt == openMamaStoreMessageListenerBySymbol.end()) {
        logger.information("Creating new listener for %s", symbol);
        result = new OpenMamaStoreMessageListener(this, this->defaultPayloadBridge);
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
SubsystemOpenMama::removeOpenMamaStoreMessageListener(const string& symbol) {
    ScopedLock<Mutex> scopedLock(listenerMapMutex);
    auto existingIt = openMamaStoreMessageListenerBySymbol.find(symbol);
    if (existingIt != openMamaStoreMessageListenerBySymbol.end()) {
        openMamaStoreMessageListenerBySymbol.erase(symbol);
    }
}

std::string
SubsystemOpenMama::getSnapshotAsJson(const std::string &symbol, long &eventCount) {
    if (nullptr == dictionary) {
        logger.error("Cannot acquire a snapshot - do not yet have a dictionary");
        return string();
    }

    // The below will happen in a function call rather than here
    logger.debug("Getting message listener for %s", symbol);
    auto* listener = getOpenMamaStoreMessageListener(symbol);

    logger.debug("Getting snapshot from listener for %s", symbol);
    MamaMsg* snapshot = listener->getSnapshot();

    if (nullptr == snapshot) {
        removeOpenMamaStoreMessageListener(symbol);
        delete listener;
        throw NotFoundException("Could not establish a MAMA subscription");
    }

    logger.debug("Getting underlying message for %s", symbol);
    mamaMsg msg = snapshot->getUnderlyingMsg();

    logger.debug("Getting json string for %s", symbol);
    string snapshotAsJson = string(mamaMsg_toJsonStringWithDictionary(msg, dictionary->getDictC()));

    // Clean up the snapshot message - we own this memory
    delete snapshot;

    eventCount = listener->getEventCount();

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

// It is possible this method may be called before the initialization is complete. If so, wait.
Wombat::MamaDictionary *SubsystemOpenMama::getDictionary() {
    if (nullptr == this->dictionary) {
        logger.information("Waiting for dictionary...");
        dictionarySemaphore.wait();
        logger.information("Dictionary obtained");
    }
    return this->dictionary;
}
