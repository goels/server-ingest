
#ifndef TRUE
#define TRUE            1
#define FALSE           0
#endif

#define SYNC_BYTE       0x47
#define MAX_TS_PKT_SIZE 208
#define SECT_LEN_STATES 3
#define PAT_SECT_STATES 5
#define PMT_SECT_STATES 9
#define PMT_TABLE_ID    2


struct stream
{
    unsigned char pat[1024];
    unsigned char pmt[1024];
    unsigned int firstPAT;
    unsigned int firstPMT;
    unsigned int tsPktSize; 
    unsigned int pos;
    unsigned int bufLen;
    unsigned char* buffer;
    unsigned short offset;
    unsigned short program;
    unsigned short patPID;
    unsigned short pmtPID;
    unsigned short videoPID;
    unsigned short audioPID;
    unsigned char type;
};

struct packet
{
    unsigned int length;
    unsigned int headerState;
    unsigned int pointer;
    unsigned short pid;
    unsigned char pusi;
};

struct section
{
    unsigned int lengthState;
    unsigned int state;
    unsigned int read;
    unsigned int start;
    unsigned short len;
    unsigned short offset;
    unsigned char tid;
    unsigned char number;
    unsigned char lastNumber;
};


void decode_mp2ts(struct stream* strm, unsigned char **pat, unsigned char **pmt);

void discoverPktSize(struct stream* strm);

