#include "Base_include.h"
#include "CBaseEntity.h"

//#include <typeinfo>

CBaseEntity::CBaseEntity()
{
	xPos = 0;
	yPos = 0;
#ifdef _3D
	zPos = 0;
#endif
}

CBaseEntity::CBaseEntity(double x, double y)
{
	xPos = x;
	yPos = y;
}

CBaseEntity::~CBaseEntity() 
{

}

#ifdef _3D
CBaseEntity::CBaseEntity(double x, double y, double z)
{
	xPos = x;
	yPos = y;
	zPos = z;
}
#endif

bool CBaseEntity::operator != (CBaseEntity another)
{
	if(this != &another)
	{
		return false;
	}

	return true;
}

void CBaseEntity::Update()
{

}