#ifndef __INC_REALTIME_PACKETS_H__
#define __INC_REALTIME_PACKETS_H__

// Client to Game realtime packets
enum
{
    HEADER_CG_REALTIME_SUBSCRIBE = 200,
    HEADER_CG_REALTIME_UNSUBSCRIBE = 201,
};

// Game to Client realtime packets  
enum
{
    HEADER_GC_REALTIME = 200,
    HEADER_GC_REALTIME_BULK = 201,
};

typedef struct SPacketCGRealtimeSubscribe
{
    BYTE bHeader;
    DWORD dwTargetID;
    BYTE bEventType;
} TPacketCGRealtimeSubscribe;

typedef struct SPacketCGRealtimeUnsubscribe
{
    BYTE bHeader;
    DWORD dwTargetID;
    BYTE bEventType;
} TPacketCGRealtimeUnsubscribe;

typedef struct SPacketGCRealtime
{
    BYTE bHeader;
    BYTE bType;
    DWORD dwPlayerID;
    DWORD dwTargetID;
    long lX;
    long lY;
    DWORD dwData1;
    DWORD dwData2;
    DWORD dwTimestamp;
} TPacketGCRealtime;

typedef struct SPacketGCRealtimeBulk
{
    BYTE bHeader;
    WORD wSize;
    BYTE bCount;
    // Followed by bCount number of TPacketGCRealtime
} TPacketGCRealtimeBulk;

#endif