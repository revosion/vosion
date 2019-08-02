#include <json11.hpp>
#include "vfd.h"
#include <iostream>

using namespace std;
using namespace json11;

namespace vosion
{
VarFreqDrive::VarFreqDrive(const Json &t_device_config, const Json &t_command_config)
    : m_device_config(t_device_config), m_command_config(t_command_config)
{
    char *can_iface = (char *)m_device_config["can_iface"].string_value().c_str();
    m_can_sender = Sender(can_iface);
};

void VarFreqDrive::send_command(const std::string t_command)
{
    Json::object msg;
    msg["param_addr"] = m_command_config[t_command]["param_addr"];
    msg["param_val"] = m_command_config[t_command]["param_val"];
    msg["operation"] = m_command_config[t_command]["operation"];
    msg["slave_id"] = m_device_config["slave_id"];
    m_can_sender.Send(m_msgid, msg);
    cout << Json(msg).dump() << endl;
};

void VarFreqDrive::send_command(const std::string t_command, const int t_value)
{
    Json::object msg;
    msg["param_addr"] = m_command_config[t_command]["param_addr"];
    msg["param_val"] = to_string(t_value);
    msg["operation"] = m_command_config[t_command]["operation"];
    msg["slave_id"] = m_device_config["slave_id"];
    m_can_sender.Send(m_msgid, msg);
    cout << Json(msg).dump() << endl;
};

void VarFreqDrive::start()
{
    send_command("start");
};

void VarFreqDrive::stop()
{
    send_command("stop");
};

void VarFreqDrive::set_frequency(int t_frequency)
{
    send_command("set_frequency", t_frequency);
};

} // namespace vosion