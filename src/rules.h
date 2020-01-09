// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2019 Litecoin Plus
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _LITECOINPLUS_RULES_H_
#define _LITECOINPLUS_RULES_H_ 1

#include <set>
#include <string>
#include <map>

#include "uint256.h"
#include "util.h"
#include "sync.h"
#include "alert.h"

class CRules;
extern CCriticalSection cs_mapRules;
extern std::map<int, CRules> mapRules;

// the PALADIN system class definition
class CRules
{
public:

	// saving stuff for the rules/alert (encrypted only)
    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

	// variables
	int alertId;						// link to alert ID
	int nVersion;						// version of rule packet
	int nID;							// rule ID
	int nMinVer;						// lowest version inclusive
	int nMaxVer;						// highest version inclusive
	int fromHeight;						// starting from block height
	int toHeight;						// ending at block height
	int ruleType;						// just an enum of rule types
	int ruleValue;						// the value for this rule
    enum ruleTypes
    {
        RULE_POW_ON_OFF = 1,			// PoW ON/OFF flag
        RULE_CLOCK_DRIFT = 2,			// adjust nMaxClockDrift value dynamically
        RULE_POS_PERCENT = 3,			// adjust PoS % reward
		RULE_POW_REWARD = 4,			// adjust PoW absolute reward
		RULE_BLOCK_TARGET = 5,			// adjust the block target, in s
		RULE_DISABLE_OLD_CLIENTS = 6,	// only accept clients >= 5.0.0.1
		RULE_SUSPEND_SENDING = 7,		// refuse to send coins, globally
        RULE_POS_ON_OFF = 8,			// PoS ON/OFF flag
    };

	// creator
    CRules()
    {
        SetNull();
    }

    void SetNull();
    std::string ToString() const;
    void print() const;

	// static methods

	// parse a single rule, template: must be declared in the .h file
	template<typename T1>
	static bool parseRules(int nHeight, ruleTypes rule, T1* var, T1 defaultValue)
	{
		CRules* r;

		r = NULL;
		r = CRules::getRule(nHeight, rule);
		if (r)
		{
			*var = r->ruleValue;
		}
		else	// default behavior when no rule is active
		{
			*var = defaultValue;
		}

		return r != NULL;
	}

	static bool insert(const CAlert& alert);
	static CRules* getRule(int nHeight, ruleTypes rType);
	static void parseGenericRules(int nHeight);
	static int getCurrentRulesHeight();
};

class CDiskRules
{
public:

	// saving stuff for the rules/alert (encrypted only)
    std::vector<CRules> vRules;

    IMPLEMENT_SERIALIZE
    (
	// different behavior for reading/writing
        if (fWrite)
        {
		    int nUBuckets = vRules.size();
		    READWRITE(nUBuckets);
			BOOST_FOREACH(CRules rule, vRules)
			{
				READWRITE(rule.vchMsg);
				READWRITE(rule.vchSig);
			}

		}
		else
		{
		    int nUBuckets = 0;
		    READWRITE(nUBuckets);
			for (int i = 0; i < nUBuckets; i++)
			{
				CAlert alert;
				READWRITE(alert.vchMsg);
				READWRITE(alert.vchSig);

			// process as if it was received by a node
		        if (alert.ProcessAlert())
				{
					{
						LOCK(cs_vNodes);
						BOOST_FOREACH(CNode* pnode, vNodes)
							alert.RelayTo(pnode);
					}
				}
			}
		}
    )

	// creator
    CDiskRules()
    {
        SetNull();
    }

    void SetNull();

	void Fill();
};

#endif
