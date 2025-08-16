#include "log.h"
#include "horsename_manager.h"
#include "MarkManager.h"
#include "realtime_manager.h"
#include "realtime_packets.h"

#define ENABLE_CHAT_LOGGING

		case HEADER_CG_TARGET_INFO_LOAD:
			TargetInfoLoad(ch, c_pData);
			break;

		case HEADER_CG_REALTIME_SUBSCRIBE:
			RealtimeSubscribe(ch, c_pData);
			break;

		case HEADER_CG_REALTIME_UNSUBSCRIBE:
			RealtimeUnsubscribe(ch, c_pData);
			break;
	}
	return (iExtraLen);
}

	}

	ch->ChatPacket(CHAT_TYPE_TALKING, "%s", pinfo->szText);
	
	// Send realtime chat event
	ch->SendRealtimeEvent(CRealtimeManager::REALTIME_CHAT_MESSAGE, 0, 
	                      CHAT_TYPE_TALKING, 0, pinfo->szText);
}

void CInputMain::Whisper(LPCHARACTER ch, const char * data)

	}

	ch->ChatPacket(CHAT_TYPE_WHISPER, "%s : %s", ch->GetName(), pinfo->szText);
	
	// Send realtime whisper event
	ch->SendRealtimeEvent(CRealtimeManager::REALTIME_CHAT_MESSAGE, pkChr->GetPlayerID(), 
	                      CHAT_TYPE_WHISPER, 0, pinfo->szText);
}

void CInputMain::RealtimeSubscribe(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGRealtimeSubscribe* pinfo = (TPacketCGRealtimeSubscribe*) c_pData;
	
	if (!ch)
		return;

	CRealtimeManager::instance().SubscribePlayer(ch->GetPlayerID(), pinfo->dwTargetID);
	sys_log(1, "REALTIME: Player %u subscribed to %u for event type %d", 
	        ch->GetPlayerID(), pinfo->dwTargetID, pinfo->bEventType);
}

void CInputMain::RealtimeUnsubscribe(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGRealtimeUnsubscribe* pinfo = (TPacketCGRealtimeUnsubscribe*) c_pData;
	
	if (!ch)
		return;

	CRealtimeManager::instance().UnsubscribePlayer(ch->GetPlayerID(), pinfo->dwTargetID);
	sys_log(1, "REALTIME: Player %u unsubscribed from %u for event type %d", 
	        ch->GetPlayerID(), pinfo->dwTargetID, pinfo->bEventType);
}