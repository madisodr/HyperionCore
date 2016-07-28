/* HyperionCore */
#include "ScriptMgr.h"
#include <vector>

using namespace std;

class Talent {
	public:
		enum PerkType {
			ITEM,
			SPELL,
			OBJECT,
			SUMMON,

			 
		}
		Talent(uin32 id, uint32 perk, PerkType t) {
			this->m_guid = id;
			this->m_perk = perk;
			this->m_type = t;
		}

		~Talent() {} // empty destructorrrrrrrr

	private:
		uint32 m_guid;
		uint32 m_perk;
		PerkType m_type;
};

class TalentHandler() {
	public:
		TalentHandler() {
			QueryResult result = WorldDatabase.PQuery("SELECT * FROM apollo_talents");
			if(!result) 
				return false;

			Field* field;

			do {
				field = result->Fetch();
				Talent* tmp = new Talent(field[0].GetUint32(), field[1].GetUint32(), field[2].GetUint32());
				m_talents.push_back(tmp);
			} while(result->NextRow());
		}

		~TalentHandler() {
			for(int i = 0; i < m_talents.size(); i++) { 
				delete m_talents[i];
			}
		}
	private:
		vector<Talent*> m_talents;
};



