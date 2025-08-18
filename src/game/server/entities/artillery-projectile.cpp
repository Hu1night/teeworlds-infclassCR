/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/vmath.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>

#include "artillery-projectile.h"
#include "artillery-laser.h"

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
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_RIFLE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
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
		}
		else if(m_Type == WEAPON_RIFLE && pOwnerChar)
		{
			new CArtilleryLaser(GameWorld(), CurPos, normalize(CurPos - PrevPos), 0.f, m_Owner, 0,
					pOwnerChar->m_HasAirStrike ? 8 : 3,
					200);
			if(pOwnerChar->m_HasAirStrike)
			{
				pOwnerChar->m_HasAirStrike = false;
				pOwnerChar->m_HasIndicator = false;
				pOwnerChar->GetPlayer()->ResetNumberKills();
			}
		}

		GameServer()->m_World.DestroyEntity(this);
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
