/*
// Copyright (c) 2025 Hewlett Packard Enterprise Development, LP
//
// Hewlett-Packard and the Hewlett-Packard logo are trademarks of
// Hewlett-Packard Development Company, L.P. in the U.S. and/or other countries.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <fstream>
#include <iostream>
#include <string>


static constexpr bool DEBUG = false;

boost::asio::io_context io;
auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
auto objServer = sdbusplus::asio::object_server(systemBus);

const std::string unknown("Unknown");
const std::string procdevtree("/proc/device-tree/");

std::string GetServerId()
{
    std::string id;
    std::ifstream fin("/sys/class/soc/xreg/server_id");
    if(!fin)
        return unknown;
    getline(fin, id);
    fin.close();
    return id;
}

std::string GetManufacturer()
{
    static const std::string manufacturer("Hewlett Packard Enterprise");
    return manufacturer;
}

std::string GetProcDevTreeVal(const std::string &name)
{
    std::string id;
    std::ifstream fin(procdevtree+name);
    if(!fin)
        return unknown;
    getline(fin, id);
    fin.close();
    return id;
}

std::string GetPartNumber()
{
    return GetProcDevTreeVal("pn");
}

std::string GetSerialNumber()
{
    return GetProcDevTreeVal("sn");
}

std::string GetPcaPartNumber()
{
    return GetProcDevTreeVal("model");
}

std::string GetPcaSerialNumber()
{
    return GetProcDevTreeVal("serial-number");
}

void DumpFRU()
{
    std::cout << "SERVER_ID=" << GetServerId() << std::endl;
    std::cout << "PRODUCT_MANUFACTURER=" << GetManufacturer() << std::endl;
    std::cout << "PartNumber=" << GetPartNumber() << std::endl;
    std::cout << "SerialNumber=" << GetSerialNumber() << std::endl;
    std::cout << "PCAPartNumber=" << GetPcaPartNumber() << std::endl;
    std::cout << "PCASerialNumber=" << GetPcaSerialNumber() << std::endl;
}

void AddFRUObjectToDbus(std::shared_ptr<sdbusplus::asio::dbus_interface>& iface)
{
    std::string productName = "/xyz/openbmc_project/FruDevice/HPE";
    iface = objServer.add_interface(productName, "xyz.openbmc_project.FruDevice");

    iface->register_property("SERVER_ID", GetServerId());
    iface->register_property("PRODUCT_MANUFACTURER", GetManufacturer());
    iface->register_property("PRODUCT_PART_NUMBER", GetPartNumber());
    iface->register_property("PRODUCT_SERIAL_NUMBER", GetSerialNumber());
    iface->register_property("PCA_PART_NUMBER", GetPcaPartNumber());
    iface->register_property("PCA_SERIAL_NUMBER", GetPcaSerialNumber());

    iface->initialize();
}

void rescanBus(std::shared_ptr<sdbusplus::asio::dbus_interface>& iface)
{
    if(iface != nullptr) {
        objServer.remove_interface(iface);
    }
    AddFRUObjectToDbus(iface);
}

int main()
{
#if 0
    DumpFRU();
#endif
    systemBus->request_name("xyz.openbmc_project.GscFruDevice");

    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceFruDevice = nullptr;
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceFruDeviceManager =
        objServer.add_interface("/xyz/openbmc_project/FruDevice",
                                "xyz.openbmc_project.FruDeviceManager");

    ifaceFruDeviceManager->register_method("ReScan", [&]() {
        rescanBus(ifaceFruDevice);
    });
    ifaceFruDeviceManager->initialize();

    // run the initial scan
    rescanBus(ifaceFruDevice);

    io.run();

    return 0;
}
