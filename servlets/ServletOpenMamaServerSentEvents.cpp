//
// Created by fquinn on 19/08/2018.
//

#include <Poco/URI.h>
#include <Poco/Util/Application.h>
#include "ServletOpenMamaServerSentEvents.h"
#include "../subsystems/SubsystemServletRouter.h"
#include "../subsystems/SubsystemOpenMama.h"

using namespace cascadium;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

void ServletOpenMamaServerSentEvents::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    Application& app = Application::instance();
    const string& uri = request.getURI();
    app.logger().notice("Handling a %s request for %s", request.getMethod(), uri);

    URI uriParser(uri);
    const string& path = uriParser.getPath();
    URI::QueryParameters queryParameters = uriParser.getQueryParameters();
    vector<string> pathSegments;
    uriParser.getPathSegments(pathSegments);
    if (pathSegments.size() < 2) {
        app.logger().warning("No valid symbol found in URL string '%s'", path);
        return;
    }
    string symbol = pathSegments[1];

    app.logger().information("Getting snapshot for the symbol '%s' for HTTP client '%s'",
            symbol, request.clientAddress().toString());

    response.add("Content-Type", "text/event-stream");
    response.add("Cache-Control", "no-cache");
    ostream& out = response.send();

    auto& oms = app.getSubsystem<SubsystemOpenMama>();
    try {
        string oldSnapshot;
        long eventCount = 0;
        long oldEventCount = 0;
        while (out.good())
        {
            string jsonSnapshot = oms.getSnapshotAsJson(pathSegments[1], eventCount);
            if (jsonSnapshot.empty()) {
                Poco::Thread::sleep(100);
                continue;
            } else if (oldEventCount > 0 && oldEventCount == eventCount) {
                app.logger().information("No change since last update - skipping...");
                Poco::Thread::sleep(100);
                continue;
            }
            app.logger().information("Sending %d response [%s] to HTTP client '%s'",
                                     (int)eventCount, jsonSnapshot, request.clientAddress().toString());

            out << "data: " << jsonSnapshot << "\n\n";
            out.flush();
            Poco::Thread::sleep(10);
        }
    } catch (NotFoundException& notFoundException) {
        app.logger().error("Could not get snapshot for %s: %s", symbol, notFoundException.message());
        response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
        response.send() << "Failed to retrieve response for " << symbol << ": " << notFoundException.message() << endl;
    }
}


