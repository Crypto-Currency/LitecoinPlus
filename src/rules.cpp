// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2019 Litecoin Plus
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Rules system
//

#include <boost/foreach.hpp>
#include <map>

#include <openssl/ec.h> // for EC_KEY definition

#include "db.h"
#include "key.h"
#include "net.h"
#include "sync.h"
#include "ui_interface.h"
#include "rules.h"
#include "alert.h"
#include "wallet.h"

using namespace std;

std::map<int, CRules> mapRules;
CCriticalSection cs_mapRules;

void CRules::SetNull()
{
    vchMsg.clear();
    vchSig.clear();
	alertId    = 0;
	nVersion   = 1;
	nID        = 0;
	nMinVer    = 0;
	nMaxVer    = 0;
	fromHeight = 0;
	toHeight   = 0;
	ruleType   = 0;
	ruleValue  = 0;
}

std::string CRules::ToString() const
{
    return strprintf(
        "CRules(\n"
        "    alertId      = %d\n"
        "    nVersion     = %d\n"
        "    nID          = %d\n"
        "    nMinVer      = %d\n"
        "    nMaxVer      = %d\n"
        "    fromHeight   = %d\n"
        "    toHeight     = %d\n"
        "    ruleType     = %d\n"
        "    ruleValue    = %d\n"
        ")\n",
		alertId,        
		nVersion,
        nID,
        nMinVer,
        nMaxVer,
        fromHeight,
        toHeight,
		ruleType,
		ruleValue);
}

void CRules::print() const
{
    printf("%s", ToString().c_str());
}

bool CRules::insert(const CAlert& alert)
{
    {
        LOCK(cs_mapRules);

	// first let's decode the rule
		CRules rule;

		rule.vchMsg = alert.vchMsg;
		rule.vchSig = alert.vchSig;
		rule.alertId = alert.nID;
		if (sscanf(alert.strStatusBar.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d",
			&rule.nVersion, &rule.nID, &rule.nMinVer, &rule.nMaxVer, &rule.fromHeight, &rule.toHeight, &rule.ruleType, &rule.ruleValue) < 8)
		{
			return false;		// decode error
		}

	// it's a insert, all the rest is ignored
        map<int, CRules>::iterator mi = mapRules.find(rule.nID);
        if (mi == mapRules.end())
		{
			mapRules.insert(make_pair(rule.nID, rule));

		// dump everything to disk
			CDiskRules rules;
			rules.Fill();
			CRulesDB rdb;
			rdb.Write(rules);

		// update rule height
			nPaladinRuleHeight = mapRules.size();
		}
    }
	return true;
}

int CRules::getCurrentRulesHeight()
{
    {
        LOCK(cs_mapRules);
		return mapRules.size();
	}
}

CRules* CRules::getRule(int nHeight, ruleTypes rType)
{
	CRules *retRule = NULL;

    {
        LOCK(cs_mapRules);

		// find the rule that matches (the greatest one, actually we can have multiple, always return the biggest one)
		for (map<int, CRules>::iterator mi = mapRules.begin(); mi != mapRules.end(); mi++)
		{
			CRules* rule = &(*mi).second;

			if (((rType == rule->ruleType) && (nHeight >= rule->fromHeight) && (nHeight <= rule->toHeight)) ||			// range
				((rType == rule->ruleType) && (nHeight >= rule->fromHeight) && (rule->toHeight == 0)))					// unlimited scope rule (can still be override by a higher ID one)
			{
				retRule = rule;
			}
		}

		return retRule;
	}
}

// generic rule parser, can be placed as soon as a block is received
// "generic rules" for PALADIN are, by definition, rules that "DO NOT DIRECTLY AFFECT THE ACCEPTANCE OF A BLOCK"
// other rules are parsed in specific parts of the code and threads around, to match and be in sync perfectly
void CRules::parseGenericRules(int nHeight)
{
	CRules rule;

// only accept clients >= 5.0.0.1 (false by default)
	rule.parseRules(nHeight, RULE_DISABLE_OLD_CLIENTS, &nPaladinOnlyClients, false);

// refuse to send coins, globally (false by default)
	rule.parseRules(nHeight, RULE_SUSPEND_SENDING, &nSendSuspended, false);
}

void CDiskRules::SetNull()
{
	vRules.clear();
}

void CDiskRules::Fill()
{
	// find the rule that matches (the greatest one, actually we can have multiple, always return the biggest one)
	for (map<int, CRules>::iterator mi = mapRules.begin(); mi != mapRules.end(); mi++)
	{
		vRules.push_back((*mi).second);
	}
}

