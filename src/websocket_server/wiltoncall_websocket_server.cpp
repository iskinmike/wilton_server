



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

namespace {
    std::vector<wilton_websocket_server*> websocket_servers;

    std::vector<ws_view> extract_and_delete_handlers(sl::json::value& conf) {
        std::vector<sl::json::field>& fields = conf.as_object_or_throw(TRACEMSG(
                "Invalid configuration object specified: invalid type," +
                " conf: [" + conf.dumps() + "]"));
        std::vector<ws_view> views;
        uint32_t i = 0;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            sl::json::field& fi = *it;
            if ("views" == fi.name()) {
                if (sl::json::type::array != fi.json_type()) throw support::exception(TRACEMSG(
                        "Invalid configuration object specified: 'views' attr is not a list," +
                        " conf: [" + conf.dumps() + "]"));
                for (auto& va : fi.as_array()) {
                    if (sl::json::type::object != va.json_type()) throw support::exception(TRACEMSG(
                            "Invalid configuration object specified: 'views' is not a 'object'," +
                            "index: [" + sl::support::to_string(i) + "], conf: [" + conf.dumps() + "]"));
                    views.emplace_back(ws_view(va));
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
}

support::buffer websocket_server_create(sl::io::span<const char> data) {
     auto json = sl::json::load(data);
    // auto views = extract_and_delete_views(conf_in);
     auto conf = json.dumps();

     int number_of_threads = -1;
     int port = -1;
     auto ip = std::string{};

     auto views = extract_and_delete_handlers(json);

     std::cout << "views size: [" << views.size() <<  "]" << std::endl;
//     views.clear();

     /*"numberOfThreads": uint32_t,
//     "tcpPort": uint16_t,
//     "ipAddress": "x.x.x.x",*/

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

//     ipv4 = asio::ip::address_v4();
    int id = websocket_servers.size();
    wilton_websocket_server* server = new wilton_websocket_server(number_of_threads, port, asio::ip::address_v4::from_string(ip), views, id);

    websocket_servers.push_back(server);

    server->start();

     /*server_ctx ctx;
     auto paths = create_paths(views, ctx);
     auto paths_pass = wrap_paths(paths);
     wilton_Server* server = nullptr;
     char* err = wilton_Server_create(std::addressof(server),
             conf.c_str(), static_cast<int>(conf.length()),
             paths_pass.data(), static_cast<int>(paths_pass.size()));
     if (nullptr != err) support::throw_wilton_error(err, TRACEMSG(err));
     auto sreg = shared_server_registry();
     int64_t handle = sreg->put(server, std::move(ctx));
    std::cout << "[" << conf << "]" << std::endl;*/

    return support::make_json_buffer({
        { "websocketServerHandle", id}
    });
}

support::buffer websocket_server_stop(sl::io::span<const char> data) {
    // json parse
     auto json = sl::json::load(data);
    /*
     // int64_t handle = -1;
    // for (const sl::json::field& fi : json.as_object()) {
    //     auto& name = fi.name();
    //     if ("serverHandle" == name) {
    //         handle = fi.as_int64_or_throw(name);
    //     } else {
    //         throw support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
    //     }
    // }
    // if (-1 == handle) throw support::exception(TRACEMSG(
    //         "Required parameter 'serverHandle' not specified"));
    // // get handle
    // auto sreg = shared_server_registry();
    // auto pa = sreg->remove(handle);
    // if (nullptr == pa.first) throw support::exception(TRACEMSG(
    //         "Invalid 'serverHandle' parameter specified"));
    // // call wilton
    // char* err = wilton_Server_stop(pa.first);
    // if (nullptr != err) {
    //     sreg->put(pa.first, std::move(pa.second));
    //     support::throw_wilton_error(err, TRACEMSG(err));
    // }
    */
    uint64_t id = -1;
    for (const sl::json::field& fi : json.as_object()) {
         auto& name = fi.name();
         if ("websocketServerHandle" == name) {
             id = fi.as_int64_or_throw(name);
         } else {
             throw wilton::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
         }
     }

    websocket_servers[id]->stop();

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
    websocket_servers[wss_id]->send(ws_id, message);
    return support::make_null_buffer();
}

}// namespace websocket_server
}// namespace wilton
