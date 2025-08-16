#ifndef __INC_REALTIME_MANAGER_H__
#define __INC_REALTIME_MANAGER_H__

#include "../../common/singleton.h"
#include <boost/unordered_map.hpp>
#include <queue>

class CRealtimeManager : public singleton<CRealtimeManager>
{
public:
    enum ERealtimeEventType
    {
        REALTIME_PLAYER_MOVE,
        REALTIME_PLAYER_ATTACK,
        REALTIME_PLAYER_SKILL,
        REALTIME_CHAT_MESSAGE,
        REALTIME_ITEM_DROP,
        REALTIME_MONSTER_SPAWN,
        REALTIME_GUILD_WAR,
        REALTIME_MAX_EVENT
    };

    struct SRealtimeEvent
    {
        ERealtimeEventType type;
        DWORD dwPlayerID;
        DWORD dwTargetID;
        long x, y;
        DWORD dwData1;
        DWORD dwData2;
        std::string strData;
        DWORD dwTimestamp;

        SRealtimeEvent() : type(REALTIME_MAX_EVENT), dwPlayerID(0), dwTargetID(0), 
                          x(0), y(0), dwData1(0), dwData2(0), dwTimestamp(0) {}
    };

private:
    std::queue<SRealtimeEvent> m_eventQueue;
    boost::unordered_map<DWORD, std::vector<DWORD>> m_playerSubscriptions;
    DWORD m_dwLastProcessTime;
    bool m_bEnabled;

public:
    CRealtimeManager();
    virtual ~CRealtimeManager();

    void Initialize();
    void Destroy();
    
    void Enable(bool bEnable) { m_bEnabled = bEnable; }
    bool IsEnabled() const { return m_bEnabled; }

    // Event management
    void AddEvent(ERealtimeEventType type, DWORD dwPlayerID, DWORD dwTargetID = 0, 
                  long x = 0, long y = 0, DWORD dwData1 = 0, DWORD dwData2 = 0, 
                  const std::string& strData = "");
    
    void ProcessEvents();
    void BroadcastEvent(const SRealtimeEvent& event);
    
    // Subscription management
    void SubscribePlayer(DWORD dwPlayerID, DWORD dwTargetID);
    void UnsubscribePlayer(DWORD dwPlayerID, DWORD dwTargetID);
    void ClearSubscriptions(DWORD dwPlayerID);
    
    // Optimized broadcasting
    void BroadcastToNearby(LPCHARACTER ch, const SRealtimeEvent& event);
    void BroadcastToGuild(DWORD dwGuildID, const SRealtimeEvent& event);
    void BroadcastToEmpire(BYTE bEmpire, const SRealtimeEvent& event);
    
    // Performance monitoring
    DWORD GetEventQueueSize() const { return m_eventQueue.size(); }
    DWORD GetLastProcessTime() const { return m_dwLastProcessTime; }
};

#endif