//
// Created by fquinn on 19/08/2018.
//

#include "ServletRequestHandlerFactory.h"
#include "ServletOpenMamaSnapshot.h"
#include <iostream>
#include <Poco/Net/HTTPResponse.h>

using namespace cascadium;
using namespace Poco;
using namespace Poco::Net;
using namespace std;

class ServletReturnHttpStatus: public HTTPRequestHandler {
public:
    explicit ServletReturnHttpStatus(HTTPResponse::HTTPStatus status) : HTTPRequestHandler() {
        this->status = status;
    }

    void handleRequest(HTTPServerRequest &request, HTTPServerResponse &response) override {
        response.setStatus(status);
        response.send() << "Oops - no known endpoint for " << request.getURI() << endl;
    }
private:
    HTTPResponse::HTTPStatus status;
};

HTTPRequestHandler* ServletRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
    if (0 == icompare(std::string("/snapshots"), request.getURI().substr(0,10)))
        return new ServletOpenMamaSnapshot;
    else
        return new ServletReturnHttpStatus(HTTPResponse::HTTP_NOT_FOUND);
}
