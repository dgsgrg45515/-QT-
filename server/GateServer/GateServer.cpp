#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

#include "asioIOserverPool.h"
#include "configMgr.h"
#include "CServer.h"

int main() {
    try {
        const auto& config = configMgr::Inst();
        const auto port_text = config["GateServer"]["Port"];
        const auto port_number = std::strtoul(port_text.c_str(), nullptr, 10);
        if (port_number == 0 || port_number > 65535) {
            throw std::runtime_error("Invalid GateServer port");
        }
        const auto port = static_cast<unsigned short>(port_number);

        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int) {
            if (!error) {
                ioc.stop();
            }
            });

        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "GateServer listening on port: " << port << std::endl;
        ioc.run();
        AsioIOServicePool::GetInstance()->Stop();
    }
    catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
