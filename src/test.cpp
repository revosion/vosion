#include <vector>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>
#include <chrono>
#include <fstream>
#include <streambuf>
#include <string>
#include <json11.hpp>
#include "vfd.h"
#include <typeinfo>
using namespace json11;
using namespace vosion;

int main()
{
    std::ifstream t("config.json");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    std::string err;
    const auto config = Json::parse(str, err);
    // VFD Device setup
    std::map<std::string, VarFreqDrive> vfds;
    for (auto &item : config["vfds"]["devices"].array_items())
    {
        std::cout << "type is " << typeid(item).name() << std::endl;
        VarFreqDrive vfd(item, config["vfds"]["commands"]);
        vfd.start();
        vfd.stop();
        vfd.set_frequency(222);
        const std::string name = item["name"].string_value();
        vfds[name] = vfd;
    }

}