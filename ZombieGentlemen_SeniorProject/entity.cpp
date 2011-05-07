#include "entity.h"

// constructor destructor
entity::entity()
{
	m_object = NULL;
}
entity::~entity()
{
	// to destroy objects, tell the object manager to pop this object
}

// member functions
void entity::animate()
{
	// this function needs more design before it is implimented
}

// mutators
void entity::setObject(object * a_object){m_object = a_object;}
void entity::entityDead(){alive = false;}
void entity::entityAlive(){alive = true;}

//accessors
object * entity::getObject(){return m_object;}
bool entity::isAlive(){return alive;}
bool entity::hasArmor(){return armor;}