#ifndef _MW_ASR_H_
#define _MW_ASR_H_


typedef enum{
	MEDIAWIN = 0,
	IFLY,
	THF
}ASR_ENGINE;

enum{
	WAKEN = 0,
	VAD_BEGIN,
	VAD_END
};

enum 
{
	CHL_SPEECH = 0,          //基本语音，Synchorinze directive
	CHL_PLAY,                //音乐，Play Directive
	CHL_ALERT,               //定时器或报警器
	CHL_COUNT
};

extern void* pUser;

typedef struct {
	int (*asr_create)();
	int (*asr_process)(short*,int);
	void (*asr_destroy)();
}ASR_OPS;

typedef void (*audioStatus_Changed)(void* userp,int type);
typedef void (*audioData_Callback)(void* userp,int seq,unsigned short* pData,unsigned int length);

int mw_createWakeuper(ASR_ENGINE engine);

void mw_setUserInfo(void* userInfo);

void mw_playAudio(unsigned int channel,const char* file);

void mw_startWake(audioData_Callback cb,audioStatus_Changed status_cb);

void mw_stopWake();

void mw_enableWake(int enable);

void mw_startRecord();
void mw_stopRecord();

void mw_play_lock();
void mw_pcm_play(unsigned int channel,char* pcm_buf,int pcm_len,unsigned int sample_rate,int channel_num);
void mw_play_unlock();

void mw_destroyWakeuper();

#endif
