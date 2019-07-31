#ifndef SENDER_H_
#define SENDER_H_




#include <net/if.h>
#include <linux/can/bcm.h>
#include <json11.hpp>

class Sender
{
public:
    Sender(char *iface);
    Sender(){};
    int Init();
    int Close();
    int Send(int msgid, json11::Json msg);
    int SendCyclic(int msgid, int ival, std::vector<json11::Json> msgs);

private:
    int socket_;
    char *iface_;
    struct sockaddr_can addr_;
    struct ifreq ifr_;
};
#endif // SENDER_H_
