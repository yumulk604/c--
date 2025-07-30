#include "stdafx.h"
#include "kingdom.h"
#include "db.h"

CKingdom::CKingdom(DWORD id, DWORD owner_id, const char* name, long map_index, long x, long y)
    : m_id(id), m_owner_id(owner_id), m_map_index(map_index), m_x(x), m_y(y)
{
    strlcpy(m_name, name, sizeof(m_name));
    m_last_tax_date = time(0);
}

CKingdom::~CKingdom()
{
}

void CKingdom::SetName(const char* name)
{
    strlcpy(m_name, name, sizeof(m_name));
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdoms SET name = '%s' WHERE id = %u", m_name, m_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::SetMapIndex(long map_index)
{
    m_map_index = map_index;
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdoms SET map_index = %ld WHERE id = %u", m_map_index, m_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::SetX(long x)
{
    m_x = x;
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdoms SET x = %ld WHERE id = %u", m_x, m_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::SetY(long y)
{
    m_y = y;
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdoms SET y = %ld WHERE id = %u", m_y, m_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::SetLastTaxDate(time_t date)
{
    m_last_tax_date = date;
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdoms SET last_tax_date = FROM_UNIXTIME(%ld) WHERE id = %u", m_last_tax_date, m_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::AddNPC(DWORD npc_vnum, long x, long y)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "INSERT INTO kingdom_npcs (kingdom_id, npc_vnum, x, y) VALUES(%u, %u, %ld, %ld)", m_id, npc_vnum, x, y);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::RemoveNPC(DWORD npc_id)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "DELETE FROM kingdom_npcs WHERE id = %u", npc_id);
    CDBManager::instance().AsyncQuery(szQuery);
}

void CKingdom::UpdateNPC(DWORD npc_id, long x, long y)
{
    char szQuery[1024];
    snprintf(szQuery, sizeof(szQuery), "UPDATE kingdom_npcs SET x = %ld, y = %ld WHERE id = %u", x, y, npc_id);
    CDBManager::instance().AsyncQuery(szQuery);
}
