/*
 * Copyright 2016, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   wilton_server.cpp
 * Author: alex
 * 
 * Created on June 4, 2016, 8:15 PM
 */

#include "wilton/wilton_server.h"

#include <functional>
#include <vector>

#include "staticlib/config.hpp"
#include "staticlib/json.hpp"
#include "staticlib/utils.hpp"

#include "wilton/support/alloc.hpp"
#include "wilton/support/buffer.hpp"

#include "server/websocket_views.hpp"
#include "staticlib/pion.hpp"

#include "server/http_path.hpp"
#include "server/request.hpp"
#include "server/response_writer.hpp"
#include "server/server.hpp"

#include "serverconf/response_metadata.hpp"

struct wilton_Server {
private:
    wilton::server::server delegate;

public:
    wilton_Server(wilton::server::server&& delegate) :
    delegate(std::move(delegate)) { }

    wilton::server::server& impl() {
        return delegate;
    }
};

struct wilton_Request {
private:
    sl::support::observer_ptr<wilton::server::request> delegate;
    
public:
    wilton_Request(wilton::server::request& delegate) :
    delegate(std::addressof(delegate)) { }

    wilton::server::request& impl() {
        return *delegate;
    }
};

struct wilton_ResponseWriter {
private:
    wilton::server::response_writer delegate;

public:
    wilton_ResponseWriter(wilton::server::response_writer&& delegate) :
    delegate(std::move(delegate)) { }

    wilton::server::response_writer& impl() {
        return delegate;
    }
};

struct wilton_HttpPath {
private:
    wilton::server::http_path delegate;

public:

    wilton_HttpPath(wilton::server::http_path&& delegate) :
    delegate(std::move(delegate)) { }

    wilton::server::http_path& impl() {
        return delegate;
    }
};

struct wilton_WebsocketPath {
private:
    wilton::websocket_server::ws_views delegate;

public:

    wilton_WebsocketPath(wilton::websocket_server::ws_views&& delegate) :
    delegate(std::move(delegate)) { }

    wilton::websocket_server::ws_views& impl() {
        return delegate;
    }
};

struct wilton_WebsocketView {
private:
    wilton::websocket_server::ws_view delegate;

public:

    wilton_WebsocketView(wilton::websocket_server::ws_view&& delegate) :
    delegate(std::move(delegate)) { }

    wilton::websocket_server::ws_view& impl() {
        return delegate;
    }
};

struct wilton_WebsocketServiceData {
private:
    sl::pion::websocket_service_data* delegate;

public:
    // Здесь тоже не требуется хранение, достаточно скопировать shared_ptr
    wilton_WebsocketServiceData(sl::pion::websocket_service_data* delegate)
        : delegate(delegate) {
    }

    sl::pion::websocket_service_data* impl() {
        return delegate;
    }
};

// Do not store websocket_service object, stores ptr to that object
// Used only like wrapper for prepare callback function
struct wilton_WebsocketService {
private:
    sl::pion::websocket_service_ptr delegate;

public:
    wilton_WebsocketService(sl::pion::websocket_service_ptr delegate) {
        this->delegate.swap(delegate);
    }

    sl::pion::websocket_service_ptr impl() {
        return delegate;
    }
};

namespace { // anonymous

std::vector<sl::support::observer_ptr<wilton::server::http_path>> wrap_paths(wilton_HttpPath** paths, uint16_t paths_len) {
    std::vector<sl::support::observer_ptr<wilton::server::http_path>> res;
    for (int i = 0; i < paths_len; i++) {
        wilton_HttpPath* ptr = paths[i];
        auto obs = sl::support::make_observer_ptr(ptr->impl());
        res.push_back(obs);
    }
    return res;
}

std::vector<sl::pion::websocket_service_data*> wrap_ws_data(wilton_WebsocketServiceData** paths, uint16_t paths_len) {
    std::vector<sl::pion::websocket_service_data*> res;
    for (int i = 0; i < paths_len; i++) {
        wilton_WebsocketServiceData* ptr = paths[i];
        res.push_back(ptr->impl());
    }
    return res;
}


} // namespace

char* wilton_HttpPath_create(wilton_HttpPath** http_path_out, const char* method, int method_len,
        const char* path, int path_len, void* handler_ctx, 
        void (*handler_cb)(void* handler_ctx, wilton_Request* request)) {
    if (nullptr == http_path_out) return wilton::support::alloc_copy(TRACEMSG("Null 'http_path_out' parameter specified"));
    if (nullptr == method) return wilton::support::alloc_copy(TRACEMSG("Null 'method' parameter specified"));
    if (!sl::support::is_uint16_positive(method_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'method_len' parameter specified: [" + sl::support::to_string(method_len) + "]"));
    if (nullptr == path) return wilton::support::alloc_copy(TRACEMSG("Null 'path' parameter specified"));
    if (!sl::support::is_uint16_positive(path_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'path_len' parameter specified: [" + sl::support::to_string(path_len) + "]"));
    if (nullptr == handler_cb) return wilton::support::alloc_copy(TRACEMSG("Null 'handler_cb' parameter specified"));
    try {
        uint16_t method_len_u16 = static_cast<uint16_t> (method_len);
        std::string method_str = std::string(method, method_len_u16);
        uint16_t path_len_u16 = static_cast<uint16_t> (path_len);
        std::string path_str = std::string(path, path_len_u16);
        auto ha_ctx = handler_ctx;
        auto ha_cb = handler_cb;
        auto handler = [ha_ctx, ha_cb](wilton::server::request& req) {
            wilton_Request req_pass{req};
            ha_cb(ha_ctx, std::addressof(req_pass));
        };
        wilton::server::http_path http_path{std::move(method_str), std::move(path_str), handler};
        wilton_HttpPath* http_path_ptr = new wilton_HttpPath(std::move(http_path));
        *http_path_out = http_path_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_HttpPath_destroy(wilton_HttpPath* path) {
    delete path;
    return nullptr;
}


char*wilton_WebsocketView_create(
        wilton_WebsocketView** ws_view_out, const char* in_json,
        int in_json_len){
    if (nullptr == ws_view_out) return wilton::support::alloc_copy(TRACEMSG("Null 'ws_view_out' parameter specified"));
    try {
        wilton::websocket_server::ws_view view;
        uint16_t json_len_u16 = static_cast<uint16_t> (in_json_len);
        sl::json::value json = sl::json::loads(std::string{in_json, json_len_u16});
        view.setup_values(json);
        // copy prepared views
        wilton_WebsocketView* ws_view_ptr = new wilton_WebsocketView(std::move(view));
        *ws_view_out = ws_view_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_WebsocketView_destroy(wilton_WebsocketView* view) {
    if (nullptr != view) {
        delete view;
        view = nullptr;
    }
    return nullptr;
}

char* wilton_WebsocketPath_create(
        wilton_WebsocketPath** ws_path_out,
        wilton_WebsocketView* open_view,
        wilton_WebsocketView* close_view,
        wilton_WebsocketView* error_view,
        wilton_WebsocketView* message_view){
    if (nullptr == ws_path_out) return wilton::support::alloc_copy(TRACEMSG("Null 'http_path_out' parameter specified"));
    // other parameters may be null
    try {
        wilton::websocket_server::ws_views ws_path;
        // copy prepared views
        if (nullptr != open_view) ws_path.onopen_view = open_view->impl();
        if (nullptr != close_view) ws_path.onclose_view = close_view->impl();
        if (nullptr != error_view) ws_path.onerror_view = error_view->impl();
        if (nullptr != message_view) ws_path.onmessage_view = message_view->impl();
        wilton_WebsocketPath* ws_path_ptr = new wilton_WebsocketPath(std::move(ws_path));
        *ws_path_out = ws_path_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_WebsocketPath_destroy(wilton_WebsocketPath* path) {
    if (nullptr != path) {
        delete path;
        path = nullptr;
    }
    return nullptr;
}


char* wilton_WebsocketServiceData_create(wilton_WebsocketServiceData** ws_service_data_out,
                                         wilton_WebsocketPath* path,
                                         void (*onopen_cb)(void* passed, void* user_data),
                                         void (*onclose_cb)(void* passed, void* user_data),
                                         void (*onerror_cb)(void *passed, void *user_data, const char *err, int err_len),
                                         void (*onmessage_cb)(void *passed, void *user_data, const char *msg, int msg_len),
                                         void (*main_prepare_cb)(wilton_WebsocketService* ws_service))
{
    if (nullptr == ws_service_data_out) return wilton::support::alloc_copy(TRACEMSG("Null 'ws_service_data_out' parameter specified"));
    try {
        sl::pion::websocket_service_data* data = new sl::pion::websocket_service_data();//std::make_shared<sl::pion::websocket_service_data>();
        if (path->impl().onopen_view.is_setted) {
            data->open_data = static_cast<void*> (path->impl().onopen_view.json_data_ptr);
            data->open_handler = onopen_cb;
        }
        if (path->impl().onclose_view.is_setted) {
            data->close_data = static_cast<void*> (path->impl().onclose_view.json_data_ptr);
            data->close_handler = onclose_cb;
        }
        if (path->impl().onerror_view.is_setted) {
            data->error_data = static_cast<void*> (path->impl().onerror_view.json_data_ptr);
            data->error_handler = onerror_cb;
        }
        if (path->impl().onmessage_view.is_setted) {
            data->message_data = static_cast<void*> (path->impl().onmessage_view.json_data_ptr);
            data->message_handler = onmessage_cb;
        }

        auto prepare_handler = [main_prepare_cb] (sl::pion::websocket_service_ptr service) {
            wilton_WebsocketService* wilton_service = new wilton_WebsocketService{service}; // TODO mayde different path of creation?
            main_prepare_cb(wilton_service);
        };

        data->websocket_prepare_handler = prepare_handler;

        data->resource = path->impl().onopen_view.path;

        wilton_WebsocketServiceData* ws_data_ptr = new wilton_WebsocketServiceData{data};
        *ws_service_data_out = ws_data_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}
char* wilton_WebsocketServiceData_destroy(wilton_WebsocketServiceData* ws_service_data){
    if (nullptr != ws_service_data) {
        delete ws_service_data;
        ws_service_data = nullptr;
    }
    return nullptr;
}

char* wilton_WebsocketService_create(wilton_WebsocketService** ws_service_out, wilton_WebsocketServiceData* service_data){
    (void)ws_service_out;
    (void)service_data;
    return nullptr;
}
char* wilton_WebsocketService_destroy(wilton_WebsocketService* ws_service){
    if (nullptr != ws_service) {
        if (nullptr != ws_service->impl()){
            void* user_data_ptr = ws_service->impl()->get_user_data();
            delete static_cast<uint64_t*> (user_data_ptr);
//            delete ws_service->impl();
            delete ws_service;
        }
        ws_service = nullptr;
    }
    return nullptr;
}
void wilton_WebsocketService_setup_user_data(wilton_WebsocketService* ws_service, void* user_data){
    ws_service->impl()->setup_user_data(user_data);
}
void wilton_WebsocketService_send(wilton_WebsocketService* ws_service, const char *err, int err_len){
    std::string message{err, static_cast<uint64_t> (err_len)};
   ws_service->impl()->send(message);
}

char* wilton_Server_create /* noexcept */ (wilton_Server** server_out, const char* conf_json,
        int conf_json_len, wilton_HttpPath** paths, int paths_len, wilton_WebsocketServiceData** ws_data, int ws_data_len) /* noexcept */ {
    if (nullptr == server_out) return wilton::support::alloc_copy(TRACEMSG("Null 'server_out' parameter specified"));    
    if (nullptr == conf_json) return wilton::support::alloc_copy(TRACEMSG("Null 'conf_json' parameter specified"));
    if (!sl::support::is_uint32_positive(conf_json_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'conf_json_len' parameter specified: [" + sl::support::to_string(conf_json_len) + "]"));
    if (!sl::support::is_uint16(paths_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'paths_len' parameter specified: [" + sl::support::to_string(paths_len) + "]"));
    if (sl::support::is_uint16_positive(paths_len) && nullptr == paths) return wilton::support::alloc_copy(TRACEMSG("Null 'paths' parameter specified"));
    try {
        sl::json::value json = sl::json::load({conf_json, conf_json_len});
        uint16_t paths_len_u16 = static_cast<uint16_t>(paths_len);
        auto pathsvec = wrap_paths(paths, paths_len_u16);

//        if (nullptr != ws_data) {
        auto datavec = wrap_ws_data(ws_data, static_cast<uint16_t>(ws_data_len));
//        }
        wilton::server::server server{std::move(json), std::move(pathsvec), std::move(datavec)};
        wilton_Server* server_ptr = new wilton_Server(std::move(server));
        *server_out = server_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Server_stop(wilton_Server* server) /* noexcept */ {
    if (nullptr == server) return wilton::support::alloc_copy(TRACEMSG("Null 'server' parameter specified"));
    try {
        server->impl().stop();
        delete server;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_get_request_metadata(wilton_Request* request, char** metadata_json_out,
        int* metadata_json_len_out) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'server' parameter specified"));
    if (nullptr == metadata_json_out) return wilton::support::alloc_copy(TRACEMSG("Null 'metadata_json_out' parameter specified"));
    if (nullptr == metadata_json_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'metadata_json_len_out' parameter specified"));
    try {
        auto meta = request->impl().get_request_metadata();
        sl::json::value json = meta.to_json();
        std::string res = json.dumps();
        *metadata_json_out = wilton::support::alloc_copy(res);
        *metadata_json_len_out = static_cast<int>(res.length());
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_get_request_data(wilton_Request* request, char** data_out,
        int* data_len_out) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == data_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_out' parameter specified"));
    if (nullptr == data_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_len_out' parameter specified"));
    try {
        const std::string& res = request->impl().get_request_data();
        *data_out = wilton::support::alloc_copy(res);
        *data_len_out = static_cast<int>(res.length());
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_get_request_form_data(wilton_Request* request, char** data_out,
        int* data_len_out) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == data_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_out' parameter specified"));
    if (nullptr == data_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_len_out' parameter specified"));
    try {
        sl::json::value json = request->impl().get_request_form_data();
        wilton::support::buffer res = wilton::support::make_json_buffer(json);
        *data_out = res.data();
        *data_len_out = res.size_int();
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_get_request_data_filename(wilton_Request* request, 
        char** filename_out, int* filename_len_out) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == filename_out) return wilton::support::alloc_copy(TRACEMSG("Null 'filename_out' parameter specified"));
    if (nullptr == filename_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'filename_len_out' parameter specified"));
    try {
        const std::string& res = request->impl().get_request_data_filename();
        *filename_out = wilton::support::alloc_copy(res);
        *filename_len_out = static_cast<int>(res.length());
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

// TODO: fixme json copy
char* wilton_Request_set_response_metadata(wilton_Request* request,
        const char* metadata_json, int metadata_json_len) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == metadata_json) return wilton::support::alloc_copy(TRACEMSG("Null 'metadata_json' parameter specified"));
    if (!sl::support::is_uint32_positive(metadata_json_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'metadata_json_len' parameter specified: [" + sl::support::to_string(metadata_json_len) + "]"));
    try {
        sl::json::value json = sl::json::load({metadata_json, metadata_json_len});
        wilton::serverconf::response_metadata rm{json};
        request->impl().set_response_metadata(std::move(rm));
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_send_response(wilton_Request* request, const char* data,
        int data_len) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == data) return wilton::support::alloc_copy(TRACEMSG("Null 'data' parameter specified"));
    if (!sl::support::is_uint32(data_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'data_len' parameter specified: [" + sl::support::to_string(data_len) + "]"));
    try {
        uint32_t data_len_u32 = static_cast<uint32_t> (data_len);
        request->impl().send_response(data, data_len_u32);
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_send_file(
        wilton_Request* request,
        const char* file_path,
        int file_path_len,
        void* finalizer_ctx,
        void (*finalizer_cb)(
        void* finalizer_ctx,
        int sent_successfully)) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == file_path) return wilton::support::alloc_copy(TRACEMSG("Null 'file_path' parameter specified"));
    if (!sl::support::is_uint16_positive(file_path_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'file_path_len' parameter specified: [" + sl::support::to_string(file_path_len) + "]"));
    if (nullptr == finalizer_cb) return wilton::support::alloc_copy(TRACEMSG("Null 'finalizer_cb' parameter specified"));
    try {
        uint16_t file_path_len_u16 = static_cast<uint16_t> (file_path_len);
        std::string file_path_str{file_path, file_path_len_u16};
        request->impl().send_file(std::move(file_path_str),
                [finalizer_ctx, finalizer_cb](bool success) {
                    int success_int = success ? 1 : 0;
                    finalizer_cb(finalizer_ctx, success_int);
                });
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_send_mustache(
        wilton_Request* request,
        const char* mustache_file_path,
        int mustache_file_path_len,
        const char* values_json,
        int values_json_len) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));
    if (nullptr == mustache_file_path) return wilton::support::alloc_copy(TRACEMSG("Null 'mustache_file_path' parameter specified"));
    if (!sl::support::is_uint16_positive(mustache_file_path_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'mustache_file_path_len' parameter specified: [" + sl::support::to_string(mustache_file_path_len) + "]"));
    if (nullptr == values_json) return wilton::support::alloc_copy(TRACEMSG("Null 'values_json' parameter specified"));
    if (!sl::support::is_uint32_positive(values_json_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'values_json_len' parameter specified: [" + sl::support::to_string(values_json_len) + "]"));
    try {
        uint16_t mustache_file_path_len_u16 = static_cast<uint16_t> (mustache_file_path_len);
        std::string mustache_file_path_str{mustache_file_path, mustache_file_path_len_u16};
        uint32_t values_json_len_u32 = static_cast<uint32_t> (values_json_len);
        std::string values_json_str{values_json, values_json_len_u32};
        sl::json::value json = sl::json::loads(values_json_str);
        request->impl().send_mustache(std::move(mustache_file_path_str), std::move(json));
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Request_send_later(
        wilton_Request* request,
        wilton_ResponseWriter** writer_out) /* noexcept */ {
    if (nullptr == request) return wilton::support::alloc_copy(TRACEMSG("Null 'request' parameter specified"));    
    if (nullptr == writer_out) return wilton::support::alloc_copy(TRACEMSG("Null 'writer_out' parameter specified"));
    try {
        wilton::server::response_writer writer = request->impl().send_later();
        wilton_ResponseWriter* writer_ptr = new wilton_ResponseWriter(std::move(writer));
        *writer_out = writer_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_ResponseWriter_send(
        wilton_ResponseWriter* writer,
        const char* data,
        int data_len) /* noexcept */ {
    if (nullptr == writer) return wilton::support::alloc_copy(TRACEMSG("Null 'writer' parameter specified"));
    if (nullptr == data) return wilton::support::alloc_copy(TRACEMSG("Null 'data' parameter specified"));
    if (!sl::support::is_uint32(data_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'data_len' parameter specified: [" + sl::support::to_string(data_len) + "]"));
    try {
        uint32_t data_len_u32 = static_cast<uint32_t> (data_len);
        writer->impl().send(data, data_len_u32);
        delete writer;
        return nullptr;
    } catch (const std::exception& e) {
        delete writer;
        return wilton::support::alloc_copy(TRACEMSG("WRITER_INVALIDATED\n" + e.what() + "\nException raised"));
    }    
}
