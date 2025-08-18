/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_ARTILLERYLASER_H
#define GAME_SERVER_ENTITIES_ARTILLERYLASER_H

#include <game/server/entity.h>

class CArtilleryLaser : public CEntity
{
public:
	CArtilleryLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Dmg, int AirStrike = 2, int AirStrikeDeley = 200);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	
protected:
	bool HitCharacter(vec2 From, vec2 To) { return false; };
	void DoBounce();

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
	int m_Dmg;
	int m_AirStrike;
	int m_AirStrikeTotal;
	int m_AirStrikeDeley;
};

#endif
