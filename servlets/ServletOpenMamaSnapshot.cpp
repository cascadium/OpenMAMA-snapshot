//
// Created by fquinn on 19/08/2018.
//

#include <Poco/URI.h>
#include <Poco/Util/Application.h>
#include "ServletOpenMamaSnapshot.h"
#include "../subsystems/SubsystemServletRouter.h"
#include "../subsystems/SubsystemOpenMama.h"

using namespace cascadium;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

void ServletOpenMamaSnapshot::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    Application& app = Application::instance();
    const string& uri = request.getURI();

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

    auto& oms = app.getSubsystem<SubsystemOpenMama>();

    string jsonSnapshot = oms.getSnapshotAsJson(pathSegments[1]);

    if (jsonSnapshot.empty()) {
        response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }

    response.setChunkedTransferEncoding(true);
    response.setContentType("text/html");

    response.send() << jsonSnapshot;
}
