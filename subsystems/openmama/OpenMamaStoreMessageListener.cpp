//
// Created by fquinn on 17/08/2018.
//

#include <mutex>
#include <iostream>
#include "OpenMamaStoreMessageListener.h"
#include "../SubsystemOpenMama.h"
#include <mama/status.h>
#include <mama/msg.h>
#include <mamda/MamdaOrderBookHandler.h>
#include <mamda/MamdaOrderBookFields.h>

using namespace Wombat;
using namespace std;
using namespace cascadium;
using namespace Poco;

class OpenMamaStoreMessageListenerOrderBookHandler: public MamdaOrderBookHandler {
public:
    void onBookRecap(MamdaSubscription *subscription, MamdaOrderBookListener &listener, const MamaMsg *msg,
                     const MamdaOrderBookComplexDelta *delta, const MamdaOrderBookRecap &event,
                     const MamdaOrderBook &book) override {

    }

    void onBookDelta(MamdaSubscription *subscription, MamdaOrderBookListener &listener, const MamaMsg *msg,
                     const MamdaOrderBookSimpleDelta &event, const MamdaOrderBook &book) override {

    }

    void onBookComplexDelta(MamdaSubscription *subscription, MamdaOrderBookListener &listener, const MamaMsg *msg,
                            const MamdaOrderBookComplexDelta &event, const MamdaOrderBook &book) override {

    }

    void onBookClear(MamdaSubscription *subscription, MamdaOrderBookListener &listener, const MamaMsg *msg,
                     const MamdaOrderBookClear &event, const MamdaOrderBook &book) override {

    }

    void onBookGap(MamdaSubscription *subscription, MamdaOrderBookListener &listener, const MamaMsg *msg,
                   const MamdaOrderBookGap &event, const MamdaOrderBook &book) override {

    }
};

OpenMamaStoreMessageListener::OpenMamaStoreMessageListener(SubsystemOpenMama* subsystemOpenMamaStore) :
        logger(Poco::Logger::get(SUBSYSTEM_NAME_OPEN_MAMA))
        , initialResponseSemaphore(1)
        , hasReceivedSnapshot(false)
        , isOrderBookSubscription(false)
{
    lastValueCache = new MamaFieldCache();
    lastValueCache->setUseLock(true);
    lastValueCache->create();
    // Decrement initial value from 1 immediately
    initialResponseSemaphore.wait();
    MamdaOrderBookFields::setDictionary(*subsystemOpenMamaStore->getDictionary());
    orderBookCache = new MamdaOrderBook();
    orderBookCache->generateDeltaMsgs(true);
    orderBookListener = new MamdaOrderBookListener(orderBookCache);
    orderBookListener->addHandler(new OpenMamaStoreMessageListenerOrderBookHandler());
    this->subsystemOpenMamaStore = subsystemOpenMamaStore;
}

void OpenMamaStoreMessageListener::onMsg(MamdaSubscription *subscription, const MamaMsg &msg, short msgType) {
    switch(msgType) {
        case MAMA_MSG_TYPE_INITIAL:
        case MAMA_MSG_TYPE_RECAP:
        case MAMA_MSG_TYPE_SNAPSHOT:
            lastValueCache->apply(msg, subsystemOpenMamaStore->getDictionary());
            logger.debug("Level 1 full image [%d] received", (int) msgType);
            hasReceivedSnapshot = true;
            initialResponseSemaphore.set();
            break;
        case MAMA_MSG_TYPE_UPDATE:
        case MAMA_MSG_TYPE_CANCEL:
        case MAMA_MSG_TYPE_ERROR:
        case MAMA_MSG_TYPE_CORRECTION:
        case MAMA_MSG_TYPE_CLOSING:
        case MAMA_MSG_TYPE_EXPIRE:
        case MAMA_MSG_TYPE_PREOPENING:
        case MAMA_MSG_TYPE_QUOTE:
        case MAMA_MSG_TYPE_TRADE:
        case MAMA_MSG_TYPE_ORDER:
        case MAMA_MSG_TYPE_SEC_STATUS:
            lastValueCache->apply(msg, subsystemOpenMamaStore->getDictionary());
            logger.debug("Level 1 update [%d] received", (int)msgType);
            break;
        case MAMA_MSG_TYPE_BOOK_INITIAL:
        case MAMA_MSG_TYPE_BOOK_RECAP:
            orderBookListener->onMsg(subscription, msg, msgType);
            logger.debug("Level 2 full image [%d] received", (int)msgType);
            isOrderBookSubscription = true;
            hasReceivedSnapshot = true;
            initialResponseSemaphore.set();
            break;
        case MAMA_MSG_TYPE_BOOK_UPDATE:
        case MAMA_MSG_TYPE_BOOK_CLEAR:
        case MAMA_MSG_TYPE_BOOK_SNAPSHOT:
            orderBookListener->onMsg(subscription, msg, msgType);
            logger.debug("Level 2 update [%d] received", (int)msgType);
            break;
        case MAMA_MSG_TYPE_NOT_PERMISSIONED:
        case MAMA_MSG_TYPE_NOT_FOUND:
        case MAMA_MSG_TYPE_END_OF_INITIALS:
        case MAMA_MSG_TYPE_WOMBAT_REQUEST:
        case MAMA_MSG_TYPE_WOMBAT_CALC:
        case MAMA_MSG_TYPE_DDICT_SNAPSHOT:
        case MAMA_MSG_TYPE_TIBRV:
        case MAMA_MSG_TYPE_FEATURE_SET:
        case MAMA_MSG_TYPE_SYNC_REQUEST:
        case MAMA_MSG_TYPE_REFRESH:
        case MAMA_MSG_TYPE_WORLD_VIEW:
        case MAMA_MSG_TYPE_NEWS_QUERY:
        case MAMA_MSG_TYPE_NULL:
        case MAMA_MSG_TYPE_ENTITLEMENTS_REFRESH:
        case MAMA_MSG_TYPE_UNKNOWN:
        default:
            logger.warning("Unexpected message type [%d]: ignoring update", (int)msgType);
            break;
    }
}

void OpenMamaStoreMessageListener::onError(MamdaSubscription *subscription, MamdaErrorSeverity severity, MamdaErrorCode errorCode,
             const char *errorStr) {
    logger.debug("In the MAMDA subscription callback onError");
}

void OpenMamaStoreMessageListener::onQuality(MamdaSubscription *subscription, mamaQuality quality) {
    logger.debug("In the MAMDA subscription callback onQuality");
}

MamaMsg* OpenMamaStoreMessageListener::getSnapshot() {
    auto *msg = new MamaMsg();
    mamaPayloadBridge plb = Mama::loadPayloadBridge("qpidmsg");
    msg->createForPayloadBridge(plb);
    if (!hasReceivedSnapshot) {
        // Wait for first snapshot to land
        initialResponseSemaphore.wait();
    }

    if (isOrderBookSubscription) {
        logger.debug("Is order book subscription");
        MamdaOrderBook* orderBook = orderBookListener->getOrderBook();
        orderBook->populateRecap(*msg);
        logger.debug("Dump has completed...");
    } else {
        logger.debug("Is level 1 subscription");
        lastValueCache->getFullMessage(*msg);
    }

    return msg;
}

void OpenMamaStoreMessageListener::setSubscription(MamdaSubscription* subscription) {
    this->subscription = subscription;
}

MamdaSubscription* OpenMamaStoreMessageListener::getSubscription() {
    return this->subscription;
}

OpenMamaStoreMessageListener::~OpenMamaStoreMessageListener() {
    delete lastValueCache;
    delete orderBookCache;
    delete orderBookListener;
}
