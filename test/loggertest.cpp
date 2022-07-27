#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LOGCPLUS_TESTS

#include <boost/test/unit_test.hpp>
#include <regex>

#include "predefinedpollingconditions.h"
#include "testsfixture.h"
#include "logcplus.h"

namespace dev::marcinromanowski {

    inline static logcplus::LogManager* LOG_MANAGER = logcplus::LogManager::instance();

    BOOST_AUTO_TEST_CASE(logHeaderWithLogLevelAndDateShouldBeIncludedToOutput)
    {
        // setup
        auto coutHandler = redirectStdOutToTemporaryFile("logHeaderShouldBeIncludedToOutput");

        // given
        LOG_MANAGER->disableFileWatcher();
        LOG_MANAGER->disableDirectoryWatcher();
        LOG_MANAGER->setLogLevel(logcplus::Logger::LogLevel::Info);
        LOG_MANAGER->initialize();

        // when
        auto logger = logcplus::LogManager::getLogger();
        logger->info("Test log");

        // then
        std::vector<std::string> logs;
        BOOST_CHECK_NO_THROW(PredefinedPollingConditions::WAIT.eventually([&logs]() -> bool {
            logs = getLogsFromFile("logHeaderShouldBeIncludedToOutput");
            return logs.size() == 1;
        }));

        delete coutHandler;
        auto logRegex = std::regex(R"(^\[INFO\] \d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} - Test log$)");
        BOOST_CHECK(std::regex_match(logs[0], logRegex));
    }

}
