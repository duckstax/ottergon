#include "operator_update.hpp"
#include <services/collection/collection.hpp>

namespace services::collection::operators {

    operator_update::operator_update(context_collection_t* context, document_ptr update)
        : read_write_operator_t(context, operator_type::update)
        , update_(std::move(update)) {
    }

    void operator_update::on_execute_impl(components::pipeline::context_t* pipeline_context) {
        if (left_ && left_->output() && !left_->output()->documents().empty()) {
            modified_ = make_operator_write_data(context_->resource());
            no_modified_ = make_operator_write_data(context_->resource());
            for (auto& document : left_->output()->documents()) {
                context_->index_engine()->delete_document(document, pipeline_context); //todo: can optimized
                if (document->update(update_)) {
                    modified_->append(get_document_id(document));
                } else {
                    no_modified_->append(get_document_id(document));
                }
                context_->index_engine()->insert_document(document, pipeline_context);
            }
        }
    }

} // namespace services::collection::operators
