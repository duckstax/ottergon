#pragma once

#include <services/collection/operators/operator.hpp>
#include <services/collection/operators/predicates/predicate.hpp>

namespace services::collection::operators {

    class full_scan final : public read_only_operator_t {
    public:
        full_scan(context_collection_t* collection, predicates::predicate_ptr predicate, predicates::limit_t limit);

    private:
        void on_execute_impl(operator_data_t* data) final;

        const predicates::predicate_ptr predicate_;
        const predicates::limit_t limit_;
    };

} // namespace services::operators