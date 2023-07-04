#include "parser.hpp"

#include "parser/base/parser_result.hpp"
#include "parser/common/parser_invalid.hpp"
#include "parser/database/parser_database.hpp"
#include "parser/insert/parser_insert.hpp"

#define PARSE(F) if (!ok) ok = F::parse(query, result)

namespace components::sql {

    parse_result::parse_result(const ql::variant_statement_t& ql)
        : ql(ql)
        , error(parse_error::no_error, "") {
    }

    parse_result::parse_result(const error_t& error)
        : ql(ql::unused_statement_t{})
        , error(error) {
    }


    parse_result parse(std::string_view query) {
        ql::variant_statement_t result;
        components::sql::impl::parser_result ok{false};
        PARSE(database);
        PARSE(insert);
        PARSE(invalid);
        if (ok.is_error()) {
            return parse_result(error_t(ok.error,
                                        std::string_view(ok.error_token.begin, ok.error_token.size()),
                                        ok.what));
        }
        return parse_result(result);
    }

    parse_result parse(const std::string& query) {
        return parse(std::string_view{query});
    }

    parse_result parse(const char* query) {
        return parse(std::string_view{query});
    }

} // namespace components::sql
