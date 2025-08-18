/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/server/roundstatistics.h>

#include "artillery-laser.h"
#include "artillery-projectile.h"

CArtilleryLaser::CArtilleryLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Dmg, int AirStrike, int AirStrikeDeley)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_ARTILLERY_LASER)
{
	m_Dmg = Dmg;
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_AirStrike = AirStrike;
	m_AirStrikeTotal = AirStrike;
	GameWorld()->InsertEntity(this);
	DoBounce();
}

void CArtilleryLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0 && m_AirStrike <= 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir * m_Energy;

	if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);

			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
			m_Bounces++;

			if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
				m_Energy = -1;

			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}

	//CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(m_AirStrike <= 0)
	{
		
		return;
	}
	else if(m_AirStrike % 2)
	{
		m_AirStrike--;
		new CArtilleryProjectile(GameWorld(), m_Owner, m_Pos + vec2(0.f, -640.f), vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f	, WEAPON_GRENADE, m_Pos.y - 96.f);
	}
	else
	{
		m_AirStrike -= 2;
		new CArtilleryProjectile(GameWorld(), m_Owner, m_Pos + vec2(-48.f * ((m_AirStrikeTotal - m_AirStrike - 1) % 3 ), -640.f), vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f, WEAPON_GRENADE, m_Pos.y - 96.f);
		new CArtilleryProjectile(GameWorld(), m_Owner, m_Pos + vec2(48.f * ((m_AirStrikeTotal - m_AirStrike - 1) % 3 ), -640.f), vec2(0.f, 1.f), Server()->TickSpeed() * 0.7f, WEAPON_GRENADE, m_Pos.y - 96.f);
	}
}

void CArtilleryLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CArtilleryLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*200.f)/1000.0f)
		DoBounce();
}

void CArtilleryLaser::TickPaused()
{
	++m_EvalTick;
}

void CArtilleryLaser::Snap(int SnappingClient)
{
	if(IsDontSnapEntity(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
