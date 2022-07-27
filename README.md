# logcplus
Simple C++ library for logging.

## Features
- Log on to the console or to the file
- Max log file size
- Log files retention
- Optional configuration file
```text
LogDirectoryPath <absolute path>
MaxLogFileSize <size B, KB, KiB, MB, MiB, GB, GiB>
RemoveLogsOlderThan <days>
LogLevel <Debug, Info, Warn, Error Fatal>
LogMode <Console, File>
CheckPoint <hours:minutes>
EnableFileWatcher <true / false>
EnableAutoRemove <true / false>
```
- Log levels (__DEBUG__, __INFO__, __WARN__, __ERROR__, __FATAL__)

|           | **DEBUG** | **INFO** | **WARN** | **ERROR** | **FATAL** |
|-----------|:---------:|:--------:|:--------:|:---------:|:---------:|
| **DEBUG** |    _x_    |    _x_   |    _x_   |    _x_    |    _x_    |
|  **INFO** |           |    _x_   |    _x_   |    _x_    |    _x_    |
|  **WARN** |           |          |    _x_   |    _x_    |    _x_    |
| **ERROR** |           |          |          |    _x_    |    _x_    |
| **FATAL** |           |          |          |           |    _x_    |

__[LEGEND]__:
* column - defined log level
* row - the message levels that will be printed by the logger

## Requirements
- cmake _>= 3.22_
- libstdc++ _>= 9.1, c++17 support is required_

## Build

You can build tests using cmake
```
cmake .
make -j <available processors>
```

## Built with
* [cmake](https://cmake.org)
* [Boost](https://www.boost.org)

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
