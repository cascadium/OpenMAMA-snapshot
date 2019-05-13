//
// Created by fquinn on 19/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_SERVLETOPENMAMASERVERSENTEVENTS_H
#define OPENMAMA_SNAPSHOT_SERVLETOPENMAMASERVERSENTEVENTS_H

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace cascadium {
class ServletOpenMamaServerSentEvents : public Poco::Net::HTTPRequestHandler {
        void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) override;
    };

};


#endif //OPENMAMA_SNAPSHOT_SERVLETOPENMAMASERVERSENTEVENTS_H
