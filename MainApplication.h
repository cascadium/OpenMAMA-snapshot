//
// Created by fquinn on 22/07/2018.
//

#ifndef OPENMAMA_SNAPSHOT_APPLICATIONGOVERNOR_H
#define OPENMAMA_SNAPSHOT_APPLICATIONGOVERNOR_H

#define MAIN_APPLICATION_NAME "MainApplication"

#include <Poco/Util/ServerApplication.h>

/***
 * Application governors are responsible for encapsulating multiple application runners and routing things like
 * admin commands and external signals.
 */
namespace cascadium {
    class MainApplication: public Poco::Util::ServerApplication {
    public:
        const char *name() const override;

        MainApplication();

    protected:
        void initialize(Application &self) override;

        void defineOptions(Poco::Util::OptionSet &options) override;

        void uninitialize() override;

        void reinitialize(Application &self) override;

        void handleOption(const std::string &name, const std::string &value) override;

        int main(const std::vector<std::string> &args) override;

        static void signalHandler(int signum);

        void displayUsage(int exitCode);
    };
}


#endif //OPENMAMA_SNAPSHOT_APPLICATIONGOVERNOR_H
