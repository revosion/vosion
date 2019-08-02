#ifndef VOSION_VFD_H_
#define VOSION_VFD_H_

#include <json11.hpp>
#include "sender.h"

namespace vosion
{
class VarFreqDrive
{
public:
    VarFreqDrive(const json11::Json &t_device_config, const json11::Json &t_command_config); //, const Sender &t_can_sender);
    VarFreqDrive(){};
    void start();
    void stop();
    //    void set_frequency(const int t_frequency);
    void set_frequency( int t_frequency);

private:
    json11::Json m_device_config;
    json11::Json m_command_config;
    Sender m_can_sender;
    void send_command(const std::string t_command, const int t_value);
    void send_command(const std::string t_command);
    int m_msgid{819};
};
} // namespace vosion
#endif // VOSION_VFD_H_