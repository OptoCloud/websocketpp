#include <iostream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;


bool validate(server & s, connection_hdl hdl) {
    server::connection_ptr con = s.get_con_from_hdl(hdl);

    std::cout << "Cache-Control: " << con->get_request_header("Cache-Control") << std::endl;

    std::span<const std::string> subp_requests = con->get_requested_subprotocols();

    for (const auto& req : subp_requests) {
        std::cout << "Requested: " << req << std::endl;
    }

    if (subp_requests.size() > 0) {
        con->select_subprotocol(subp_requests[0]);
    }

    return true;
}

int main() {
    try {
        server s;

        s.set_validate_handler(bind(&validate,ref(s),::_1));

        s.init_asio();
        s.listen(9005);
        s.start_accept();

        s.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}
