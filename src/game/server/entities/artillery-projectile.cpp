/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/vmath.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "artillery-projectile.h"

CArtilleryProjectile::CArtilleryProjectile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Span, int Type, float ExplodeHeight, int Dmg)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_ARTILLERY_PROJECTILE)
{
	m_Pos = Pos;
	m_ActualPos = Pos;
	m_ActualDir = Dir;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Type = Type;
	m_ExplodeHeight = ExplodeHeight;
	m_AirStrikeLeft = 0;
	m_AirStrikeTotal = 0;
	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CArtilleryProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CArtilleryProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = 0;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed * 2;
			break;

		case WEAPON_RIFLE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed * 2;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CArtilleryProjectile::TickPaused()
{
	m_StartTick++;
}

void CArtilleryProjectile::Tick()
{
	if(m_AirStrikeTotal)
	{
		if(Server()->Tick() >= m_DoAirStrikeTick)
		{
			if(m_AirStrikeLeft % 2)
			{
				m_AirStrikeLeft--;
				new CArtilleryProjectile(GameWorld(), m_Owner, m_ActualPos + vec2((rand() % 48) - 23.5f, -640.f), vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f, WEAPON_GRENADE, m_ActualPos.y - 32.f);
			}
			else
			{
				m_AirStrikeLeft -= 2;
				new CArtilleryProjectile(GameWorld(), m_Owner,
						m_ActualPos + vec2(-((rand() % 48) + 24.5f) * ((m_AirStrikeTotal - m_AirStrikeLeft - 1) % g_Config.m_InfAirStrikeRange), -640.f),
						vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f, WEAPON_GRENADE, m_ActualPos.y - 32.f);
				new CArtilleryProjectile(GameWorld(), m_Owner,
						m_ActualPos + vec2(((rand() % 48) + 24.5f) * ((m_AirStrikeTotal - m_AirStrikeLeft - 1) % g_Config.m_InfAirStrikeRange), -640.f),
						vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f, WEAPON_GRENADE, m_ActualPos.y - 32.f);
			}
			m_DoAirStrikeTick = Server()->Tick() + g_Config.m_InfAirStrikeDeley / (1000.f / Server()->TickSpeed());
		}
		if(m_AirStrikeLeft)
			return;
		else
			GameServer()->m_World.DestroyEntity(this);
	}
	
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	m_ActualPos = CurPos;
	m_ActualDir = normalize(CurPos - PrevPos);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, nullptr);
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, pOwnerChar);

	m_LifeSpan--;

	if(TargetChr || ((m_ActualPos.y > m_ExplodeHeight || m_ExplodeHeight == 0.f) && Collide) || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_Type == WEAPON_GRENADE)
		{
			GameServer()->CreateExplosion(CurPos, m_Owner, WEAPON_GRENADE, false);
			GameServer()->CreateSound(CurPos, SOUND_GRENADE_EXPLODE);
			GameServer()->m_World.DestroyEntity(this);
		}
		else if(m_Type == WEAPON_RIFLE && pOwnerChar)
		{
			if(pOwnerChar->m_HasAirStrike)
			{
				pOwnerChar->m_HasAirStrike = false;
				pOwnerChar->m_HasIndicator = false;
				pOwnerChar->GetPlayer()->ResetNumberKills();
				m_AirStrikeTotal = g_Config.m_InfAirStrikeNumSuper;
				m_AirStrikeLeft = g_Config.m_InfAirStrikeNumSuper;
			}
			else
			{
				m_AirStrikeTotal = g_Config.m_InfAirStrikeNum;
				m_AirStrikeLeft = g_Config.m_InfAirStrikeNum;
			}
			m_DoAirStrikeTick = Server()->Tick() + (1000.f / Server()->TickSpeed()) / g_Config.m_InfAirStrikeDeley;
		}
	}
}

void CArtilleryProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_ActualPos.x;
	pProj->m_Y = (int)m_ActualPos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = Server()->Tick();
	pProj->m_Type = m_Type;
}

void CArtilleryProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	
	if(IsDontSnapEntity(SnappingClient, GetPos(Ct)))
		return;
	
	if(m_Type == WEAPON_GRENADE)
	{
		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
		if(pProj)
			FillInfo(pProj);
	}
	else if(m_Type == WEAPON_RIFLE)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = (int)m_ActualPos.x;
		pObj->m_Y = (int)m_ActualPos.y;
		pObj->m_FromX = (int)m_ActualPos.x;
		pObj->m_FromY = (int)m_ActualPos.y;
		pObj->m_StartTick = Server()->Tick();
	}
}
