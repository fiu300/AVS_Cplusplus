#include <future>
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "json.h"
#include "config.h"
#include "agent.h"
#include "tcpserver.h"

#define  QUEUE_LINE  12
#define  SOURCE_PORT 41200
#define  SOURCE_IP_ADDRESS  "192.168.8.45"  // "192.168.1.123" //"192.168.31.117"

#define RECVLEN 1024
#define TAGLEN 2
#define STLENLEN 1
#define EXLENLEN 2
#define SUMLEN 1

#define BROADCAST_IP     "239.12.12.1"
#define BROADCAST_PORT   30001


typedef enum {
    UART_COMMAD=0x0,
    UOP_NEXT,
    UOP_PREV,
    UOP_PLAYPAUSE,
    UOP_VOLUP,
    UOP_VOLDOWN,
    UOP_STOP,
    UOP_GETVOL,
    UOP_GETSTATUS,
    UOP_NETMUSIC,
    UOP_NEXTC,
    UOP_PREVC,
    UOP_MUTE,
    UOP_SETVOL,
    UOP_PREVALBUM,
    UOP_NEXTALBUM,
    UOP_SWITCHSCR,
    UOP_PREVARTIST,
    UOP_NEXTARTIST,
    UOP_GETPBINFO,
    NET_COMMAND=0x20,
    NET_GETSERNAME,
    NET_SETSERNAME,
    NET_RETURNNAME,
    NET_SWITCHAP,
    NET_SWITCHSTA,
    NET_SCANSSID,
    NET_RETURNSSID,
    NET_SWITCHBRIDGE,
    NET_GETMODE,
    SYS_COMMNAD=0x40,
    SYS_PBSTATUS,
    SYS_NETSTATUS,
    SYS_STUS,
    SYS_RESET,
    SYS_SIMPLELINK,
    SYS_UPGRADE,
    SYS_GET_VERSION,
    SYS_RETURN_VERSION,
    SYS_PLAY_WARNINGTONE=0x50,
    SYS_HEARTBEAT=0x51,
    SYS_SONGSWITCH=0x52,
    SYS_PBINFO=0x53,
    SYS_DEVINFO=0x54,
    SYS_USERREG=0x55,
    SYS_GETINFO=0x56,
    USER_COMMAND=0x60,
    PWM_CMD_SET_COLOR_OP = 0x61,
    PWM_CMD_SET_BRIGHTNESS_OP = 0x62,
    PWM_CMD_SET_LED_MODE_OP = 0x63,
    PWM_CMD_REM_COLOR_OP = 0x64,
    PWM_CMD_SET_FACTORY_OP = 0x65,
    PWM_CMD_GET_STATUS_OP = 0x66,
    PWM_CMD_GET_STATUS_RES = 0x67,
    PWM_CMD_SET_VAPOR_OP = 0x68,
    USER_SET_ALARM = 0x71,
    USER_BROWSE_ALARM = 0x72,
    USER_BROWSE_ALARM_RES = 0x73,
    USER_REMOVE_ALARM = 0x74,
    USER_SET_TIMEZONE = 0x75,
    USER_GET_TIMEZONE = 0x76,
    USER_GET_TIMEZONE_RES = 0x77,
    MUL_COMMAND=0x80,
    MUL_SETMASTER,
    MUL_SETSLAVER,
    MUL_SETCHMODE,
    MUL_GETSYNC,
    MUL_RETSYNC,
    EQ_COMMAND=0x90,
    EQ_SOUNDEFFECT,
    EQ_SOUNDDB,
    EQ_SOUNDBL,
    EQ_SOUNDVOL,
    EQ_INPUTSIG,
    EQ_SETINPUT,
    EQ_SETSUBWOOFER,
    EQ_SETPREMIX,
    EQ_SETPOLARITY,
    EQ_GETDEVINFO=0x9a,
    MAX_COMMON_ID=0xAF,
    GPIO_COMMAND=0xFA,
    GPIO_LONG_COMMAND=0xFB
}CONFIG_RKEY_TYPE;


typedef struct _remote_cmd_{
    CONFIG_RKEY_TYPE        type;
    unsigned char           cmdid;
    unsigned char   *       data;
    int                     len;
    struct _remote_cmd_ *   next;
}REMOTE_CMD;



using namespace std;


void process_info(int sock);
int FindFirstTag(unsigned char * readbuf,int len);
unsigned char handlerUserOP(int usock, unsigned char opCode,unsigned char * data,int len);
unsigned short Uartcmd_Notify(int usock, unsigned char opCode,unsigned char * data,unsigned int len);
unsigned char  process_net(int sock, unsigned char userOP, unsigned char * data,int len);
int mk_json(string &dest);
int parse_json(unsigned char *data, int len);
int mk_devinfo_json(string &dest);

void f_grantx(std::map<std::string, std::string>&& a_query);

unsigned char CmdBufUart[RECVLEN];

bool g_can_refresh = false;

string g_clientId;
string g_redirectUri;
string g_authCode;
string g_codeVerifier;

void tcpServerSetClientId(std::string &clientid)
{
    g_clientId = clientid;
}

// int tcpServerRun(std::thread &wsthread, t_agent& a_agent)
int tcpServerRun(t_agent *a_agent)
{
    int on;
    int socket_fd,accept_fd;
    sockaddr_in server;
    sockaddr_in remote_addr;
    int conn_fd;

    int broadcast_sockfd; // 多播套接字文件描述符
    struct sockaddr_in broadcast_addr; // 多播ip


    if(( socket_fd = socket(AF_INET,SOCK_STREAM, 0)) < 0 ){
        throw ("socket() failed");
    }

    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);  //inet_addr(SOURCE_IP_ADDRESS); //
    server.sin_port = htons(SOURCE_PORT);

    /* connect the socket */
    while(1) {
        if(connect(socket_fd, (struct sockaddr *)&server, sizeof(struct sockaddr))<0){
            sleep(1);
            continue;
        }
        break;
    }
    int flag = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flag|O_NONBLOCK);  //non-block operation for the connect socket operation

    int nSendBuf=256;
    setsockopt(socket_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));

    broadcast_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // 建立套接字
    if (broadcast_sockfd == -1) {
        perror("socket()");
        return -1;
    }
    // 初始化多播 ip 信息
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP); // 目的地址，为多播地址
    broadcast_addr.sin_port = htons(BROADCAST_PORT);   // 多播服务器的端口


    // std::thread wsthread;
    // wsthread = std::thread([&]
    // {
        // sleep(10);

        printf("TcpServer run................................:\n");

        pid_t pid;
        pthread_t procid;
        pthread_attr_t attr ;
        pthread_attr_init( &attr ) ;

        socklen_t client_len = sizeof(struct sockaddr_in);;
        struct sockaddr_in  addr_client;

        int maxfdp=socket_fd+1;
        fd_set readfds, writefds;
        struct timeval timeOut={0,1};

        int nfds;

        string devinfo;

        printf("begin accept:\n");

        unsigned  int icnt = 0;

        while (true) {                      /// 由于accept会阻塞，导致不能实时响应退出
            printf("` \n");

            FD_ZERO(&readfds);
            FD_SET(socket_fd, &readfds);

            FD_ZERO(&writefds);
            FD_SET(broadcast_sockfd, &writefds);
            maxfdp = broadcast_sockfd > socket_fd ? (broadcast_sockfd+1):(socket_fd+1);

            // printf("-->%s,%d\n", __FUNCTION__, __LINE__);

            nfds = select(maxfdp, &readfds, &writefds, NULL, &timeOut);
            if (nfds > 0) {
                if(FD_ISSET(socket_fd, &readfds)){
                    process_info(socket_fd);
                    if (g_can_refresh) {
                        g_can_refresh = false;

                        a_agent->f_grant(g_clientId,g_authCode,g_redirectUri,g_codeVerifier, [&](auto)
                        {
                            ;// if (a_agent->f_session()) 
                            //     printf("go to grant\n");
                        });
                    }
                }
 
                if(FD_ISSET(broadcast_sockfd, &writefds)){
                    mk_devinfo_json(devinfo);
                    sendto(broadcast_sockfd, devinfo.c_str(), devinfo.size(), 0,(struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
                    // printf("send %s\n", devinfo.c_str());
                }

            } else if (nfds < 0) {
                //break;
                std::cout<<"select error."<<std::endl;
            }
            if(icnt < 100)
                sleep(1);
            else
                sleep(5);

            icnt++;

            // printf("-->%s,%d\n", __FUNCTION__, __LINE__);
        }
    // });

    printf("-->%s,%d\n", __FUNCTION__, __LINE__);

    close(socket_fd);
    close(broadcast_sockfd);

    return 0;
}



void process_info(int sock)
{
    int i;
    int recvLen = 0;
    static int tLen =0;

    //usock = sock;

        recvLen = recv(sock, (CmdBufUart+tLen), (RECVLEN-tLen), 0);

        if(recvLen > 0){

            tLen += recvLen;

            // for(i=0; i<tLen; i++) {
            //     printf("%02x ", CmdBufUart[i]);
            // } printf("\n");

            while(tLen > 4){
                int fLen = FindFirstTag(CmdBufUart,tLen);
                // printf("fLen = %d\n", fLen);
                if(fLen < 0)
                {   //not found tag 0xff 0x55 or 0xff 0x56
                    if(tLen > 256)
                    {
                        memmove(CmdBufUart, CmdBufUart+250, (tLen-250));
                        tLen -= 250;
                    }
                    break;
                }
                else if(fLen+5 > tLen)
                {
                    //not enough,remove the unused data,just copy the head
                    if(tLen > 256)
                    {
                        memmove(CmdBufUart, CmdBufUart+250, (tLen-250));
                        tLen -= 250;
                    }

                    break;
                }
                else
                {
                    int LENLEN = 1,cmdLen=0 ,i=0;
                    unsigned char a = 0;
                    if(CmdBufUart[fLen+TAGLEN-1] == 0x55)
                    {
                        LENLEN = STLENLEN;
                        cmdLen = CmdBufUart[fLen+TAGLEN];
                        a -= CmdBufUart[fLen+TAGLEN];
                    }
                    else if(CmdBufUart[fLen+TAGLEN-1] == 0x56)
                    {
                        LENLEN = EXLENLEN;
                        cmdLen = CmdBufUart[fLen+TAGLEN]<<8|CmdBufUart[fLen+TAGLEN+1];
                        a -= CmdBufUart[fLen+TAGLEN];
                        a -= CmdBufUart[fLen+TAGLEN+1];
                    }

                    if(cmdLen+TAGLEN+LENLEN+SUMLEN > tLen)
                    {
                        //not enough,remove!
                        break;
                    }

                    unsigned char checksum = CmdBufUart[fLen+TAGLEN+LENLEN+cmdLen];

                    while(i<cmdLen){
                        a-= CmdBufUart[fLen+TAGLEN+LENLEN+i];
                        i++;
                    };

                    if(checksum == a)
                    {
                        unsigned char cmdnum = CmdBufUart[fLen+TAGLEN+LENLEN];

                        printf("cmdnum = %x,  cmdLen=%d\n", cmdnum, cmdLen);

                        if(cmdnum < 0x60 || cmdnum > 0x80){
                            if(cmdLen > 1){
                                unsigned char * cmdData = (unsigned char *)malloc(cmdLen+1);
                                memcpy(cmdData,  (const unsigned char *)(CmdBufUart+fLen+TAGLEN+LENLEN+1),  (cmdLen-1));
                                handlerUserOP(sock, cmdnum,  cmdData, (cmdLen-1));
                            }
                            else
                                handlerUserOP(sock, cmdnum,NULL,0);
                        }
                    }
                    else
                    {//skip current tag and find next
                        tLen -= (2+fLen);
                        memmove(CmdBufUart,(CmdBufUart+2+fLen),tLen);
                        continue;
                    }
                    tLen = tLen - fLen -(cmdLen+TAGLEN+LENLEN+SUMLEN);

                    memmove(CmdBufUart,CmdBufUart+(fLen+cmdLen+TAGLEN+LENLEN+SUMLEN),tLen);
                }
            }
        }
}

unsigned short Uartcmd_Notify(int usock, unsigned char opCode, unsigned char * data,unsigned int len)
{
    unsigned char cmd[1024];
    int sendlen = 0;
    int index = 0;

    if(usock < 0)
        return 0;

    cmd[index]=0xFF;
    index++;
    if(len < 255)
    {
        cmd[index]=0x55;
        index++;
        cmd[index]=len+1;
        index++;
    }else
    {
        unsigned int tmp =0;
        cmd[index]=0x56;
        index++;
        tmp = len+1;
        cmd[index] = (unsigned char)((tmp>>8)&0xff) ;
        index++;
        cmd[index] = (unsigned char)(tmp&0xff) ;
        index++;
    }

    cmd[index]=opCode;
    index++;
    memcpy(&cmd[index],data,len);

    cmd[index+len] = 0;
    int i = 0;
    while(i < len+(index-2)){
        cmd[index+len] -= cmd[2+i];
        i++;
    }
    index++;
    sendlen = index+len;


    int ret = 0;

    if(usock) {
        ret = send(usock,cmd,sendlen,0);
        cout<<"send: "<<ret<<endl;
    }
}


unsigned char  process_net(int sock, unsigned char userOP, unsigned char * data,int len)
{
    int i;
    printf("userOP=%02x\n", userOP);
//  for(i=0;i<len;i++) {
//      printf("%02x ", data[i]);
//  }printf("\n");

    //printf("sock=%d\n", sock);

    unsigned char ssid[] = {"knife-sci macbookpro WPAPSK 100"};

    string info;

    switch (userOP) {
    case 0x26:
        Uartcmd_Notify(sock, userOP, (unsigned char *)ssid, strlen((const char*)ssid));
        break;
    case 0x14: // app get device info
        mk_json(info);
        Uartcmd_Notify(sock, userOP, (unsigned char *)info.c_str(),  info.size());
        break;
    case 0x30: //get code
//      printf("data: %s, %d\n", data, len);
        parse_json(data, len);
        break;
    }

    return 0;
}


unsigned char handlerUserOP(int sock, unsigned char opCode,unsigned char * data,int len)
{
    int type = 0;

    if(opCode>UART_COMMAD && opCode<NET_COMMAND)
    {
        type = UART_COMMAD;
    }
    else if(opCode> NET_COMMAND && opCode< SYS_COMMNAD)
    {
        type = NET_COMMAND;
    }
    else if(opCode> SYS_COMMNAD && opCode< USER_COMMAND)
    {
        type = SYS_COMMNAD;
    }
    else if(opCode> USER_COMMAND && opCode< MUL_COMMAND)
    {
        type = USER_COMMAND;
    }
    else if(opCode> MUL_COMMAND && opCode< EQ_COMMAND)
    {
        type = MUL_COMMAND;
    }
    else if(opCode> EQ_COMMAND && opCode< MAX_COMMON_ID)
    {
        type = EQ_COMMAND;
    }

    printf("-->%s op=%02x  type=%02x\n", __FUNCTION__, opCode, type);

    process_net(sock, opCode, data, len);

    return 1;
}

int FindFirstTag(unsigned char * readbuf,int len)
{
    int i = 0;
    while(i<(len-1)){
        if((readbuf[i]==0xff) &&(readbuf[i+1]==0x55||readbuf[i+1]==0x56))
            return i;
        else{
            i++;
            continue;
        }
    }
    i = -1;
    return i;
}

int mk_json(string &dest)
{
    dest = picojson::value(picojson::value::object{
        {"PrepareInfo", picojson::value(picojson::value::object{
            {"productID", picojson::value("smartspeakertestdevice")},
            {"deviceSerialNumber", picojson::value("123456")}
        })}
    }).serialize();

    // std::cout<<"-->"<<dest<<std::endl;

    return 0;
}

int parse_json(unsigned char *data, int len)
{
    picojson::value configuration;
    std::string err;
    picojson::parse(configuration, data, data+len, &err);
    if (err.empty()) {

        g_clientId = configuration / "clientId"_jss;
        g_redirectUri = configuration / "redirectUri"_jss;
        g_authCode = configuration / "authCode"_jss;
        g_codeVerifier = configuration / "codeVerifier"_jss;

        g_can_refresh = true;

       // cout<<"-------------------------------------------------------------------"<<endl;
       // cout<<"clientId = "<<g_clientId<<endl;
       // cout<<"redirectUri = "<<g_redirectUri<<endl;
       // cout<<"authCode = "<<g_authCode<<endl;
       // cout<<"codeVerifier = "<<g_codeVerifier<<endl;
       // cout<<"-------------------------------------------------------------------"<<endl;
    }

    return 0;
}


int mk_devinfo_json(string &dest)
{
    string ver(APP_VERSION);
    dest = picojson::value(picojson::value::object{
        {"DeviceInfo", picojson::value(picojson::value::object{
            {"productId", picojson::value("smartspeakertestdevice")},
            {"deviceSerialNumber", picojson::value("123456")},
            {"deviceId", picojson::value("EFFFED")},
            {"name", picojson::value("smartzhu")},
            {"clientId", picojson::value(g_clientId)},
            {"firmwareVersion", picojson::value(ver)}
        })}
    }).serialize();

    // std::cout<<"-->"<<dest<<std::endl;

    return 0;
}
