/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_ARTILLERYGRENADE_H
#define GAME_SERVER_ENTITIES_ARTILLERYGRENADE_H

class CArtilleryProjectile : public CEntity
{
public:
	int m_Owner;
	
public:
	CArtilleryProjectile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Span, int Type, float ExplodeHeight = 0.f, int Dmg = 6);

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	vec2 m_ActualPos;

private:
	vec2 m_ActualDir;
	vec2 m_Direction;
	int m_StartTick;
	int m_Type;
	int m_LifeSpan;
	float m_ExplodeHeight;
	int m_AirStrikeLeft;
	int m_AirStrikeTotal;
	int m_DoAirStrikeTick;
};

#endif
