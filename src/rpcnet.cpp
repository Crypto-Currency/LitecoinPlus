// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "net.h"
#include "bitcoinrpc.h"
#include "alert.h"
#include "rules.h"
#include "wallet.h"
#include "db.h"
#include "walletdb.h"

using namespace json_spirit;
using namespace std;

Value getconnectioncount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getconnectioncount\n"
            "Returns the number of connections to other nodes.");

    LOCK(cs_vNodes);
    return (int)vNodes.size();
}

static void CopyNodeStats(std::vector<CNodeStats>& vstats)
{
    vstats.clear();

    LOCK(cs_vNodes);
    vstats.reserve(vNodes.size());
    BOOST_FOREACH(CNode* pnode, vNodes) {
        CNodeStats stats;
        pnode->copyStats(stats);
        vstats.push_back(stats);
    }
}

Value getpeerinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpeerinfo\n"
            "Returns data about each connected network node.");

    vector<CNodeStats> vstats;
    CopyNodeStats(vstats);

    Array ret;

    BOOST_FOREACH(const CNodeStats& stats, vstats) {
        Object obj;

        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%08" PRI64x, (long long unsigned int)stats.nServices)));
        obj.push_back(Pair("lastsend", (boost::int64_t)stats.nLastSend));
        obj.push_back(Pair("lastrecv", (boost::int64_t)stats.nLastRecv));
        obj.push_back(Pair("conntime", (boost::int64_t)stats.nTimeConnected));
        obj.push_back(Pair("version", stats.nVersion));
        obj.push_back(Pair("subver", stats.cleanSubVer));
        obj.push_back(Pair("inbound", stats.fInbound));
        obj.push_back(Pair("releasetime", (boost::int64_t)stats.nReleaseTime));
        obj.push_back(Pair("startingheight", stats.nStartingHeight));
        obj.push_back(Pair("banscore", stats.nMisbehavior));

        ret.push_back(obj);
    }

    return ret;
}
 
// ppcoin: send alert.  
// There is a known deadlock situation with ThreadMessageHandler
// ThreadMessageHandler: holds cs_vSend and acquiring cs_main in SendMessages()
// ThreadRPCServer: holds cs_main and acquiring cs_vSend in alert.RelayTo()/PushMessage()/BeginMessage()
Value sendalert(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 5)
		throw runtime_error(
            "sendalert <message> <privatekey> <minver> <maxver> <priority> <id> [cancelupto]\n"
            "<message> is the alert text message\n"
            "<privatekey> is hex string of alert master private key\n"
            "<minver> is the minimum applicable internal client version\n"
            "<maxver> is the maximum applicable internal client version\n"
            "<priority> is integer priority number\n"
            "[cancelupto] cancels all alert id's up to this number\n"
            "Returns true or false.");

	if (CAlert::isRule(params[4].get_int()))
	{
        throw runtime_error(
            "Priority 999 is reserved for special rule setting packets.\n");
	}

    CAlert alert;
    CKey key;

	// by Simone: where we place the string, depends on the alert type
	if (CAlert::isInfo(params[4].get_int()) || CAlert::isWarning(params[4].get_int()) || CAlert::isCritical(params[4].get_int()))
	{
		alert.strComment = params[0].get_str();
	}
	else
	{
		alert.strStatusBar = params[0].get_str();
	}
    alert.nMinVer = params[2].get_int();
    alert.nMaxVer = params[3].get_int();
    alert.nPriority = params[4].get_int();
    alert.nID = CAlert::getNextID();						// by Simone: auto alert ID
    if (params.size() > 5)
        alert.nCancel = params[5].get_int();
    alert.nVersion = CONTROL_PROTOCOL_VERSION;
    alert.nPermanent = false;								// always false when sending a manual packet
    alert.nRelayUntil = GetAdjustedTime() + 180;
    alert.nExpiration = GetAdjustedTime() + 180;

    CDataStream sMsg(SER_NETWORK, PROTOCOL_VERSION);
    sMsg << (CUnsignedAlert)alert;
    alert.vchMsg = vector<unsigned char>(sMsg.begin(), sMsg.end());

    vector<unsigned char> vchPrivKey = ParseHex(params[1].get_str());
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end())); // if key is not correct openssl may crash
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
        throw runtime_error(
            "Unable to sign alert, check private key?\n");  
    if(!alert.ProcessAlert()) 
        throw runtime_error(
            "Failed to process alert.\n");
    // Relay alert
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            alert.RelayTo(pnode);
    }

    Object result;
	result.push_back(Pair("strComment", alert.strComment));
	result.push_back(Pair("strStatusBar", alert.strStatusBar));
	result.push_back(Pair("strReserved", alert.strReserved));
    result.push_back(Pair("nVersion", alert.nVersion));
    result.push_back(Pair("nMinVer", alert.nMinVer));
    result.push_back(Pair("nMaxVer", alert.nMaxVer));
    result.push_back(Pair("nPriority", alert.nPriority));
    result.push_back(Pair("nID", alert.nID));
    if (alert.nCancel > 0)
        result.push_back(Pair("nCancel", alert.nCancel));
    return result;
}

Value listalerts(const Array& params, bool fHelp)
{
	if (fHelp)
		throw runtime_error(
            "listalerts [show_rules=false]\n"
            "List all active alerts that are living in the network (if true, will show those packets that are rules).");

	// just loop and show rule values
    Array ret;
    BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
	{
        const CAlert& alert = item.second;

	// by Simone: show those embedded rule packets only if asked to
		if (params.size() > 0)
		{
			if (!params[0].get_bool() && CAlert::isRule(alert.nPriority))
				continue;
		}
		else
		{
			if (CAlert::isRule(alert.nPriority))
				continue;
		}

        Object obj;
		obj.push_back(Pair("strComment", alert.strComment));
		obj.push_back(Pair("strStatusBar", alert.strStatusBar));
		obj.push_back(Pair("strReserved", alert.strReserved));
		obj.push_back(Pair("nVersion", alert.nVersion));
		obj.push_back(Pair("nMinVer", alert.nMinVer));
		obj.push_back(Pair("nMaxVer", alert.nMaxVer));
		obj.push_back(Pair("nPriority", alert.nPriority));
		obj.push_back(Pair("nID", alert.nID));
		if (alert.nCancel > 0)
		    obj.push_back(Pair("nCancel", alert.nCancel));
		obj.push_back(Pair("nPermanent", alert.nPermanent));
        ret.push_back(obj);
    }
    return ret;
}

// by Simone: rules RPC calls
Value sendrule(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 2)
		throw runtime_error(
            "sendrule <privatekey> <id> <encoded-rule>\n"
            "<privatekey> is hex string of alert master private key\n"
            "<encoded-rule> the encoded string that defines the rule\n"
            "Returns the added rule.");

	// check stuff here, everything must actually make sense
	CRules rule;
	if (sscanf(params[1].get_str().c_str(), "%d,%d,%d,%d,%d,%d,%d,%d",
		&rule.nVersion, &rule.nID, &rule.nMinVer, &rule.nMaxVer, &rule.fromHeight, &rule.toHeight, &rule.ruleType, &rule.ruleValue) < 8)
	{
        throw runtime_error(
            "Rule has not been encoded correctly, please check again.\n");  
	}
	if (IsInitialBlockDownload())
	{
        throw runtime_error(
            "The wallet is still syncing, cannot send a rule right now.\n");  
	}
	if (IsInitialRuleDownload())
	{
        throw runtime_error(
            "The rules are not synced, cannot send a rule right now.\n");  
	}
	if (rule.fromHeight < nBestHeight)
	{
        throw runtime_error(
            "You cannot start a rule in the past, it must be set in the present or future.\n");  
	}
	if ((rule.toHeight != 0) && (rule.toHeight <= rule.fromHeight))
	{
        throw runtime_error(
            "Ending block cannot be the same or lower than starting block.\n");  
	}
	if ((rule.toHeight != 0) && (rule.toHeight - rule.fromHeight > 200000))
	{
        throw runtime_error(
            "Block range is over 200000 blocks.\n");  
	}


	// use alert system to spread the rule
    CAlert alert;
    CKey key;

    alert.strStatusBar = params[1].get_str();			// the encoded rule string
    alert.nMinVer = CONTROL_PROTOCOL_VERSION;
    alert.nMaxVer = CONTROL_PROTOCOL_VERSION;
    alert.nPriority = 999;								// rule is a special alert with priority = 999 with new protocol
    alert.nID = CAlert::getNextID();					// associated alert ID is automatic
    alert.nVersion = CONTROL_PROTOCOL_VERSION;
    alert.nPermanent = true;							// new permanent flag, the alert lives until is (or can be) manually cancelled
    alert.nRelayUntil = GetAdjustedTime() + 60;
    alert.nExpiration = GetAdjustedTime() + 60;

    CDataStream sMsg(SER_NETWORK, CONTROL_PROTOCOL_VERSION);
    sMsg << (CUnsignedAlert)alert;
    alert.vchMsg = vector<unsigned char>(sMsg.begin(), sMsg.end());

    vector<unsigned char> vchPrivKey = ParseHex(params[0].get_str());
    key.SetPrivKey(CPrivKey(vchPrivKey.begin(), vchPrivKey.end()));	// if key is not correct openssl may crash
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
        throw runtime_error(
            "Unable to sign alert, check private key?\n");  
    if(!alert.ProcessAlert()) 
        throw runtime_error(
            "Failed to process alert.\n");

	// relay alert to all nodes
    {
		LOCK(cs_vNodes);
		BOOST_FOREACH(CNode* pnode, vNodes)
			alert.RelayTo(pnode);
    }

	return true;
}

Value listrules(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listrules\n"
            "List all active rules that are living in the network.");

	// just loop and show rule values
    Array ret;
    BOOST_FOREACH(PAIRTYPE(const int, CRules)& item, mapRules)
	{
        const CRules& rule = item.second;
        Object obj;
        obj.push_back(Pair("alertId", rule.alertId));
        obj.push_back(Pair("nVersion", rule.nVersion));
        obj.push_back(Pair("nID", rule.nID));
        obj.push_back(Pair("nMinVer", rule.nMinVer));
        obj.push_back(Pair("nMaxVer", rule.nMaxVer));
        obj.push_back(Pair("fromHeight", rule.fromHeight));
        obj.push_back(Pair("toHeight", rule.toHeight));
        obj.push_back(Pair("ruleType", rule.ruleType));
        obj.push_back(Pair("ruleValue", rule.ruleValue));
        ret.push_back(obj);
    }
    return ret;
}

Value testrule(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 2)
		throw runtime_error(
            "testrule <height> <rule-type>\n"
            "<height> the block height to test\n"
            "<rule-type> the rule type required\n"
            "Returns the computed rule for the indicated height.");

    CRules *rule = CRules::getRule(params[0].get_int(), (CRules::ruleTypes)params[1].get_int());
    Object obj;
	if (rule)
	{
		obj.push_back(Pair("alertId", rule->alertId));
		obj.push_back(Pair("nVersion", rule->nVersion));
		obj.push_back(Pair("nID", rule->nID));
		obj.push_back(Pair("nMinVer", rule->nMinVer));
		obj.push_back(Pair("nMaxVer", rule->nMaxVer));
		obj.push_back(Pair("fromHeight", rule->fromHeight));
		obj.push_back(Pair("toHeight", rule->toHeight));
		obj.push_back(Pair("ruleType", rule->ruleType));
		obj.push_back(Pair("ruleValue", rule->ruleValue));
	}
    return obj;
}

