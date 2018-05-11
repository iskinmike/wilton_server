



#include <cstdio>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "staticlib/support.hpp"
#include "staticlib/json.hpp"
#include "staticlib/utils.hpp"

//#include "wilton/wilton_server.h"
#include "wilton_websocket_server.h"
#include "wilton/wiltoncall.h"

#include "wilton/support/buffer.hpp"
#include "wilton/support/exception.hpp"
#include "wilton/support/handle_registry.hpp"
#include "wilton/support/payload_handle_registry.hpp"
#include "wilton/support/logging.hpp"
#include "wilton/support/misc.hpp"

namespace wilton {
namespace websocket_server {

class websocket_server_context {
public:
    std::vector<ws_views> views;
    std::vector<std::shared_ptr<worker_data>> paths;
    websocket_server_context(){}
    ~websocket_server_context(){
        views.clear(); // TODO удаление данных
        paths.clear();
    }
};
namespace {



//    typedef  std::vector<std::string> ws_server_ctx;
    // initialized from wilton_module_init
    std::shared_ptr<support::payload_handle_registry<wilton_websocket_server, websocket_server_context>> shared_websocket_server_registry() {
        static auto registry = std::make_shared<wilton::support::payload_handle_registry<wilton_websocket_server, websocket_server_context>>(
            [] (wilton_websocket_server * server) STATICLIB_NOEXCEPT {
                server->stop();
            });
        return registry;
    }

    // initialized from wilton_module_init
    std::shared_ptr<support::handle_registry<ws_worker>> shared_websocket_worker_registry() {
        static auto registry = std::make_shared<wilton::support::handle_registry<ws_worker>>(
            [] (ws_worker* worker) STATICLIB_NOEXCEPT {
                // TODO
//                (void* ) worker;
                worker->stop();
                delete worker;
            });
        return registry;
    }

//    std::vector<wilton_websocket_server*> websocket_servers;

    ws_views extract_and_delete_websocket_views(sl::json::value& conf) {
        std::vector<sl::json::field>& fields = conf.as_object_or_throw(TRACEMSG(
                "Invalid configuration object specified: invalid type," +
                " conf: [" + conf.dumps() + "]"));
        ws_views views;
        uint32_t i = 0;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            sl::json::field& fi = *it;
            if ("websocket_views" == fi.name()) {
                if (sl::json::type::array != fi.json_type()) throw support::exception(TRACEMSG(
                        "Invalid configuration object specified: 'websocket_views' attr is not a list," +
                        " conf: [" + conf.dumps() + "]"));
                for (auto& va : fi.as_array()) {
                    if (sl::json::type::object != va.json_type()) throw support::exception(TRACEMSG(
                            "Invalid configuration object specified: 'views' is not a 'object'," +
                            "index: [" + sl::support::to_string(i) + "], conf: [" + conf.dumps() + "]"));
                    // Нужно определить метод
                    for (const sl::json::field& fi : va.as_object()) {
                        auto& name = fi.name();
                        if ("method" == name) {
                            std::string method = fi.as_string_nonempty_or_throw(name);
                            if (!method.compare("ONOPEN")) {
                                views.onopen_view.setup_values(va);
                            } else if (!method.compare("ONCLOSE")){
                                views.onclose_view.setup_values(va);
                            } else if (!method.compare("ONMESSAGE")){
                                views.onmessage_view.setup_values(va);
                            } else if (!method.compare("ONERROR")){
                                views.onerror_view.setup_values(va);
                            } else {
                                throw sl::support::exception(TRACEMSG("Unknown method type: [" + name + "]"));
                            }
                        }
                    }
                }
                // drop views attr and return immediately (iters are invalidated)
                fields.erase(it);
                return views;
            }
            i++;
        }
        throw support::exception(TRACEMSG(
                "Invalid configuration object specified: 'views' list not specified," +
                " conf: [" + conf.dumps() + "]"));
    }


    std::vector<std::shared_ptr<worker_data>> create_websocket_paths(
            const std::vector<ws_views>& views) {
        std::vector<std::shared_ptr<worker_data>> res;

        // basic handler that calls wiltoncall_runscript
        auto basic_handler = [] (void* passed, std::string message, int64_t requestHandle){

            char* passed_char_ptr = static_cast<char*> (passed);
            std::string passed_json_str(passed_char_ptr);
//            std::cout << "passed_json_str:    [" << passed_json_str << "]" << std::endl;
            sl::json::value cb_ptr = sl::json::loads(passed_json_str);//static_cast<sl::json::value*> (passed);

            std::string engine("");
            char* out = nullptr;
            int out_len = 0;

            sl::json::value json =  cb_ptr.clone();//sl::json::loads(json_in);
            std::vector<sl::json::value> args;
            args.emplace_back(requestHandle); // id value
            if (!message.empty()) {
                args.emplace_back(message);
            }
            json.as_object_or_throw().emplace_back("args", std::move(args));
            std::string json_in = json.dumps();

            auto err = wiltoncall_runscript(engine.c_str(), engine.size(), json_in.c_str(), static_cast<int> (json_in.size()),
                                 std::addressof(out), std::addressof(out_len));
            if (nullptr == err) {
                if (nullptr != out) {
                    wilton_free(out);
                }
            } else {
                std::string msg = TRACEMSG(err);
                wilton_free(err);
                std::cout << "error run handler: [" << msg << "]" << std::endl;
            }
        };

        // handler that registers websocket for connection
        auto open_handler = [basic_handler] (void* passed, void* user_data) {
            int64_t requestHandle = *(static_cast<int64_t*>(user_data)); //rreg->put(tmp_server, static_cast<std::vector<std::string>> (ctx));
            basic_handler(passed, std::string{}, requestHandle);
        };
        // handler that removes registered websocket
        auto close_handler = [basic_handler] (void* passed, void* user_data){
            int64_t requestHandle = *(static_cast<int64_t*>(user_data));
            basic_handler(passed, std::string{}, requestHandle);
            auto rworker = shared_websocket_worker_registry();
            rworker->remove(requestHandle);
        };
        auto error_handler = [basic_handler] (void* passed, void* user_data){
            int64_t requestHandle = *(static_cast<int64_t*>(user_data));
            basic_handler(passed, std::string{}, requestHandle);
        };
        auto message_handler = [basic_handler] (void* passed, void* user_data, std::string message){
            int64_t requestHandle = *(static_cast<int64_t*>(user_data));
            basic_handler(passed, message, requestHandle);
        };

        auto ws_main_handler = [] (ws_worker* worker) {
            // Положить в регистр и получить id
            auto rworker = shared_websocket_worker_registry();
            int64_t worker_id = rworker->put(worker);
            // Записать id в данные для onopen
            worker->setup_user_data(static_cast<void*> (new int64_t(worker_id)));
            // Собственно все
        };

        for (auto& vi : views) {
            worker_data tmpdata;
            std::shared_ptr<worker_data> data = std::make_shared<worker_data>(tmpdata);
            // setup data
            data->open_data =    static_cast<void*> (const_cast<char*>(vi.onopen_view.data.c_str()));
            data->close_data =   static_cast<void*> (const_cast<char*>(vi.onclose_view.data.c_str()));
            data->error_data =   static_cast<void*> (const_cast<char*>(vi.onerror_view.data.c_str()));
            data->message_data = static_cast<void*> (const_cast<char*>(vi.onmessage_view.data.c_str()));
            // setup handlers
            data->open_handler =    open_handler;
            data->close_handler =   close_handler;
            data->error_handler =   error_handler;
            data->message_handler = message_handler;

            data->websocket_handler = ws_main_handler;

            data->resource = vi.onopen_view.path;
//            const char* err = nullptr; // test.c_str();
//            if (nullptr != err) throw support::exception(TRACEMSG(err));
            res.push_back(data);
        }
        return res;
    }
}

void initialize(){
    shared_websocket_worker_registry();
    shared_websocket_server_registry();
}


support::buffer websocket_server_create(sl::io::span<const char> data) {
     auto json = sl::json::load(data);

    websocket_server_context* ctx = new websocket_server_context();
//     std::vector<ws_views> real_views;
//     std::vector<std::shared_ptr<worker_data>> ws_paths;

     int number_of_threads = -1;
     int port = -1;
     auto ip = std::string{};

//     auto views = extract_and_delete_handlers(json);
     ws_views ws_views_data = extract_and_delete_websocket_views(json);
//     ws_server_ctx ctx; // TODO Проверку всего контекста и путей а не только 1 раз
     /*real_views*/ ctx->views.push_back(ws_views_data);
     ctx->paths = create_websocket_paths(ctx->views);
     // Нужно сделать обертку по типу сервера через
     // create_websocket_paths(ws_views_data);
     // Там внутри будут созданы каллбак функции которые будут отвечать за
     // обработку void* данных и вызывать wiltoncall_runscript
     // Их следует отправить в конструктор websocket_server
     // И уже на их основе будет работать

     /*"numberOfThreads": uint32_t,
     "tcpPort": uint16_t,
     "ipAddress": "x.x.x.x",*/

     for (const sl::json::field& fi : json.as_object()) {
         auto& name = fi.name();
         if ("numberOfThreads" == name) {
             number_of_threads = fi.as_int64_or_throw(name);
         } else if ("tcpPort" == name) {
             port = fi.as_int64_or_throw(name);
         } else if ("ipAddress" == name) {
             ip = fi.as_string_nonempty_or_throw(name);
         } else {
             throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
         }
     }

    wilton_websocket_server* server = new wilton_websocket_server(number_of_threads, port,
            asio::ip::address_v4::from_string(ip));

    auto regserver = shared_websocket_server_registry();
    int64_t serverHandle = regserver->put(server, std::move(*(ctx)));

    for (auto& path: ctx->paths){
        server->add_websocket_handler(path->resource, *(path.get()));
    }

    server->start();

    return support::make_json_buffer({
        { "websocketServerHandle", serverHandle}
    });
}

support::buffer websocket_server_stop(sl::io::span<const char> data) {
    // json parse
    auto json = sl::json::load(data);

    uint64_t id = -1;
    for (const sl::json::field& fi : json.as_object()) {
         auto& name = fi.name();
         if ("websocketServerHandle" == name) {
             id = fi.as_int64_or_throw(name);
         } else {
             throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
         }
     }

    auto regserver = shared_websocket_server_registry();
    auto server = regserver->remove(id);

    server.first->stop();
//    delete server.first;

    std::cout << "[" << json.dumps() << "]" << std::endl;
    return support::make_null_buffer();
}

support::buffer websocket_send(sl::io::span<const char> data) {
    auto json = sl::json::load(data);
    uint64_t wss_id = -1;
    uint64_t ws_id = -1;
    auto message = std::string{};
    for (const sl::json::field& fi : json.as_object()) {
        auto& name = fi.name();
        if ("websocketServerHandle" == name) {
            wss_id = fi.as_int64_or_throw(name);
        } else if ("websocketHandle" == name) {
            ws_id = fi.as_int64_or_throw(name);
        } else if ("data" == name) {
            message = fi.as_string_or_throw(name);
        }else {
            throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }

    (void) wss_id; // suppress
    auto rworker = shared_websocket_worker_registry();
    auto worker = rworker->peek(ws_id);
    worker->send(message);
//    websocket_servers[wss_id]->send(ws_id, message);
    return support::make_null_buffer();
}

}// namespace websocket_server
}// namespace wilton
