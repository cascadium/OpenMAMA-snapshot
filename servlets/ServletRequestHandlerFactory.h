//
// Created by fquinn on 19/08/2018.
//

#ifndef OPENMAMA_SNAPSHOT_SERVLETREQUESTHANDLERFACTORY_H
#define OPENMAMA_SNAPSHOT_SERVLETREQUESTHANDLERFACTORY_H

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>

namespace cascadium {
class ServletRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
        Poco::Net::HTTPRequestHandler *createRequestHandler(const Poco::Net::HTTPServerRequest &request) override;
    };
};


#endif //OPENMAMA_SNAPSHOT_SERVLETREQUESTHANDLERFACTORY_H
