#include "log.h"
#include "horsename_manager.h"
#include "MarkManager.h"
#include "polymorph.h"

#define ENABLE_CHAT_LOGGING

	}
}

// Polymorph drop commands
else if (!strcmp(arg1, "polymorph_drop"))
{
	if (!*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: polymorph_drop <on|off|percent>");
		return;
	}

	if (!strcmp(arg2, "on"))
	{
		quest::CQuestManager::instance().RequestSetEventFlag("polymorph_drop", 100);
		ch->ChatPacket(CHAT_TYPE_INFO, "Polymorph drop enabled (100%% rate)");
	}
	else if (!strcmp(arg2, "off"))
	{
		quest::CQuestManager::instance().RequestSetEventFlag("polymorph_drop", 0);
		ch->ChatPacket(CHAT_TYPE_INFO, "Polymorph drop disabled");
	}
	else if (!strcmp(arg2, "percent"))
	{
		char arg3[256];
		one_argument(argument, arg3, sizeof(arg3));
		one_argument(arg3, arg3, sizeof(arg3));
		one_argument(arg3, arg3, sizeof(arg3));
		
		if (*arg3)
		{
			int percent = atoi(arg3);
			quest::CQuestManager::instance().RequestSetEventFlag("polymorph_drop", percent);
			ch->ChatPacket(CHAT_TYPE_INFO, "Polymorph drop rate set to %d%%", percent);
		}
		else
		{
			int current = quest::CQuestManager::instance().GetEventFlag("polymorph_drop");
			ch->ChatPacket(CHAT_TYPE_INFO, "Current polymorph drop rate: %d%%", current);
		}
	}
}

else if (!strcmp(arg1, "item_id_range"))
{
	ch->ChatPacket(CHAT_TYPE_INFO, "ItemIDRange: %u ~ %u", ITEM_MANAGER::instance().GetItemID().dwMin, ITEM_MANAGER::instance().GetItemID().dwMax);