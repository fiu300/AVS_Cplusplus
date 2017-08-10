

#ifndef TCPSERVERRUN_HH
#define TCPSERVERRUN_HH

#include "agent.h"


// int tcpServerRun(std::thread &wsthread, t_agent& a_agent);
int tcpServerRun(t_agent *a_agent);
void tcpServerSetClientId(std::string &cid);

#endif //TCPSERVERRUN_HH

