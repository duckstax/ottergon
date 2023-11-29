#pragma once

#include <vector>

#include <actor-zeta/actor-zeta.hpp>

#include <components/document/document_view.hpp>
#include <components/ql/base.hpp>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set_hook.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>


namespace components::cursor {

    using data_t = components::document::document_view_t;
    using data_ptr = const data_t*;
    using index_t = int32_t;
    constexpr index_t start_index = -1;

    enum class error_code_t {
        none = 0,
        database_already_exists,
        database_not_exists,
        collection_already_exists,
        collection_not_exists,
        collection_dropped,
        sql_parse_error,
        create_phisical_plan_error,
        other_error
    };

    struct error_t {
        error_code_t type;
        std::string what;

        explicit error_t();
        explicit error_t(error_code_t type);
        explicit error_t(error_code_t type, const std::string& what);
    };

    struct list_addresses_t final {
        struct res_t {
            collection_full_name_t name;
            actor_zeta::address_t address;
        };

        std::pmr::vector<res_t> addresses;

        explicit list_addresses_t(std::pmr::memory_resource *resource);
    };

    class sub_cursor_t : public boost::intrusive::list_base_hook<> {
    public:
        sub_cursor_t(std::pmr::memory_resource* resource, actor_zeta::address_t collection);
        actor_zeta::address_t& address();
        std::size_t size() const;
        std::pmr::vector<data_t>& data();
        void append(data_t);

    private:
        actor_zeta::address_t collection_;
        std::pmr::vector<data_t> data_;
    };

    class cursor_t : public boost::intrusive_ref_counter<cursor_t> {
    public:
        cursor_t(std::pmr::memory_resource* resource);
        cursor_t(const error_t& error);
        cursor_t(bool success);
        void push(sub_cursor_t* sub_cursor);
        std::size_t size() const;
        std::pmr::vector<std::unique_ptr<sub_cursor_t>>::iterator begin();
        std::pmr::vector<std::unique_ptr<sub_cursor_t>>::iterator end();
        bool has_next() const;
        data_ptr next();
        data_ptr get() const;
        data_ptr get(std::size_t index) const;
        bool is_success() const;
        bool is_error() const;
        error_t error() const;
        void sort(std::function<bool(data_ptr, data_ptr)> sorter);

    private:
        std::size_t size_{};
        index_t current_index_{start_index};
        std::pmr::vector<std::unique_ptr<sub_cursor_t>> sub_cursor_;
        std::pmr::vector<data_ptr> sorted_;
        error_t error_;
        bool success_{true};

        void create_list_by_sort();
        data_ptr get_sorted(std::size_t index) const;
        data_ptr get_unsorted(std::size_t index) const;
    };

    using cursor_t_ptr = boost::intrusive_ptr<cursor_t>;

    cursor_t_ptr make_error(error_code_t type, const std::string& what = std::string());
    cursor_t_ptr make_from_sub_cursor(std::pmr::memory_resource* resource, actor_zeta::address_t collection);
    cursor_t_ptr make_from_sub_cursor(sub_cursor_t* sub);

} // namespace components::cursor
