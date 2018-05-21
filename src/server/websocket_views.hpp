

#ifndef WEBSOCKET_VIEWS_HPP
#define WEBSOCKET_VIEWS_HPP

#include <string>
#include "staticlib/json.hpp"
//#include <iostream>

namespace wilton {
namespace websocket_server {

class ws_view {
public:
    std::string path; // Analog for module /server/views/hi
    std::string method; // ONOPEN, ONCLOSE, ONMESSAGE, ONERROR
    sl::json::value json_data;
    sl::json::value* json_data_ptr;
    bool is_setted;

    ws_view(): json_data_ptr(nullptr), is_setted(false) { }

    ws_view(const ws_view& view) {
        path     = view.path;
        method   = view.method;
        json_data  = view.json_data.clone();
        json_data_ptr = &json_data;
    }

    ws_view& operator =(const ws_view& view) {
        this->path     = view.path;
        this->method   = view.method;
        this->json_data  = view.json_data.clone();
        this->json_data_ptr = &(this->json_data);
        this->is_setted = view.is_setted;
        return *this;
    }

    void setup_values(const sl::json::value& json){
        if (sl::json::type::object != json.json_type()) throw sl::support::exception(TRACEMSG(
                "Invalid 'views' entry: must be an 'object'," +
                " entry: [" + json.dumps() + "]"));
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("method" == name) {
                method = fi.as_string_nonempty_or_throw(name);
            } else if ("path" == name) {
                path = fi.as_string_nonempty_or_throw(name);
            } else if ("callbackScript" == name) {
                json_data = fi.val().clone();
                json_data_ptr = &json_data;
            } else {
                throw sl::support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
            }
        }
        is_setted = true;
    }

//    ~ws_view(){
//         std::cout<< "----- delete ws_view" << std::endl;
//    }
};

class ws_views {
public:
    ws_view onopen_view;
    ws_view onclose_view;
    ws_view onerror_view;
    ws_view onmessage_view;

    ws_views() {}
    ws_views(const ws_views& views) {
        onopen_view    = views.onopen_view;
        onclose_view   = views.onclose_view;
        onerror_view   = views.onerror_view;
        onmessage_view = views.onmessage_view;
    }
};

} // namespace websocket_server
} // namespace wilton

#endif /* WEBSOCKET_VIEWS_HPP */
