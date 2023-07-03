#include "parser.hpp"

#include "parser/base/parser_result.hpp"
#include "parser/common/parser_invalid.hpp"
#include "parser/database/parser_database.hpp"
#include "parser/insert/parser_insert.hpp"

namespace components::sql {

    ql::variant_statement_t parse(std::string_view query) {
        ql::variant_statement_t result;
        auto ok = database::parse(query, result)
                || insert::parse(query, result)
                || invalid::parse(query, result);
        if (ok/*.is_error()*/) {
            //todo: error
        }
        return result;
    }

    ql::variant_statement_t parse(const std::string& query) {
        return parse(std::string_view{query});
    }

    ql::variant_statement_t parse(const char* query) {
        return parse(std::string_view{query});
    }

} // namespace components::sql
