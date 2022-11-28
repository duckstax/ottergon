#pragma once

#include <memory>
#include "expression.hpp"
#include "key.hpp"

namespace components::expressions {

    class compare_expression_t;
    using compare_expression_ptr = boost::intrusive_ptr<compare_expression_t>;

    class compare_expression_t final : public expression_i {
    public:
        compare_expression_t(const compare_expression_t&) = delete;
        compare_expression_t(compare_expression_t&&) = default;
        ~compare_expression_t() final = default;

        compare_expression_t(compare_type type, const key_t& key, core::parameter_id_t);
        explicit compare_expression_t(compare_type condition);

        compare_type type() const;
        const key_t& key() const;
        core::parameter_id_t value() const;
        const std::vector<compare_expression_ptr>& children() const;

        void append_child(const compare_expression_ptr& child);

        bool is_union() const;

    private:
        compare_type type_;
        key_t key_;
        core::parameter_id_t value_;
        std::vector<compare_expression_ptr> children_;
        bool union_;

        hash_t hash_impl() const final;
        std::string to_string_impl() const final;
        bool equal_impl(const expression_i* rhs) const final;
    };

    compare_expression_ptr make_compare_expression(compare_type condition, const key_t& key, core::parameter_id_t id);
    compare_expression_ptr make_compare_expression(compare_type condition);
    compare_expression_ptr make_compare_union_expression(compare_type condition);

} // namespace components::expressions