#ifndef DBTERM_QUERYRESULT_H
#define DBTERM_QUERYRESULT_H

#include <string>
#include <vector>
#include <cstdint>

namespace dbterm {

struct QueryResult {
    bool success{false};
    std::string errorMessage;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    uint64_t affectedRows{0};
};

} // namespace dbterm

#endif // DBTERM_QUERYRESULT_H
