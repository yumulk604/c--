#include "stdafx.h"
#include "realtime_manager.h"
#include "char.h"
#include "char_manager.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "guild.h"
#include "guild_manager.h"
#include "sectree_manager.h"

CRealtimeManager::CRealtimeManager() : m_dwLastProcessTime(0), m_bEnabled(true)
{
}

CRealtimeManager::~CRealtimeManager()
{
    Destroy();
}

void CRealtimeManager::Initialize()
{
    m_eventQueue = std::queue<SRealtimeEvent>();
    m_playerSubscriptions.clear();
    m_dwLastProcessTime = get_dword_time();
    m_bEnabled = true;
    
    sys_log(0, "RealtimeManager: Initialized");
}

void CRealtimeManager::Destroy()
{
    while (!m_eventQueue.empty())
        m_eventQueue.pop();
    
    m_playerSubscriptions.clear();
    m_bEnabled = false;
    
    sys_log(0, "RealtimeManager: Destroyed");
}

void CRealtimeManager::AddEvent(ERealtimeEventType type, DWORD dwPlayerID, DWORD dwTargetID,
                                long x, long y, DWORD dwData1, DWORD dwData2, 
                                const std::string& strData)
{
    if (!m_bEnabled)
        return;

    SRealtimeEvent event;
    event.type = type;
    event.dwPlayerID = dwPlayerID;
    event.dwTargetID = dwTargetID;
    event.x = x;
    event.y = y;
    event.dwData1 = dwData1;
    event.dwData2 = dwData2;
    event.strData = strData;
    event.dwTimestamp = get_dword_time();

    m_eventQueue.push(event);
    
    // Immediate processing for critical events
    if (type == REALTIME_PLAYER_ATTACK || type == REALTIME_PLAYER_SKILL)
    {
        ProcessEvents();
    }
}

void CRealtimeManager::ProcessEvents()
{
    if (!m_bEnabled)
        return;

    DWORD dwCurrentTime = get_dword_time();
    int iProcessedEvents = 0;
    const int MAX_EVENTS_PER_CYCLE = 100; // Prevent lag

    while (!m_eventQueue.empty() && iProcessedEvents < MAX_EVENTS_PER_CYCLE)
    {
        SRealtimeEvent event = m_eventQueue.front();
        m_eventQueue.pop();
        
        // Skip old events (older than 1 second)
        if (dwCurrentTime - event.dwTimestamp > 1000)
        {
            iProcessedEvents++;
            continue;
        }

        BroadcastEvent(event);
        iProcessedEvents++;
    }

    m_dwLastProcessTime = dwCurrentTime;
}

void CRealtimeManager::BroadcastEvent(const SRealtimeEvent& event)
{
    LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(event.dwPlayerID);
    if (!ch)
        return;

    switch (event.type)
    {
        case REALTIME_PLAYER_MOVE:
            BroadcastToNearby(ch, event);
            break;
            
        case REALTIME_PLAYER_ATTACK:
        case REALTIME_PLAYER_SKILL:
            BroadcastToNearby(ch, event);
            break;
            
        case REALTIME_CHAT_MESSAGE:
            BroadcastToNearby(ch, event);
            break;
            
        case REALTIME_GUILD_WAR:
            if (ch->GetGuild())
                BroadcastToGuild(ch->GetGuild()->GetID(), event);
            break;
            
        case REALTIME_ITEM_DROP:
        case REALTIME_MONSTER_SPAWN:
            BroadcastToNearby(ch, event);
            break;
            
        default:
            break;
    }
}

void CRealtimeManager::BroadcastToNearby(LPCHARACTER ch, const SRealtimeEvent& event)
{
    if (!ch || !ch->GetSectree())
        return;

    // Create realtime packet
    TPacketGCRealtime packet;
    packet.bHeader = HEADER_GC_REALTIME;
    packet.bType = event.type;
    packet.dwPlayerID = event.dwPlayerID;
    packet.dwTargetID = event.dwTargetID;
    packet.lX = event.x;
    packet.lY = event.y;
    packet.dwData1 = event.dwData1;
    packet.dwData2 = event.dwData2;
    packet.dwTimestamp = event.dwTimestamp;
    
    // Broadcast to nearby players
    ch->PacketAround(&packet, sizeof(TPacketGCRealtime));
}

void CRealtimeManager::BroadcastToGuild(DWORD dwGuildID, const SRealtimeEvent& event)
{
    CGuild* pGuild = CGuildManager::instance().FindGuild(dwGuildID);
    if (!pGuild)
        return;

    TPacketGCRealtime packet;
    packet.bHeader = HEADER_GC_REALTIME;
    packet.bType = event.type;
    packet.dwPlayerID = event.dwPlayerID;
    packet.dwTargetID = event.dwTargetID;
    packet.lX = event.x;
    packet.lY = event.y;
    packet.dwData1 = event.dwData1;
    packet.dwData2 = event.dwData2;
    packet.dwTimestamp = event.dwTimestamp;

    // Send to all guild members
    pGuild->ForEachOnlineMember([&packet](LPCHARACTER ch) {
        if (ch && ch->GetDesc())
            ch->GetDesc()->Packet(&packet, sizeof(TPacketGCRealtime));
    });
}

void CRealtimeManager::BroadcastToEmpire(BYTE bEmpire, const SRealtimeEvent& event)
{
    TPacketGCRealtime packet;
    packet.bHeader = HEADER_GC_REALTIME;
    packet.bType = event.type;
    packet.dwPlayerID = event.dwPlayerID;
    packet.dwTargetID = event.dwTargetID;
    packet.lX = event.x;
    packet.lY = event.y;
    packet.dwData1 = event.dwData1;
    packet.dwData2 = event.dwData2;
    packet.dwTimestamp = event.dwTimestamp;

    const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
    for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
    {
        LPDESC d = *it;
        if (d->GetCharacter() && d->GetCharacter()->GetEmpire() == bEmpire)
            d->Packet(&packet, sizeof(TPacketGCRealtime));
    }
}

void CRealtimeManager::SubscribePlayer(DWORD dwPlayerID, DWORD dwTargetID)
{
    m_playerSubscriptions[dwPlayerID].push_back(dwTargetID);
}

void CRealtimeManager::UnsubscribePlayer(DWORD dwPlayerID, DWORD dwTargetID)
{
    auto it = m_playerSubscriptions.find(dwPlayerID);
    if (it != m_playerSubscriptions.end())
    {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), dwTargetID), vec.end());
    }
}

void CRealtimeManager::ClearSubscriptions(DWORD dwPlayerID)
{
    m_playerSubscriptions.erase(dwPlayerID);
}