//
// Created by fquinn on 17/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_OPENMAMASTOREMESSAGELISTENER_H
#define OPENMAMA_SNAPSHOT_OPENMAMASTOREMESSAGELISTENER_H

#include <Poco/Logger.h>
#include <Poco/Semaphore.h>
#include "subsystems/SubsystemOpenMama.h"
#include <mamda/MamdaSubscription.h>
#include <mamda/MamdaErrorListener.h>
#include <mamda/MamdaMsgListener.h>
#include <mamda/MamdaQualityListener.h>
#include <mama/fieldcache/MamaFieldCache.h>
#include <mamda/MamdaOrderBook.h>
#include <mamda/MamdaOrderBookListener.h>

namespace cascadium {
    class OpenMamaStoreMessageListener
            : public Wombat::MamdaMsgListener, Wombat::MamdaErrorListener, Wombat::MamdaQualityListener {
    public:
        explicit OpenMamaStoreMessageListener(SubsystemOpenMama *subsystemOpenMamaStore,
                mamaPayloadBridge defaultPayloadBridge);

        ~OpenMamaStoreMessageListener() override;

        void onMsg(Wombat::MamdaSubscription *subscription, const Wombat::MamaMsg &msg, short msgType) override;

        void onError(Wombat::MamdaSubscription *subscription,
                     Wombat::MamdaErrorSeverity severity,
                     Wombat::MamdaErrorCode errorCode,
                     const char *errorStr) override;

        void onQuality(Wombat::MamdaSubscription *subscription, mamaQuality quality) override;

        Wombat::MamaMsg* getSnapshot();

        void setSubscription(Wombat::MamdaSubscription* subscription);

        Wombat::MamdaSubscription* getSubscription();

    private:
        Poco::Logger &logger;
        SubsystemOpenMama *subsystemOpenMamaStore;
        Poco::Semaphore initialResponseSemaphore;
        Wombat::MamaFieldCache* lastValueCache;
        Wombat::MamdaOrderBook* orderBookCache;
        Wombat::MamdaOrderBookListener* orderBookListener;
        bool hasReceivedSnapshot;
        bool isOrderBookSubscription;
        Wombat::MamdaSubscription* subscription;
        mamaPayloadBridge defaultPayloadBridge;
    };
}


#endif //OPENMAMA_SNAPSHOT_OPENMAMASTOREMESSAGELISTENER_H
