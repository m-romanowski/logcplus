#ifndef LOGCPLUS_TESTSFIXTURE_H
#define LOGCPLUS_TESTSFIXTURE_H

#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

namespace dev::marcinromanowski {

    inline static const std::string TEMP_DIRECTORY = std::filesystem::temp_directory_path();

    class StreamRedirection {
    public:
        StreamRedirection(std::ostream& from, const std::string& filename)
            : from{from},
              to{filename, std::ios::trunc},
              saved{from.rdbuf(to.rdbuf())} {

        }

        StreamRedirection(const StreamRedirection&) = delete;

        ~StreamRedirection() {
            from.rdbuf(saved);
        }

        void operator=(const StreamRedirection&) = delete;

    private:
        std::ostream& from;
        std::ofstream to;
        std::streambuf* const saved;
    };

    char directorySeparator() {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }

    StreamRedirection* redirectStdOutToTemporaryFile(const std::string& testFilename) {
        return new StreamRedirection(std::cout, TEMP_DIRECTORY + directorySeparator() + testFilename);
    }

    std::vector<std::string> getLogsFromFile(const std::string& testFilename) {
        std::ifstream inFile(TEMP_DIRECTORY + directorySeparator() + testFilename);
        if (!inFile) {
            throw std::invalid_argument("Cannot open " + testFilename + " file");
        }

        std::vector<std::string> fileLines;
        std::string fileLine;
        while (getline(inFile, fileLine)) {
            fileLines.push_back(fileLine);
        }

        return fileLines;
    }

}

#endif // LOGCPLUS_TESTSFIXTURE_H
