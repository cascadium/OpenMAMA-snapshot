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

void SubsystemOpenMama::initialize(Poco::Util::Application &app) {
    logger.debug("Initializing the store subsystem");
    mama_setLogCallback2(setUpSubsystemOpenMamaLogCb);
    bridge = Mama::loadBridge("zmq");
    logger.information("Application has loaded %s bridge version: %s",
            string("zmq"),
            string(Mama::getVersion(bridge)));
    Mama::open();
    source = new MamaSource("OPENMAMA", "sub", "OPENMAMA", bridge, true);
    OpenMamaDictionaryRequestor dictRequester (bridge);
    dictionary = dictRequester.requestDictionary (new MamaSource("WOMBAT", "sub", "WOMBAT", bridge));
    // Wait for first snapshot to land
    dictionarySemaphore.set();
    Mama::start(bridge);
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
    cerr << "Unitializing subsystem" << endl;
}

void SubsystemOpenMama::reinitialize(Poco::Util::Application &app) {
    Subsystem::reinitialize(app);
}

void SubsystemOpenMama::defineOptions(Poco::Util::OptionSet &options) {

    Option middleware(OPTION_NAME_OPENMAMA_MIDDLEWARE, "m",
            "OpenMAMA middleware. May be passed multiple times.");
    middleware.required(false).repeatable(true).argument("STR");
    options.addOption(middleware);

    Option transport(OPTION_NAME_OPENMAMA_TRANSPORT, "t",
            "OpenMAMA transport. May be passed multiple times.");
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

    Subsystem::defineOptions(options);
}

SubsystemOpenMama::SubsystemOpenMama() :
logger(Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA)),
dictionarySemaphore(1)
{
    dictionarySemaphore.wait();
}

OpenMamaStoreMessageListener*
SubsystemOpenMama::getOpenMamaStoreMessageListener(const std::string &symbol) {
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
        subscription->setTimeout(3.0);
        subscription->setRequireInitial(true);
        if(symbol.c_str()[0] == 'b') {
            logger.information("Reusing existing listener for %s", symbol);
            subscription->setType(MAMA_SUBSC_TYPE_BOOK);
        }
        subscription->create(Mama::getDefaultEventQueue(bridge), source, symbol.c_str(), (void*)this);
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
    auto* listener = getOpenMamaStoreMessageListener(symbol);
    MamaMsg* snapshot = listener->getSnapshot();
    mamaMsg msg = snapshot->getUnderlyingMsg();
    string snapshotAsJson = string(mamaMsg_toJsonStringWithDictionary(msg, dictionary->getDictC()));
    delete snapshot;
    return snapshotAsJson;
}

SubsystemOpenMama::~SubsystemOpenMama() {

    cerr << "IN DESCRUCTOR FOR OPENMAMA" << endl;
    // Stop dispatch
    if (nullptr != bridge) {
        Mama::stop(bridge);
    } else {
        return;
    }

//    if (mQueueGroup != NULL)
//    {
//        mQueueGroup->stopDispatch();
//    }

    delete dictionary;

    class : public MamaSubscriptionIteratorCallback {
    public:
        void onSubscription(MamaSource *source, MamaSubscription *subscription, void *closure) override {
            cerr << "Removing subscription to topic: " << subscription->getSymbol() << endl;
            subscription->destroy();
            delete subscription;
        }
    } subscriptionScrubber;
    cerr << "Iterating subscriptions" << endl;
    source->forEachSubscription(&subscriptionScrubber, nullptr);
    MamaTransport* transport = source->getTransport();
    delete transport;
    delete source;

//    // Destroy the queue group, this must be done after everything using the queues has been destroyed
//    if (mQueueGroup != NULL)
//    {
//        // Destroy all the queues and wait until everything has been cleaned up
//        mQueueGroup->destroyWait();
//        delete mQueueGroup;
//        mQueueGroup = NULL;
//    }
//
//    if ((mDictTransport !=  NULL)  && (mDictTransport  != mTransport))
//    {
//        delete mDictTransport;
//        mDictTransport = NULL;
//    }
//
//    if (mTransport != NULL)
//    {
//        delete mTransport;
//        mTransport = NULL;
//    }

    Mama::close();
}

Wombat::MamaDictionary *SubsystemOpenMama::getDictionary() {
    if (nullptr == this->dictionary) {
        logger.information("Waiting for dictionary...");
        dictionarySemaphore.wait();
    }
    return this->dictionary;
}
