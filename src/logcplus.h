#ifndef LOGCPLUS_LOGCPLUS_H
#define LOGCPLUS_LOGCPLUS_H

#include <queue>
#include <cassert>
#include <any>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <atomic>
#include <thread>
#include <regex>
#include <mutex>
#include <optional>
#include <condition_variable>

#define LOG_FILE_FORMAT "\\d{4}[-]\\d{2}[-]\\d{2}.log.\\d"

inline static std::string const& to_string(std::string const& _str) { return _str; }

/*
 * Log level pyramid:
 *
 *          DEBUG    INFO    WARN    ERROR   FATAL
 *
 * DEBUG      x       x       x        x       x
 *
 * INFO               x       x        x       x
 *
 * WARN                       x        x       x
 *
 * ERROR                               x       x
 *
 * FATAL                                       x
 *
 * [LEGEND]:
 *  column - defined log levels
 *  row - the message levels that will be printed by the logger
 */
namespace dev::marcinromanowski::logcplus {
    /**
     * @brief ConcurrentQueue is a simple thread safety wrapper around queue.
     *
     * Reference:
     *  https://en.cppreference.com/w/cpp/language/cv
     *  https://en.cppreference.com/w/cpp/thread/condition_variable
     */
    template<class T>
    class ConcurrentQueue {
        mutable std::mutex mutex_;
        std::queue<T> queueContainer_;
        std::condition_variable conditionVariable_;

    public:
        /**
         * @brief Get current head item without remove it.
         * @return Head value stored in a queue.
         */
        T take() {
            std::lock_guard<std::mutex> lock(mutex_);
            return queueContainer_.front();
        }

        /**
         * @brief Enqueue item into a queue.
         * @param _queueItem Item to insert into the queue.
         */
        void enqueue(T _queueItem) {
            std::lock_guard<std::mutex> lock(mutex_);
            queueContainer_.push(_queueItem);
            conditionVariable_.notify_one();
        }

        /**
         * @brief Remove item (dequeue) from the queue.
         * @return Head item from the queue.
         */
        T dequeue() {
            std::unique_lock<std::mutex> lock(mutex_);

            // If queue is empty we need to wait till a element is available.
            conditionVariable_.wait(lock, [&] {
                return !queueContainer_.empty();
            });

            T queueItem = queueContainer_.front();
            queueContainer_.pop();

            return queueItem;
        }

        /**
         * @brief Removed all items from the queue.
         */
        void clear() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!queueContainer_.empty()) {
                queueContainer_.pop();
            }
        }

        /**
         * @brief Current queue length.
         * @return Size of the queue.
         */
        std::uint64_t length() const {
            return queueContainer_.size();
        }

        /**
         * @brief Checks that queue is currently empty.
         * @return Empty => true, otherwise false.
         */
        bool empty() const {
            return queueContainer_.empty();
        }
    };

    /**
     * @brief The Date class is a simple wrapper around `tm struct` from the ctime library.
     */
    class Date {
        int day_; // Day of month [1; 31]
        int month_; // Month of year [0, 11]
        int year_; // Year since 1900

    public:
        /**
         * @brief The Time struct represent time in format h:m:s.
         */
        struct Time {
            Time(const int _hour = 0, const int _minute = 0, const int _second = 0) {
                this->hour = _hour;
                this->minute = _minute;
                this->second = _second;
            }

            friend bool operator==(const Time& _lhs, const Time& _rhs) {
                return (_lhs.hour == _rhs.hour && _lhs.minute == _rhs.minute && _lhs.second == _rhs.second);
            }

            friend bool operator!=(const Time& _lhs, const Time& _rhs) {
                return !(_lhs == _rhs);
            }

            friend bool operator<(const Time& _lhs, const Time& _rhs) {
                return (_lhs.hour < _rhs.hour && _lhs.minute < _rhs.minute && _lhs.second < _rhs.second);
            }

            friend bool operator>(const Time& _lhs, const Time& _rhs) {
                return (_lhs < _rhs);
            }

            friend bool operator<=(const Time& _lhs, const Time& _rhs) {
                return !(_lhs > _rhs);
            }

            friend bool operator>=(const Time& _lhs, const Time& _rhs) {
                return !(_lhs < _rhs);
            }

            std::string toString() const {
                return addTrailingZero(this->hour) + ":" + addTrailingZero(this->minute) + ":" + addTrailingZero(this->second);
            }

            static std::string toString(const Time _time) {
                return addTrailingZero(_time.hour) + ":" + addTrailingZero(_time.minute) + ":" + addTrailingZero(_time.second);
            }

            int hour; // Hours of day [0; 23]
            int minute; // Minutes of hour [0; 59]
            int second; // Seconds of minutes [0; 60]

        private:
            static std::string addTrailingZero(int _value) {
                return _value >= 0 && _value <= 9 ? "0" + std::to_string(_value) : std::to_string(_value);
            }
        } time_; // Hour, minute and second

        Date() : day_(1), month_(0), year_(1900), time_(0, 0, 0) {

        }

        int hour() const {
            return time_.hour;
        }

        int minute() const {
            return time_.minute;
        }

        int second() const {
            return time_.second;
        }

        Time time() const {
            return time_;
        }

        int day() const {
            return day_;
        }

        int month() const {
            return month_;
        }

        int year() const {
            return year_;
        }

        Date& setSeconds(int _seconds) {
            assert(_seconds >= 0 && _seconds <= 59);

            time_.second = _seconds;
            return *this;
        }

        Date& setMinutes(int _minutes) {
            assert(_minutes >= 0 && _minutes <= 59);

            time_.minute = _minutes;
            return *this;
        }

        Date& setHour(int _hour) {
            assert(_hour >= 0 && _hour <= 23);

            time_.hour = _hour;
            return *this;
        }

        Date& setDay(int _day) {
            assert(_day >= 1 && _day <= 31);

            day_ = _day;
            return *this;
        }

        Date& setMonth(int _month) {
            assert(_month >= 1 && _month <= 12);

            month_ = _month;
            return *this;
        }

        Date& setYear(int _year) {
            assert(_year > 0);

            year_ = _year;
            return *this;
        }

        /**
         * @brief Returns current date.
         * @return Hour, minute, second, day, month and year.
         */
        static Date now() {
            std::time_t now = std::time(0);
            std::tm* localTime = std::localtime(&now);

            Date date;
            date.time_.second = localTime->tm_sec;
            date.time_.minute = localTime->tm_min;
            date.time_.hour = localTime->tm_hour;
            date.day_ = localTime->tm_mday;
            date.month_ = localTime->tm_mon + 1;
            date.year_ = localTime->tm_year + 1900;

            return date;
        }

        /**
         * @brief Returns current time.
         * @return Hour, minute and second.
         */
        static Time currentTime() {
            Date currentDate = now();
            return currentDate.time();
        }

        friend bool operator==(const Date& _lhs, const Date& _rhs) {
            return (_lhs.time_ == _rhs.time_ && _lhs.day_ == _rhs.day_ && _lhs.month_ == _rhs.month_ && _lhs.year_ == _rhs.year_);
        }

        friend bool operator!=(const Date& _lhs, const Date& _rhs) {
            return !(_lhs == _rhs);
        }
    };

    /**
     * @brief
     * ExtendedMap - map container manager. Allows you to manage data in the form of key - value, save
     * these values ​​to a file and read them. Supports string-key types and `any` values from the `std::any` library.
     */
    class EMap {
        std::map<std::string, std::any> values_;

    public:
        /**
         * @brief Adds a new type or set the new value to existing key.
         * @param _key Map key.
         * @param _value Associated value with the key.
         */
        void add(const std::string& _key, const std::any _value) {
            if (containsKey(_key)) {
                set(_key, _value);
            } else {
                values_.insert(std::make_pair(_key, _value));
            }
        }

        /**
         * @brief Changes existing key value.
         * @param _key Map key.
         * @param _value Associated value with the key.
         */
        void set(const std::string& _key, const std::any _value) {
            values_.at(_key) = _value;
        }

        bool containsKey(const std::string& _key) {
            if (std::map<std::string, std::any>::iterator it = values_.find(_key); it != values_.end()) {
                return true;
            }

            return false;
        }

        /**
         * @brief Gets value by key.
         * @param _key Key sought.
         * @return Associated value with the key, otherwise `empty` std::any.
         */
        std::any get(const std::string& _key) const {
            std::any value;

            try {
                value = values_.at(_key);
            } catch (const std::exception& _ex) {
                std::cerr << "EMap: Cannot get map value for " << _key << " key" << std::endl;
                value.reset();
            }

            return value;
        }

        /**
         * @brief Returns all map data.
         */
        std::map<std::string, std::any> data() const {
            return values_;
        }

        /**
         * @brief Clears map container.
         */
        void clear() {
            values_.clear();
        }

        /**
         * @brief Checks the map file exists.
         * @param _filename File where are stored map values.
         * @return True if file exists, otherwise false.
         */
        bool isMapFileExists(const std::string& _filename) {
            return std::filesystem::exists(_filename);
        }

        /**
         * @brief Writes map values to file.
         * @param _filename The file where map data will be stored.
         * @return True if successfully saved to file, otherwise false.
         */
        bool write(const std::string& _filename) {
            bool status = true;
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::badbit);

            try {
                ofs.open(_filename, std::ios::out);

                for (const auto& it: values_) {
                    if (it.second.type() == typeid(std::string)) {
                        ofs << it.first << " " << std::any_cast<std::string>(it.second) << std::endl;
                    } else if (it.second.type() == typeid(int)) {
                        ofs << it.first << " " << std::to_string(std::any_cast<int>(it.second)) << std::endl;
                    } else if (it.second.type() == typeid(bool)) {
                        ofs << it.first << " " << (std::any_cast<bool>(it.second) ? "true" : "false") << std::endl;
                    } else {
                        std::cerr << "EMap: Unexpected value type - " << it.second.type().name() << std::endl;
                        status = false;
                    }
                }
            } catch (const std::ofstream::failure& _ex) {
                std::cerr << "EMap: Unexpected error " << _ex.what() << std::endl;
                status = false;
            }

            if (ofs.is_open()) {
                ofs.close();
            }

            return status;
        }

        /**
         * @brief Appends single key value to the file.
         * @param _filename The file where map data will be stored.
         * @param _key Map key.
         * @return True if successfully saved to file, otherwise false.
         */
        bool append(const std::string& _filename, const std::string& _key) {
            bool status = true;
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::badbit);

            try {
                ofs.open(_filename, std::ios::app);

                if (std::any value = get(_key); value.has_value() && isMapFileExists(_filename)) {
                    if (value.type() == typeid(std::string)) {
                        ofs << _key << " " << std::any_cast<std::string>(value);
                    } else if (value.type() == typeid(int)) {
                        ofs << _key << " " << std::to_string(std::any_cast<int>(value));
                    } else if (value.type() == typeid(bool)) {
                        ofs << _key << " " << (std::any_cast<bool>(value) ? "true" : "false") << std::endl;
                    } else {
                        std::cerr << "EMap: Unexpected value type - " << value.type().name() << std::endl;
                        status = false;
                    }
                } else {
                    std::cerr << "File or key doesn't exist";
                    status = false;
                }
            } catch (const std::ofstream::failure& _ex) {
                std::cerr << "EMap: Unexpected error " << _ex.what() << std::endl;
                status = false;
            }

            if (ofs.is_open()) {
                ofs.close();
            }

            return status;
        }

        /**
         * @brief Reads all map values from the file and loads to local container.
         * @param _filename The file where map data will be stored.
         * @param True if successfully read from file, otherwise false.
         */
        bool read(const std::string& _filename) {
            bool status = true;
            std::ifstream ifs;
            ifs.exceptions(std::ifstream::badbit);

            try {
                ifs.open(_filename, std::ios::out);

                std::string key, value;
                while (ifs >> key >> value) {
                    if (isNumber(value)) {
                        add(key, std::stoi(value));
                    } else if (isBool(value)) {
                        add(key, toBool(value));
                    } else {
                        add(key, value);
                    }
                }
            } catch (const std::ofstream::failure& _ex) {
                std::cerr << "EMap: Unexpected error " << _ex.what() << std::endl;
                status = false;
            }

            return status;
        }

    protected:
        /**
         * @brief Checks given text is a number. Used for parsing map file.
         * @param _str Potentially the number as text.
         * @return True when the text is a number, otherwise false.
         */
        bool isNumber(const std::string& _str) {
            return !_str.empty() && std::find_if(_str.begin(), _str.end(), [](unsigned char c) { return !std::isdigit(c); }) == _str.end();
        }

        /**
         * @brief Checks given text is a bool. We accept ONLY `true` and `false` - not numerical representation `0` or `1`.
         * @param _str Potentially bool as text.
         * @return True when text is compare to the `true` or `false`, otherwise false.
         */
        bool isBool(std::string _str) {
            std::transform(_str.begin(), _str.end(), _str.begin(), ::tolower);
            return _str.compare("true") == 0 || _str.compare("false") == 0;
        }

        /**
         * @brief Converts given text to bool.
         * @param _str Potentially bool as text.
         * @return Converted text to bool.
         * @throw std::invalid_argument When the conversion failed.
         */
        bool toBool(std::string _str) {
            std::transform(_str.begin(), _str.end(), _str.begin(), ::tolower);
            std::istringstream is(_str);

            bool b;
            is >> std::boolalpha >> b;

            // Some error may occured.
            if (!is.good()) {
                throw std::invalid_argument("EMap: Expected bool value");
            }

            return b;
        }
    };

    /**
     * @brief The Timer class allows you to measuring time or execute callback function in intervals.
     */
    template<typename Clock = std::chrono::high_resolution_clock>
    class Timer {
        typedef unsigned long long time_t;

        typename Clock::time_point startPoint_, stopPoint_; // Start / stop snapshots.
        std::atomic_bool execute_; // Is a timer currently working?
        std::thread timerThread_; // The timer processing thread (callback execution).

    public:
        /**
         * Second, minute, hour, day in milliseconds.
         */
        inline static struct TimeToMilliseconds {
            constexpr static time_t MILLISECOND = 1;
            constexpr static time_t SECOND = 1000;
            constexpr static time_t MINUTE = 60000;
            constexpr static time_t HOUR = 3600000;
            constexpr static time_t DAY = 86400000;
            // May no correct precision.
            constexpr static time_t WEEK = 604800000;
            constexpr static time_t MONTH = 2629746000;
            constexpr static time_t YEAR = 31556952000;
        } Milliseconds;

        /**
         * Minute, hour, day in seconds.
         */
        inline static struct TimeToSeconds {
            constexpr static time_t SECOND = 1;
            constexpr static time_t MINUTE = 60;
            constexpr static time_t HOUR = 3600;
            constexpr static time_t DAY = 86400;
            // May no correct precision.
            constexpr static time_t WEEK = 604800;
            constexpr static time_t MONTH = 2629746;
            constexpr static time_t YEAR = 31556952;
        } Seconds;

        Timer() {
            execute_.store(false, std::memory_order_release);
        }

        ~Timer() {
            // If someone forget stop timer manually.
            if (execute_.load(std::memory_order_acquire)) {
                stop();
            }
        }

        /**
         * @brief State of timer.
         * @return If timer is currently running returns true.
         */
        bool isRunning() const {
            return execute_;
        }

        /**
         * @brief Timer start snapshot.
         * @return Start point with `Clock` precision.
         */
        std::chrono::time_point<Clock> startTime() const {
            return startPoint_;
        }

        /**
         * @brief Starts the timer.
         */
        void start() {
            if (execute_.load(std::memory_order_acquire)) {
                return;
            }

            startPoint_ = Clock::now();
        }

        /**
         * @brief Run callback method with specific interval.
         * @param _interval The callback interval.
         * @param _callback The callback method.
         */
        template<typename Rep, typename Period>
        void startInterval(const std::chrono::duration<Rep, Period>& _interval, std::function<void()> _callback) {
            if (execute_.load(std::memory_order_acquire)) {
                stop();
            }

            execute_.store(true, std::memory_order_release);
            timerThread_ = std::thread([=]() {
                while (execute_.load(std::memory_order_acquire)) {
                    if (_callback) {
                        _callback();
                    }

                    std::this_thread::sleep_for(_interval);
                }
            });
        }

        /**
         * @brief Run callback method specific timeout.
         * @param _interval The callback interval.
         * @param _callback The callback method.
         */
        template<typename Rep, typename Period>
        void setTimeout(const std::chrono::duration<Rep, Period>& _interval, std::function<void()> _callback) {
            timerThread_ = std::thread([=]() {
                std::this_thread::sleep_for(_interval);

                if (_callback) {
                    _callback();
                }
            });
        }

        /**
         * @brief Stops timer.
         */
        void stop() {
            stopPoint_ = Clock::now();

            // Wait for thread execution.
            execute_.store(false, std::memory_order_release);
            if (timerThread_.joinable()) {
                timerThread_.join();
            }
        }

        /**
         * @brief Returns current elapsed time (between start and stop snapshot).
         * @return The timer elapsed time.
         */
        template<typename Rep = typename Clock::duration::rep, typename Units = typename Clock::duration>
        Rep elapsedTime() {
            std::atomic_thread_fence(std::memory_order_relaxed);
            auto countedTime = std::chrono::duration_cast<Units>(startPoint_ - stopPoint_).count();
            std::atomic_thread_fence(std::memory_order_relaxed);

            return static_cast<Rep>(countedTime);
        }
    };

    // Predefined timers types.
    using PreciseTimer = Timer<>;
    using SystemTimer = Timer<std::chrono::system_clock>;
    using SteadyTimer = Timer<std::chrono::steady_clock>;

    /**
     * @brief File size representation.
     */
    typedef struct FileSize {
        enum class SizeUnit {
            B = 1, KB = 1000, KiB = 1024, MB = 1000000, MiB = 1048576, GB = 1000000000, GiB = 1073741824
        };

        FileSize() : size(1), unit(SizeUnit::B) {}

        FileSize(const std::uintmax_t _size, const SizeUnit _unit) {
            this->size = _size;
            this->unit = _unit;
        }

        friend bool operator==(const FileSize& _lhs, const FileSize& _rhs) {
            return (_lhs.size == _rhs.size && _lhs.unit == _rhs.unit);
        }

        friend bool operator!=(const FileSize& _lhs, const FileSize& _rhs) {
            return !(_lhs == _rhs);
        }

        friend bool operator<(const FileSize& _lhs, const FileSize& _rhs) {
            return (_lhs.bsize() < _rhs.bsize());
        }

        friend bool operator>(const FileSize& _lhs, const FileSize& _rhs) {
            return (_lhs < _rhs);
        }

        friend bool operator<=(const FileSize& _lhs, const FileSize& _rhs) {
            return !(_lhs > _rhs);
        }

        friend bool operator>=(const FileSize& _lhs, const FileSize& _rhs) {
            return !(_lhs < _rhs);
        }

        /**
         * @brief Returns size in bytes.
         */
        std::uintmax_t bsize() const {
            return this->size * static_cast<std::uintmax_t>(this->unit);
        }

        /**
         * @brief See static `toString` implementation.
         */
        std::string toString() const {
            return FileSize::toString(this->size, this->unit);
        }

        /**
         * @brief Returns FileSize as text.
         * @param _size File size.
         * @param _unit File size unit.
         * @return FileSize as text, e.g. 50MiB
         */
        static std::string toString(std::intmax_t _size, SizeUnit _unit) {
            std::string unit;
            switch (_unit) {
                case SizeUnit::KB:
                    unit = "KB";
                    break;
                case SizeUnit::KiB:
                    unit = "KiB";
                    break;
                case SizeUnit::MB:
                    unit = "MB";
                    break;
                case SizeUnit::MiB:
                    unit = "MiB";
                    break;
                case SizeUnit::GB:
                    unit = "GB";
                    break;
                case SizeUnit::GiB:
                    unit = "GiB";
                    break;
                case SizeUnit::B:
                default:
                    unit = "B";
                    break;
            }

            return std::to_string(_size).append(unit);
        }

        /**
         * @brief Converts text file size format to class format convention.
         * @param _fileSize File size format as text.
         *
         * Expected format: NU,
         * where
         *  - N is std::intmax_t
         *  - U is SizeUnit as text
         *
         * Example: 50MB, 100KiB
         */
        static std::optional<FileSize> parseFileSize(const std::string& _fileSize) {
            FileSize result;

            try {
                // First: check if text has file size as digit.
                std::size_t it = 0;
                for (it = 0; it < _fileSize.size(); it++) {
                    if (!std::isdigit(_fileSize.at(it))) {
                        break;
                    }
                }

                // Second: check if text has file size unit.
                if (it > 0 && it < _fileSize.size()) {
                    int size = std::stoi(_fileSize.substr(0, it));
                    if (size <= 0) {
                        return std::nullopt;
                    }

                    result.size = size;
                    std::string unit = _fileSize.substr(it);

                    if (unit == "B") {
                        result.unit = SizeUnit::B;
                    } else if (unit == "KB") {
                        result.unit = SizeUnit::KB;
                    } else if (unit == "KiB") {
                        result.unit = SizeUnit::KiB;
                    } else if (unit == "MB") {
                        result.unit = SizeUnit::MB;
                    } else if (unit == "MiB") {
                        result.unit = SizeUnit::MiB;
                    } else if (unit == "GB") {
                        result.unit = SizeUnit::GB;
                    } else if (unit == "GiB") {
                        result.unit = SizeUnit::GiB;
                    } else {
                        return std::nullopt;
                    }
                }

                return result;
            } catch (const std::exception& _ex) {
                std::cerr << "logcplus: Cannot parse file size, reason: " << _ex.what() << std::endl;
                return std::nullopt;
            }
        }

        std::uintmax_t size;
        SizeUnit unit;
    } filesize_t;

    /**
     * @brief
     * The DirectoryWatcher class is a basic directory watcher (extension for the logger class). We can define max
     * file "alive" time and filename pattern. Removes all files (or matching pattern) in specified directory path.
     */
    class DirectoryWatcher {
        PreciseTimer timer_;
        const char* filePattern_; // Optional filename pattern.
        unsigned long long fileExpiration_; // File modification time expiration (milliseconds). After this value we should remove pattern files.
        std::string logDirectory_; // Directory to watch.

    public:
        ~DirectoryWatcher() {
            // Stop timer if was not stopped yet.
            stop();
        }

        /**
         * @brief Starts directory watcher.
         * @param _logDirectory The log directory.
         * @param _fileExpirationMillis The files expiration time (we checking for last modification time) in milliseconds.
         * @param _filePattern The file pattern (optional, can be empty). We can choose which files in the directory will be removed.
         * @param _timerInterval The Watcher check interval, default: every 1 hour.
         */
        void start(const std::string& _logDirectory, const unsigned long long _fileExpirationMills, const char* _filePattern = "",
                   const unsigned long long _timerInterval = Timer<>::Milliseconds.HOUR) {
            if (!timer_.isRunning()) {
                logDirectory_ = _logDirectory;
                fileExpiration_ = _fileExpirationMills;
                filePattern_ = _filePattern;

                // First call without any delay.
                removeOldLogFiles();

                if (!timer_.isRunning()) {
                    timer_.startInterval(std::chrono::milliseconds(_timerInterval), [=] {
                        removeOldLogFiles();
                    });
                }
            }
        }

        /**
         * @brief Stops directory watcher.
         */
        void stop() {
            if (timer_.isRunning()) {
                timer_.stop();
            }
        }

        /**
         * @brief Removes old log files. Called by local watcher timer with specified intervals.
         */
        void removeOldLogFiles() {
            std::vector<std::filesystem::path> filesToRemove = filesOlderThan(logDirectory_, std::chrono::milliseconds(fileExpiration_),
                                                                              filePattern_);

            try {
                // Remove all files older than mFileExpiration
                for (const auto& file: filesToRemove) {
                    std::filesystem::remove(file);
                }
            } catch (const std::exception& ex) {
                std::cerr << "logcplus: Unexpected error while deleting file " << ex.what() << std::endl;
            }
        }

        /**
         * @brief Checks file modification time with specified limit.
         * @param _path File path (filename).
         * @param _limit Expected last modification time.
         */
        template<typename Rep, typename Period>
        bool isFileOlderThan(const std::filesystem::path& _path, const std::chrono::duration<Rep, Period> _limit) const {
            auto now = std::filesystem::file_time_type::clock::now();
            return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(now - std::filesystem::last_write_time(_path)) > _limit;
        }

        /**
         * @brief Checks the all files in the specified directory.
         * @param _directory The Directory path (filename).
         * @param _limit Expected last modification time.
         * @param _fileNameRegex Optional file pattern (e.g. when we need remove only specified files).
         */
        template<typename Rep, typename Period>
        std::vector<std::filesystem::path> filesOlderThan(const std::filesystem::path& _directory, const std::chrono::duration<Rep, Period> _limit,
                                                          const std::string& _fileNameRegex = "") const {
            std::vector<std::filesystem::path> result;
            for (const auto& file: std::filesystem::recursive_directory_iterator(_directory)) {
                // Check regular expression if defined.
                // E.g. specified file name.
                if (!_fileNameRegex.empty()) {
                    const std::regex fileMatch(_fileNameRegex);
                    if (!std::regex_match(std::filesystem::path(file.path()).filename().string(), fileMatch)) {
                        continue;
                    }
                }

                // Check last file modification.
                if (std::filesystem::is_regular_file(file) && isFileOlderThan(file, _limit)) {
                    result.push_back(file);
                }
            }

            return result;
        }
    };

    /**
     * @brief
     * The FileWatcher class is a extension around logger class. Allows you to create callback if the log
     * file is more expensive as you thought or reopen file in specified time of day (checkpoint).
     */
    class FileWatcher {
    public:
        struct FileWatcherSettings;

    private:
        PreciseTimer timer_;
        std::shared_ptr<FileWatcherSettings> fileWatcherSettings_;
        std::function<void()> callback_;

    public:
        struct FileWatcherSettings {
            // Path to the file.
            std::filesystem::path filePath;
            // We expect the file to be always smaller than the value below.
            filesize_t maxFileSize;
            // We can define when the watcher will execute callback method (e.g. specified time of day, it's optional).
            std::optional<Date::Time> checkPoint;
        };

        FileWatcher() : fileWatcherSettings_(std::make_shared<FileWatcherSettings>()) {

        }

        ~FileWatcher() {
            // Stop timer and remove file handler if was not stopped yet.
            stop();
        }

        FileWatcherSettings* settings() const {
            return fileWatcherSettings_.get();
        }

        /**
         * @brief Starts file watcher timer.
         * @param _callback Callback function.
         * @param _checkInterval The timer interval in milliseconds, default: 1 min.
         */
        void start(std::function<void()> _callback, const unsigned long long _checkInterval = Timer<>::Seconds.SECOND * 60000) {
            if (!timer_.isRunning()) {
                callback_ = _callback;
                timer_.startInterval(std::chrono::milliseconds(_checkInterval), [=] {
                    isTimeToCallback();
                });
            }
        }

        /**
         * @brief Stops file watcher timer.
         */
        void stop() {
            if (timer_.isRunning()) {
                timer_.stop();
            }
        }

        /**
         * @brief Timer callback. Creates a new log file if current size is similar(1) to the settings.
         *
         * (1) Similar because timer callback is executed every e.g. 1 min (default). The file may be larger in size if
         *     logger produces more messages.
         */
        void isTimeToCallback() {
            std::error_code errorCode{};
            Date::Time currTime = Date::currentTime();

            if (errorCode != std::error_code{}) {
                std::cerr << "logcplus: Error when accessing to the file: " << fileWatcherSettings_->filePath << " error message: "
                          << errorCode.message() << std::endl;
                return;
            }

            // We check if there is the checkpoint (optional).
            if (fileWatcherSettings_->checkPoint) {
                // We skip second part (default watcher works with 1 min intervals).
                if (fileWatcherSettings_->checkPoint->hour == currTime.hour && fileWatcherSettings_->checkPoint->minute == currTime.minute) {
                    callback_();
                }
            }

            // We check if the file size has exceeded the set values.
            std::uintmax_t currFileSize = std::filesystem::file_size(fileWatcherSettings_->filePath, errorCode);
            if (currFileSize > fileWatcherSettings_->maxFileSize.bsize()) {
                callback_();
            }
        }
    };

    class LogManager;

    /**
     * @brief Basic c++ logger library.
     */
    class Logger {
    public:
        enum class LogMode;
        enum class LogLevel;

    private:
        LogMode logMode_; // Logger mode (to Console / File)
        LogLevel logLevel_; // Log level (see log level pyramid above)
        std::streambuf* coutBuf_;
        std::pair<std::ofstream, std::string> fileHandler_;
        ConcurrentQueue<std::string> messageQueue_;
        std::thread messageQueueWorker_;
        std::atomic_bool work_, wait_;

        inline static std::atomic<Logger*> instance_{nullptr};
        inline static std::mutex instanceMutex_;

        friend class LogManager;

    public:
        enum class LogMode {
            Console, File
        };

        enum class LogLevel {
            Debug, Info, Warn, Error, Fatal
        };

        /**
         * @brief Current opened file handler.
         * @return Current filename.
         */
        std::string currentFile() const {
            return fileHandler_.second;
        }

        /**
         * @brief Helper method with defined log level. See `log` method for details.
         */
        template<typename ...Args>
        void debug(const Args& ..._args) {
            if (logLevel_ <= Logger::LogLevel::Debug) {
                log(LogLevel::Debug, _args...);
            }
        }

        /**
         * @brief Helper method with defined log level. See `log` method for details.
         */
        template<typename ...Args>
        void info(const Args& ..._args) {
            if (logLevel_ <= Logger::LogLevel::Info) {
                log(LogLevel::Info, _args...);
            }
        }

        /**
         * @brief Helper method with defined log level. See `log` method for details.
         */
        template<typename ...Args>
        void warn(const Args& ..._args) {
            if (logLevel_ <= Logger::LogLevel::Warn) {
                log(LogLevel::Warn, _args...);
            }
        }

        /**
         * @brief Helper method with defined log level. See `log` method for details.
         */
        template<typename ...Args>
        void error(const Args& ..._args) {
            if (logLevel_ <= Logger::LogLevel::Error) {
                log(LogLevel::Error, _args...);
            }
        }

        /**
         * @brief Helper method with defined log level. See `log` method for details.
         */
        template<typename ...Args>
        void fatal(const Args& ..._args) {
            if (logLevel_ <= Logger::LogLevel::Fatal) {
                log(LogLevel::Fatal, _args...);
            }
        }

        /**
         * @brief Print log with timestamp.
         * @param _args Log message parameters
         */
        template<typename ...Args>
        void log(Logger::LogLevel _logLevel, const Args& ..._args) {
            // Lock mutex (multithreading calls)
            std::string header = "[" + logTypeAsString(_logLevel) + "]" + " " + currentTime("%Y-%m-%d %X") + " -";
            std::string body = concatenateLogArguments(_args...);

            messageQueue_.enqueue(header.append(body));
        }

        /**
         * @brief Adds file separator if not exits.
         * @param _file Current full filename path
         * @return Filename path with separator (optional if exists or is empty)
         */
        std::optional<std::string> addOptionalFileSeparator(const std::string& _file) {
            if (!isFileExist(_file)) {
                return std::nullopt;
            }

            if (_file.size() > 0) {
                if (_file[_file.length() - 1] != '/') {
                    return std::string(_file + "/");
                }
            }

            return std::nullopt;
        }

        /**
         * @brief Checks if given file exists.
         * @param _path Full path to file (filename)
         * @return If file exists => true
         */
        static bool isFileExist(const std::filesystem::path& _path, std::filesystem::file_status _status = std::filesystem::file_status()) {
            if (std::filesystem::status_known(_status) ? std::filesystem::exists(_status) : std::filesystem::exists(_path)) {
                return true;
            }

            return false;
        }

        /**
         * @brief Checks if exists any files in given path.
         * @param _directory Full path to directory to check
         * @param _fileSought File sought
         * @return Number of files found
         */
        static int anyFileExists(const std::string& _directory, const std::string& _fileSought) {
            int count = 0;

            for (const auto& path: std::filesystem::directory_iterator(_directory)) {
                std::string filename = path.path().filename();

                if (filename.find(_fileSought) != std::string::npos) {
                    count++;
                }
            }

            return count;
        }

    private:
        Logger() : logMode_(Logger::LogMode::Console), logLevel_(Logger::LogLevel::Debug), work_(false), wait_(false) {}

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        ~Logger() {
            stop();
        }

        /**
         * @brief Gets current singleton instance. We allows only for one instance of logger.
         * @return Logger instance
         */
        static Logger* instance() {
            if (instance_ == nullptr) {
                std::lock_guard<std::mutex> lock(instanceMutex_);
                if (instance_ == nullptr) {
                    instance_ = new Logger();
                }
            }

            return instance_;
        }

        /**
         * @brief Open log file (initialize ofstream handler). Redirects cout stream to file.
         * @param _logDirectory Path to log directory
         * @return Successfully initialized.
         */
        void initialize(std::string& _logDirectory, const std::string& _filename) {
            if (logMode_ != Logger::LogMode::File) {
                return;
            }

            wait_.store(true, std::memory_order_release); // Information for processing queue that should wait until we create a new file handler.
            fileHandler_.first.exceptions(std::ofstream::badbit);

            // Create missing directory if was specified (not exists).
            if (!isFileExist(_logDirectory)) {
                std::filesystem::create_directories(_logDirectory);
            }

            try {
                if (!fileHandler_.first.is_open()) {
                    if (std::optional<std::string> path = addOptionalFileSeparator(_logDirectory); path.has_value()) {
                        _logDirectory = path.value();
                    }

                    std::string fullPath = _logDirectory + _filename;
                    fileHandler_.first.open(fullPath, std::ios::out | std::ios::app);
                    fileHandler_.second = fullPath;

                    // Redirect cout stream to file.
                    coutBuf_ = std::cout.rdbuf();
                    std::cout.rdbuf(fileHandler_.first.rdbuf());

                    // When it's a first initialization we need start log queue processing thread.
                    if (!work_.load(std::memory_order::memory_order_acquire)) {
                        initialize();
                    }
                }
            } catch (const std::ofstream::failure& _ex) {
                std::cerr << "[" + logTypeAsString(LogLevel::Fatal) + "]" << "[" + currentTime("%Y-%m-%d %X") + "] " << _ex.what() << std::endl;
            }

            wait_.store(false, std::memory_order_release);
        }

        /**
         * @brief Runs queue worker.
         * @return Successfully initialized.
         */
        void initialize() {
            processQueue();
        }

        /**
         * @brief Close log file.
         */
        void closeHandlers() {
            if (fileHandler_.first.is_open()) {
                wait_.store(true, std::memory_order_release);

                std::cout.rdbuf(coutBuf_);
                if (fileHandler_.first.is_open()) {
                    fileHandler_.first.close();
                }

                wait_.store(false, std::memory_order_release);
            }
        }

        /**
         * @brief Closes last file handler and creates new.
         */
        void reopen(std::string _logDirectory) {
            // Skip if it's we log on the console output.
            if (logMode_ != LogMode::File) {
                return;
            }

            if (!Logger::isFileExist(_logDirectory)) {
                std::filesystem::create_directories(_logDirectory);
            }

            std::string filename = currentTime("%Y-%m-%d") + ".log";

            // Check if the file with same date exists.
            int count = anyFileExists(_logDirectory, filename);

            // Close current file handler.
            closeHandlers();

            // If its another log file on this day we append to current filename number of file
            // with the same name (date).
            if (count != 0) {
                std::optional<std::string> dir = addOptionalFileSeparator(_logDirectory);
                if (dir.has_value()) {
                    _logDirectory = dir.value();
                }

                std::string fullPath = _logDirectory + filename;
                std::string filenameSufix = "." + std::to_string(count); // Append file number
                std::filesystem::rename(fullPath, fullPath + filenameSufix);
            }

            // Create the new log file.
            initialize(_logDirectory, currentTime("%Y-%m-%d") + ".log");
        }

        /**
         * @brief Concatenates variadic arguments to single string.
         * @param _args Variadic arguments to concatenate
         */
        template<typename ...Args>
        std::string concatenateLogArguments(const Args& ..._args) {
            std::string result;
            using ::to_string;
            using std::to_string;

            int unpack[]{0, (result += " " + to_string(_args), 0)...};
            static_cast<void>(unpack);

            return result;
        }

        /**
         * @brief Get current time as string.
         * @param _format Timestamp format, e.g %Y/%m/%d
         */
        std::string currentTime(const std::string& _format) const {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);

            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), _format.c_str());

            return ss.str();
        }

        /**
         * @brief Returns log enum type as string.
         * @param _type Log type, e.g. Info
         */
        std::string logTypeAsString(LogLevel _type) const {
            switch (_type) {
                case LogLevel::Info:
                    return "INFO";
                case LogLevel::Warn:
                    return "WARN";
                case LogLevel::Debug:
                    return "DEBUG";
                case LogLevel::Error:
                    return "ERROR";
                case LogLevel::Fatal:
                    return "FATAL";
                default:
                    return "UNKNOWN";
            }
        }

        /**
         * @brief Runs message queue thread that process all incoming log messages.
         */
        void processQueue() {
            if (work_.load(std::memory_order::memory_order_acquire)) {
                return;
            }

            work_.store(true, std::memory_order_release);

            messageQueueWorker_ = std::thread([&]() {
                while (work_.load(std::memory_order_acquire)) {
                    if (!messageQueue_.empty() && !wait_.load(std::memory_order_acquire)) {
                        std::cout << messageQueue_.dequeue() << std::endl;
                    } else {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            });
        }

        /**
         * @brief Closes all log file handlers and stops processing thread.
         */
        void stop() {
            // Close all file handlers.
            closeHandlers();

            // Wait for thread execution.
            work_.store(false, std::memory_order_release);
            if (messageQueueWorker_.joinable()) {
                messageQueueWorker_.join();
            }
        }
    };

    class LoggerConfigurator {
        friend class LogManager;

        struct LoggerConfiguration {
            // Default: current directory.
            std::filesystem::path logDirectoryPath = std::filesystem::current_path();
            // Default: 50 MB for single log file.
            filesize_t maxLogFileSize = filesize_t(50, filesize_t::SizeUnit::MB);
            // Default: log file will be not remove
            unsigned long long removeLogsOlderThan = 0;
            // Default: all logs will be printed (see Logger class for details).
            Logger::LogLevel logLevel = Logger::LogLevel::Debug;
            // Default: logs will be printed on the console.
            Logger::LogMode logMode = Logger::LogMode::Console;
            // Default: not used.
            std::optional<Date::Time> checkPoint = std::nullopt;
            // Default: not enabled.
            bool enableFileWatcher = false;
            // Default: not enabled.
            bool enableAutoRemove = false;

            std::string toString() const {
                return "Logcplus settings"
                       "\n\tLogDirectoryPath: " + logDirectoryPath.string() + "\n\tMaxLogFileSize: " + maxLogFileSize.toString() + "\n\tLogLevel: " +
                       std::to_string(static_cast<int>(logLevel)) + "\n\tLogMode: " + std::to_string(static_cast<int>(logMode)) + "\n\tCheckPoint: " +
                       (checkPoint.has_value() ? checkPoint.value().toString() : "undefined") + "\n\tRemoveLogsOlderThan: " +
                       std::to_string(removeLogsOlderThan) + "ms" + "\n\tEnableFileWatcher: " + (enableFileWatcher ? "true" : "false") +
                       "\n\tEnableAutoRemove: " + (enableAutoRemove ? "true" : "false");
            }
        };

    public:
        /**
         * @brief
         * Loads logcplus configuration file and make basic parse of the all values.
         *
         * We expect file format like:
         * ---
         * LogDirectoryPath /home/user/logs
         * MaxLogFileSize 100MiB
         * RemoveLogsOlderThan 1d
         * LogLevel Info
         * LogMode File
         * CheckPoint 11:45
         * EnableFileWatcher true
         * EnableAutoRemove true
         * ---
         *
         * If we not define all options we choose default (predefined) values.
         *
         * @param _filePath Path to the configuration file.
         * @return LoggerConfiguration instance contains default / loaded configuration options.
         */
        static LoggerConfiguration load(const std::filesystem::path& _filePath) {
            LoggerConfiguration config;
            EMap mapController;

            if (bool status = mapController.read(_filePath); status && mapController.data().size() > 0) {
                try {
                    // LogDirectoryPath
                    if (auto optValue = contains(mapController, "LogDirectoryPath"); optValue.has_value()) {
                        config.logDirectoryPath = std::any_cast<std::string>(optValue);
                    }

                    // MaxLogFileSize
                    if (auto optValue = contains(mapController, "MaxLogFileSize"); optValue.has_value()) {
                        std::string castedValue = std::any_cast<std::string>(optValue);

                        if (auto result = parseMaxLogFileSize(castedValue); result.has_value()) {
                            config.maxLogFileSize = result.value();
                        } else {
                            std::cerr << "logcplus: Unexpected configuration option: " << castedValue << std::endl;
                        }
                    }

                    // RemoveLogsOlderThan
                    if (auto optValue = contains(mapController, "RemoveLogsOlderThan"); optValue.has_value()) {
                        std::string castedValue = std::any_cast<std::string>(optValue);

                        if (auto value = parseRemoveLogsOlderThan(castedValue); value != 0) {
                            config.removeLogsOlderThan = value;
                        } else {
                            std::cerr << "logcplus: Unexpected configuration option: " << castedValue << std::endl;
                        }
                    }

                    // LogLevel
                    if (auto optValue = contains(mapController, "LogLevel"); optValue.has_value()) {
                        std::string castedValue = std::any_cast<std::string>(optValue);

                        if (auto result = parseLogLevel(castedValue)) {
                            config.logLevel = result.value();
                        } else {
                            std::cerr << "logcplus: Unexpected configuration option: " << castedValue << std::endl;
                        }
                    }

                    // LogMode
                    if (auto optValue = contains(mapController, "LogMode"); optValue.has_value()) {
                        std::string castedValue = std::any_cast<std::string>(optValue);

                        if (auto result = parseLogMode(castedValue); result.has_value()) {
                            config.logMode = result.value();
                        } else {
                            std::cerr << "logcplus: Unexpected configuration option: " << castedValue << std::endl;
                        }
                    }

                    // CheckPoint
                    if (auto optValue = contains(mapController, "CheckPoint"); optValue.has_value()) {
                        std::string castedValue = std::any_cast<std::string>(optValue);

                        if (auto result = parseCheckPoint(castedValue); result.has_value()) {
                            config.checkPoint = result.value();
                        } else {
                            std::cerr << "logcplus: Unexpected configuration option: " << castedValue << std::endl;
                        }
                    }

                    // EnableFileWatcher
                    if (auto optValue = contains(mapController, "EnableFileWatcher"); optValue.has_value()) {
                        config.enableFileWatcher = std::any_cast<bool>(optValue);
                    }

                    // EnableAutoRemove
                    if (auto optValue = contains(mapController, "EnableAutoRemove"); optValue.has_value()) {
                        config.enableAutoRemove = std::any_cast<bool>(optValue);
                    }
                } catch (std::bad_any_cast& _ex) {
                    std::cerr << "logcplus: Unexpected error when parsing configuration file, message: " << _ex.what() << std::endl;
                }
            }

            return config;
        }

    private:
        static std::any contains(EMap& _emap, const std::string _key) {
            if (_emap.containsKey(_key)) {
                if (std::any result = _emap.get(_key); result.has_value()) {
                    return result;
                }
            }

            return std::nullopt;
        }

        static std::optional<filesize_t> parseMaxLogFileSize(const std::string _value) {
            if (auto fileSize = filesize_t::parseFileSize(_value); fileSize) {
                return fileSize.value();
            }

            return std::nullopt;
        }

        static unsigned long long parseRemoveLogsOlderThan(const std::string _value) {
            /* We expect format like: 2d (2 days).
             *
             * Available formats:
             * s - seconds
             * M - minutes
             * H - hours
             * d - days
             * m - months
             * y - years
             */
            const std::regex regex("^(\\d*)(S|M|H|d|w|m|y)$");
            std::smatch match;

            if (std::regex_match(_value, match, regex)) {
                if (match.size() == 3) {
                    unsigned long long millis = 1;
                    std::string unit = match[2].str();

                    if (unit.compare("S") == 0) {
                        millis = 1000; // seconds
                    } else if (unit.compare("M") == 0) {
                        millis = 60000; // minutes
                    } else if (unit.compare("H") == 0) {
                        millis = 3600000; // hours
                    } else if (unit.compare("d") == 0) {
                        millis = 86400000; // days
                    } else if (unit.compare("w") == 0) {
                        millis = 604800000; // weeks
                    } else if (unit.compare("m") == 0) {
                        millis = 2629746000; // months
                    } else { millis = 31556952000; } // years

                    return std::stoull(match[1].str()) * millis;
                }
            }

            return 0;
        }

        static std::optional<Logger::LogLevel> parseLogLevel(std::string _value) {
            std::transform(_value.begin(), _value.end(), _value.begin(), ::tolower);

            if (_value.compare("debug") == 0) {
                return Logger::LogLevel::Debug;
            }
            if (_value.compare("info") == 0) {
                return Logger::LogLevel::Info;
            }
            if (_value.compare("warn") == 0) {
                return Logger::LogLevel::Warn;
            }
            if (_value.compare("error") == 0) {
                return Logger::LogLevel::Error;
            }
            if (_value.compare("fatal") == 0) {
                return Logger::LogLevel::Fatal;
            }

            return std::nullopt;
        }

        static std::optional<Logger::LogMode> parseLogMode(std::string _value) {
            std::transform(_value.begin(), _value.end(), _value.begin(), ::tolower);

            if (_value.compare("console") == 0) {
                return Logger::LogMode::Console;
            }
            if (_value.compare("file") == 0) {
                return Logger::LogMode::File;
            }

            return std::nullopt;
        }

        static std::optional<Date::Time> parseCheckPoint(std::string _value) {
            std::replace(_value.begin(), _value.end(), ':', ' ');  // replace ':' by ' '.

            std::vector<int> array;
            std::stringstream ss(_value);

            int temp;
            while (ss >> temp) {
                if (temp >= 0) {
                    array.push_back(temp);
                }
            }

            if (array.size() > 1) {
                if (array[0] < 24 && array[1] < 60) {
                    return Date::Time(array[0], array[1]);
                }
            }

            return std::nullopt;
        }
    };

    class LogManager {
        std::unique_ptr<FileWatcher> fileWatcher_;
        std::unique_ptr<DirectoryWatcher> directoryWatcher_;
        inline static std::atomic<LogManager*> instance_{nullptr};
        inline static std::mutex instanceMutex_;
        inline static LoggerConfigurator::LoggerConfiguration configuration_;

    public:
        /**
         * @brief Gets current singleton instance. We allows only for one instance of logger.
         * @return Logger instance
         */
        static LogManager* instance() {
            if (instance_ == nullptr) {
                std::lock_guard<std::mutex> lock(instanceMutex_);
                if (instance_ == nullptr) {
                    instance_ = new LogManager();
                }
            }

            return instance_;
        }

        static Logger* getLogger() {
            return Logger::instance();
        }

        void setLogLevel(const Logger::LogLevel _logLevel) {
            configuration_.logLevel = _logLevel;
        }

        void setLogMode(const Logger::LogMode _logMode) {
            configuration_.logMode = _logMode;
        }

        void setLogDirectory(const std::filesystem::path& _logDirectory) {
            configuration_.logDirectoryPath = _logDirectory;
        }

        void setLogFileRemoveInterval(const unsigned long long _days) {
            configuration_.removeLogsOlderThan = _days;
        }

        void setMaxFileSize(const filesize_t _fileSize) {
            configuration_.maxLogFileSize = _fileSize;
        }

        void setMaxFileSize(const std::uintmax_t _size, const filesize_t::SizeUnit _unit) {
            configuration_.maxLogFileSize = filesize_t(_size, _unit);
        }

        void setMaxFileSize(const std::string& _fileSize) {
            if (auto maxLogFileSize = filesize_t::parseFileSize(_fileSize); maxLogFileSize) {
                configuration_.maxLogFileSize = maxLogFileSize.value();
            }
        }

        /**
         * @brief Initializes logger and extensions components.
         */
        void initialize() {
            // Initialize logger.

            // Set log level and log mode.
            Logger::instance()->logMode_ = configuration_.logMode;
            Logger::instance()->logLevel_ = configuration_.logLevel;

            // Reopen log file.
            if (configuration_.logMode == Logger::LogMode::File) {
                Logger::instance()->reopen(configuration_.logDirectoryPath);
                // or just initialize when we need log on the console output (start message queue processing).
            } else {
                Logger::instance()->initialize();
            }

            // Enable extensions.
            if (configuration_.enableFileWatcher) {
                FileWatcher::FileWatcherSettings* settings = fileWatcher_->settings();
                settings->filePath = getLogger()->currentFile();
                settings->maxFileSize = configuration_.maxLogFileSize;
                settings->checkPoint = configuration_.checkPoint;

                enableFileWatcher();
            }

            if (configuration_.enableAutoRemove) {
                enableDirectoryWatcher();
            }
        }

        /**
         * @brief Enables file watcher (checkpoints and file size limit).
         */
        void enableFileWatcher() {
            if (fileWatcher_ && configuration_.enableFileWatcher && configuration_.logMode == Logger::LogMode::File) {
                fileWatcher_->start([=] {
                    Logger::instance()->reopen(configuration_.logDirectoryPath);
                });
            }
        }

        /**
         * @brief Disables file watcher (checkpoints and file size limit).
         */
        void disableFileWatcher() {
            fileWatcher_->stop();
        }

        /**
         * @brief Enables auto removing old log files.
         */
        void enableDirectoryWatcher() {
            // Initialize watchers.
            if (configuration_.removeLogsOlderThan > 0 && configuration_.enableAutoRemove && directoryWatcher_ &&
                configuration_.logMode == Logger::LogMode::File) {
                directoryWatcher_->start(configuration_.logDirectoryPath, configuration_.removeLogsOlderThan, LOG_FILE_FORMAT);
            }
        }

        /**
         * @brief Disbales auto removing old log files.
         */
        void disableDirectoryWatcher() {
            directoryWatcher_->stop();
        }

        /**
         * @brief Loads logger configuration from file.
         * @param _path Path to the configuration file.
         */
        void loadConfigurationFromFile(const std::filesystem::path& _path) {
            configuration_ = LoggerConfigurator::load(_path);
            std::cout << configuration_.toString() << std::endl;
        }

    private:
        LogManager() {
            fileWatcher_ = std::make_unique<FileWatcher>();
            directoryWatcher_ = std::make_unique<DirectoryWatcher>();
        }

        LogManager(const LogManager&) = delete;
        LogManager& operator=(const LogManager&) = delete;
        ~LogManager() = default;
    };
}

#endif //LOGCPLUS_LOGCPLUS_H
