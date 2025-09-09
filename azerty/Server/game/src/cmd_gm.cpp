#include "log.h"
#include "horsename_manager.h"
#include "MarkManager.h"
#include "polymorph.h"

#define ENABLE_CHAT_LOGGING

	}
}


else if (!strcmp(arg1, "item_id_range"))
{
	ch->ChatPacket(CHAT_TYPE_INFO, "ItemIDRange: %u ~ %u", ITEM_MANAGER::instance().GetItemID().dwMin, ITEM_MANAGER::instance().GetItemID().dwMax);