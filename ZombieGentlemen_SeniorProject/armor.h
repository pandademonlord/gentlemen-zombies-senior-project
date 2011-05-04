#pragma once

//Armor class, handles Player armor

class Armor
{
private:
	int armorType;
	float armorHealth;
	float damageResistance;
public:
	Armor();
	bool initPlayerArmor(int a_armorType);
	~Armor();
	void damageArmor(int damage);
	bool armorDepleted(void);
	float getArmorHealth();
	float getDamageResistance();
	void setArmorHealth(float a_armorHealth);
	void setArmorType(int a_armorType);
	int getArmorType();
	void setDamageResistance(float a_damageResistance);
	void armorHealthRestore();
};