#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "QID.h"
#include "DBManager.h"

void CClientManager::CreateKingdom(CPeer* peer, DWORD dwHandle, const char* name, DWORD owner_id)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "INSERT INTO kingdoms (owner_id, name, map_index, x, y, last_tax_date) VALUES(%u, '%s', 0, 0, 0, NOW())", owner_id, name);
    std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
    SQLResult* pRes = pMsg->Get();

    if (pRes->uiAffectedRows > 0)
    {
        DWORD kingdom_id = pRes->uiInsertID;
        snprintf(szQuery, sizeof(szQuery), "UPDATE player SET kingdom_id = %u WHERE id = %u", kingdom_id, owner_id);
        CDBManager::instance().AsyncQuery(szQuery);
        // TODO: Send success packet to game server
    }
    else
    {
        // TODO: Send failure packet to game server
    }
}

void CClientManager::GetKingdom(CPeer* peer, DWORD dwHandle, DWORD kingdom_id)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "SELECT id, owner_id, name, map_index, x, y, last_tax_date FROM kingdoms WHERE id = %u", kingdom_id);
    CDBManager::instance().ReturnQuery(szQuery, QID_KINGDOM_GET, peer->GetHandle(), new ClientHandleInfo(dwHandle, kingdom_id));
}

void CClientManager::AddKingdomNPC(CPeer* peer, DWORD dwHandle, DWORD kingdom_id, DWORD npc_vnum, int x, int y)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "INSERT INTO kingdom_npcs (kingdom_id, npc_vnum, x, y) VALUES(%u, %u, %d, %d)", kingdom_id, npc_vnum, x, y);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CClientManager::GetKingdomNPCs(CPeer* peer, DWORD dwHandle, DWORD kingdom_id)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "SELECT id, kingdom_id, npc_vnum, x, y FROM kingdom_npcs WHERE kingdom_id = %u", kingdom_id);
    CDBManager::instance().ReturnQuery(szQuery, QID_KINGDOM_NPCS_GET, peer->GetHandle(), new ClientHandleInfo(dwHandle, kingdom_id));
}
