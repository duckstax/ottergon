#include "wal_replicate.hpp"

#include "route.hpp"

#include <crc32c/crc32c.h>

namespace services::wal {

    bool file_exist_(const std::filesystem::path& path) {
        std::filesystem::file_status s = std::filesystem::file_status{};
        return std::filesystem::status_known(s) ? std::filesystem::exists(s) : std::filesystem::exists(path);
    }

    std::size_t next_index(std::size_t index, size_tt size) { return index + size + sizeof(size_tt) + sizeof(crc32_t); }

    wal_replicate_t::~wal_replicate_t() { trace(log_, "delete wal_replicate_t"); }

    void wal_replicate_t::load(session_id_t& session, services::wal::id_t wal_id) {
        trace(log_, "wal_replicate_t::load, session: {}, id: {}", session.data(), wal_id);
        std::size_t start_index = 0;
        next_id(wal_id);
        std::vector<record_t> records;
        if (config_.sync_to_disk && find_start_record(wal_id, start_index)) {
            std::size_t size = 0;
            do {
                records.emplace_back(read_record(start_index));
                start_index = next_index(start_index, records[size].size);
            } while (records[size++].is_valid());
            records.erase(records.end() - 1);
        }
        actor_zeta::send(current_message()->sender(),
                         address(),
                         handler_id(route::load_finish),
                         session,
                         std::move(records));
    }

    void wal_replicate_t::create_database(session_id_t& session, components::ql::create_database_t& data) {
        trace(log_, "wal_replicate_t::create_database {}, session: {}", data.database_, session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::drop_database(session_id_t& session, components::ql::drop_database_t& data) {
        trace(log_, "wal_replicate_t::drop_database {}, session: {}", data.database_, session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::create_collection(session_id_t& session, components::ql::create_collection_t& data) {
        trace(log_,
              "wal_replicate_t::create_collection {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::drop_collection(session_id_t& session, components::ql::drop_collection_t& data) {
        trace(log_,
              "wal_replicate_t::drop_collection {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::insert_one(session_id_t& session, components::ql::insert_one_t& data) {
        trace(log_,
              "wal_replicate_t::insert_one {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::insert_many(session_id_t& session, components::ql::insert_many_t& data) {
        trace(log_,
              "wal_replicate_t::insert_many {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::delete_one(session_id_t& session, components::ql::delete_one_t& data) {
        trace(log_,
              "wal_replicate_t::delete_one {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::delete_many(session_id_t& session, components::ql::delete_many_t& data) {
        trace(log_,
              "wal_replicate_t::delete_many {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::update_one(session_id_t& session, components::ql::update_one_t& data) {
        trace(log_,
              "wal_replicate_t::update_one {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::update_many(session_id_t& session, components::ql::update_many_t& data) {
        trace(log_,
              "wal_replicate_t::update_many {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::create_index(session_id_t& session, components::ql::create_index_t& data) {
        trace(log_,
              "wal_replicate_t::create_index {}::{}, session: {}",
              data.database_,
              data.collection_,
              session.data());
        write_data_(data);
        send_success_(session);
    }

    void wal_replicate_t::send_success_(session_id_t& session) {
        auto sender = current_message()->sender();
        if (sender) {
            trace(log_, "wal_replicate_t::send_success session {}", session.data());
            actor_zeta::send(sender, address(), handler_id(route::success), session, services::wal::id_t(id_));
        }
    }

    void wal_replicate_t::write_buffer(buffer_t& buffer) {
        if (config_.sync_to_disk) {
            file_->append(buffer.data(), buffer.size());
        }
    }

    void wal_replicate_t::read_buffer(buffer_t& buffer, size_t start_index, size_t size) const {
        if (config_.sync_to_disk) {
            file_->read(buffer, size, off64_t(start_index));
        } else {
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), '\0');
        }
    }

    size_tt read_size_impl(buffer_t& input, int index_start) {
        size_tt size_tmp = 0;
        size_tmp = size_tt(0xff00 & input[size_t(index_start)] << 8);
        size_tmp |= size_tt(0x00ff & input[size_t(index_start) + 1]);
        return size_tmp;
    }

    static size_tt read_size_impl(const char* input, int index_start) {
        size_tt size_tmp = 0;
        size_tmp = size_tt(0xff00 & (input[index_start] << 8));
        size_tmp |= size_tt(0x00ff & (input[index_start + 1]));
        return size_tmp;
    }

    size_tt wal_replicate_t::read_size(size_t start_index) const {
        auto size_read = sizeof(size_tt);
        buffer_t buffer;
        read_buffer(buffer, start_index, size_read);
        auto size_blob = read_size_impl(buffer.data(), 0);
        return size_blob;
    }

    buffer_t wal_replicate_t::read(size_t start_index, size_t finish_index) const {
        auto size_read = finish_index - start_index;
        buffer_t buffer;
        read_buffer(buffer, start_index, size_read);
        return buffer;
    }

    template<class T>
    void wal_replicate_t::write_data_(T& data) {
        next_id(id_);
        buffer_t buffer;
        last_crc32_ = pack(buffer, last_crc32_, id_, data);
        write_buffer(buffer);
    }

    void wal_replicate_t::init_id() {
        std::size_t start_index = 0;
        auto id = read_id(start_index);
        while (id > 0) {
            id_ = id;
            start_index = next_index(start_index, read_size(start_index));
            id = read_id(start_index);
        }
    }

    bool wal_replicate_t::find_start_record(services::wal::id_t wal_id, std::size_t& start_index) const {
        start_index = 0;
        auto first_id = read_id(start_index);
        if (first_id > 0) {
            for (auto n = first_id; n < wal_id; ++n) {
                auto size = read_size(start_index);
                if (size > 0) {
                    start_index = next_index(start_index, size);
                } else {
                    return false;
                }
            }
            return wal_id == read_id(start_index);
        }
        return false;
    }

    services::wal::id_t wal_replicate_t::read_id(std::size_t start_index) const {
        auto size = read_size(start_index);
        if (size > 0) {
            auto start = start_index + sizeof(size_tt);
            auto finish = start + size;
            auto output = read(start, finish);
            return unpack_wal_id(output);
        }
        return 0;
    }

    record_t wal_replicate_t::read_record(std::size_t start_index) const {
        record_t record;
        record.size = read_size(start_index);
        if (record.size > 0) {
            auto start = start_index + sizeof(size_tt);
            auto finish = start + record.size + sizeof(crc32_t);
            auto output = read(start, finish);
            record.crc32 = read_crc32(output, record.size);
            if (record.crc32 == crc32c::Crc32c(output.data(), record.size)) {
                msgpack::unpacked msg;
                msgpack::unpack(msg, output.data(), record.size);
                const auto& o = msg.get();
                record.last_crc32 = o.via.array.ptr[0].as<crc32_t>();
                record.id = o.via.array.ptr[1].as<services::wal::id_t>();
                record.type = static_cast<components::ql::statement_type>(o.via.array.ptr[2].as<char>());
                record.set_data(o.via.array.ptr[3]);
            } else {
                record.type = components::ql::statement_type::unused;
                //todo: error wal content
            }
        } else {
            record.type = components::ql::statement_type::unused;
        }
        return record;
    }

#ifdef DEV_MODE
    bool wal_replicate_t::test_find_start_record(services::wal::id_t wal_id, std::size_t& start_index) const {
        return find_start_record(wal_id, start_index);
    }

    services::wal::id_t wal_replicate_t::test_read_id(std::size_t start_index) const { return read_id(start_index); }

    std::size_t wal_replicate_t::test_next_record(std::size_t start_index) const {
        return next_index(start_index, read_size(start_index));
    }

    record_t wal_replicate_t::test_read_record(std::size_t start_index) const { return read_record(start_index); }

    size_tt wal_replicate_t::test_read_size(size_t start_index) const { return read_size(start_index); }

    buffer_t wal_replicate_t::test_read(size_t start_index, size_t finish_index) const {
        return read(start_index, finish_index);
    }
#endif

    test_wal_supervisor_t::test_wal_supervisor_t(const configuration::config_wal& config, log_t& log)
        : actor_zeta::cooperative_supervisor<test_wal_supervisor_t>(actor_zeta::detail::pmr::get_default_resource(),
                                                                    "memory_storage")
        , e_(new actor_zeta::shared_work(1, 1000)) {
        wal_address_ =
            spawn_actor<wal::wal_replicate_t>([this](wal::wal_replicate_t* ptr) { wal.reset(ptr); }, config, log);
        add_handler(handler_id(wal::route::success), &test_wal_supervisor_t::wal_success);
        e_->start();
    }

    void test_wal_supervisor_t::call_wal(components::session::session_id_t& session,
                                         components::ql::ql_statement_t* statement) {
        i = 0;

        switch (statement->type()) {
            case statement_type::create_database:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::create_database),
                                 session,
                                 static_cast<components::ql::create_database_t&>(*statement));
                break;
            case statement_type::drop_database:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::drop_database),
                                 session,
                                 static_cast<components::ql::drop_database_t&>(*statement));
                break;
            case statement_type::create_collection:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::create_collection),
                                 session,
                                 static_cast<components::ql::create_collection_t&>(*statement));
                break;
            case statement_type::drop_collection:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::drop_collection),
                                 session,
                                 static_cast<components::ql::drop_collection_t&>(*statement));
                break;
            case statement_type::insert_one:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::insert_one),
                                 session,
                                 static_cast<components::ql::insert_one_t&>(*statement));
                break;
            case statement_type::insert_many:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::insert_many),
                                 session,
                                 static_cast<components::ql::insert_many_t&>(*statement));
                break;
            case statement_type::delete_one:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::delete_one),
                                 session,
                                 static_cast<components::ql::delete_one_t&>(*statement));
                break;
            case statement_type::delete_many:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::delete_many),
                                 session,
                                 static_cast<components::ql::delete_many_t&>(*statement));
                break;
            case statement_type::update_one:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::update_one),
                                 session,
                                 static_cast<components::ql::update_many_t&>(*statement));
                break;
            case statement_type::update_many:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::update_many),
                                 session,
                                 static_cast<components::ql::update_many_t&>(*statement));
                break;
            case statement_type::create_index:
                actor_zeta::send(wal_address_,
                                 address(),
                                 wal::handler_id(wal::route::create_index),
                                 session,
                                 static_cast<components::ql::create_index_t&>(*statement));
                break;
            default:
                throw std::runtime_error("unhandled case");
                break;
        }
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this]() { return i == 1; });
    }
    void test_wal_supervisor_t::wal_success() {
        i = 1;
        cv_.notify_all();
    }

    actor_zeta::scheduler::scheduler_abstract_t* test_wal_supervisor_t::scheduler_impl() noexcept { return e_; }

    void test_wal_supervisor_t::enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) {
        std::unique_lock<spin_lock> _(lock_);
        set_current_message(std::move(msg));
        execute(this, current_message());
    }

} //namespace services::wal
