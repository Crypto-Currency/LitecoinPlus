//
// Alert system
//

#include <boost/foreach.hpp>
#include <map>

#include <openssl/ec.h> // for EC_KEY definition

#include "key.h"
#include "net.h"
#include "sync.h"
#include "ui_interface.h"
#include "alert.h"
#include "rules.h"

using namespace std;

std::map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

// by Simone: added a map that is ordered by ID... not by HASH ! much more useful for display
std::map<int, CAlert> mapAlertsById;

static const char* pszMainKey = //"04b8d49de838594c2289037043e5330f12f4cb98f0a2f0cda90a2a957c3358c95480bb6db13fd5a50368c1f24096495eb473be801e5c919b0668a2f7acf74ed291";
"0456b310195252ae22b48d5230787bc0118410c7694701c4874cf98e8870f7eb1ed11b6bc0fb7e90c5b111724d05df9d901bab49c450322d9da088ea3c11891394";
// TestNet alerts pubKey
static const char* pszTestKey = "0471dc165db490094d35cde15b1f5d755fa6ad6f2b5ed0f340e3f17f57389c3c2af113a8cbcc885bde73305a553b5640c83021128008ddf882e856336269080496";

// TestNet alerts private key
// "308201130201010420b665cff1884e53da26376fd1b433812c9a5a8a4d5221533b15b9629789bb7e42a081a53081a2020101302c06072a8648ce3d0101022100fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f300604010004010704410479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8022100fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141020101a1440342000471dc165db490094d35cde15b1f5d755fa6ad6f2b5ed0f340e3f17f57389c3c2af113a8cbcc885bde73305a553b5640c83021128008ddf882e856336269080496"

void CUnsignedAlert::SetNull()
{
    nVersion = 1;
    nRelayUntil = 0;
    nExpiration = 0;
    nID = 0;
    nCancel = 0;
    setCancel.clear();
    nMinVer = 0;
    nMaxVer = 0;
    setSubVer.clear();
    nPriority = 0;
    nPermanent = false;

    strComment.clear();
    strStatusBar.clear();
    strReserved.clear();
}

std::string CUnsignedAlert::ToString() const
{
    std::string strSetCancel;
    BOOST_FOREACH(int n, setCancel)
        strSetCancel += strprintf("%d ", n);
    std::string strSetSubVer;
    BOOST_FOREACH(std::string str, setSubVer)
        strSetSubVer += "\"" + str + "\" ";
    return strprintf(
        "CAlert(\n"
        "    nVersion     = %d\n"
        "    nRelayUntil  = %" PRI64d "\n"
        "    nExpiration  = %" PRI64d "\n"
        "    nID          = %d\n"
        "    nCancel      = %d\n"
        "    setCancel    = %s\n"
        "    nMinVer      = %d\n"
        "    nMaxVer      = %d\n"
        "    setSubVer    = %s\n"
        "    nPriority    = %d\n"
        "    strComment   = \"%s\"\n"
        "    strStatusBar = \"%s\"\n"
        ")\n",
        nVersion,
        nRelayUntil,
        nExpiration,
        nID,
        nCancel,
        strSetCancel.c_str(),
        nMinVer,
        nMaxVer,
        strSetSubVer.c_str(),
        nPriority,
        strComment.c_str(),
        strStatusBar.c_str());
}

void CUnsignedAlert::print() const
{
    printf("%s", ToString().c_str());
}

void CAlert::SetNull()
{
    CUnsignedAlert::SetNull();
    vchMsg.clear();
    vchSig.clear();
}

bool CAlert::IsNull() const
{
    return (nExpiration == 0);
}

uint256 CAlert::GetHash() const
{
    return Hash(this->vchMsg.begin(), this->vchMsg.end());
}

bool CAlert::IsInEffect() const
{
	// by Simone: if permanent, always return true, always in effect
    return (nPermanent ? nPermanent : (GetAdjustedTime() < nExpiration));
}

bool CAlert::Cancels(const CAlert& alert) const
{
	if (!IsInEffect())
	{
		return false;		// this was a no-op before 31403
	}
	if (alert.nPermanent)
		return false;		// this type never cancels
    return (alert.nID <= nCancel || setCancel.count(alert.nID));
}

bool CAlert::AppliesTo(int nVersion, std::string strSubVerIn) const
{
    // TODO: rework for client-version-embedded-in-strSubVer ?
	if (IsInEffect())
	{
		if ((nMinVer >= CONTROL_PROTOCOL_VERSION) && (nPriority == 999))
		{
			CRules::insert(*this);
			printf("alert/rule %d has been added by dedicated message\n", nID);
			return false;			// when rule protocol was introduced, priority 999 is a rule message, doesn't apply as alert, ever
		}
		else
		{
			return (nMinVer <= nVersion && nVersion <= nMaxVer &&
				    (setSubVer.empty() || setSubVer.count(strSubVerIn)));
		}
	}
	return false;
}

bool CAlert::AppliesToMe() const
{
    return AppliesTo(PROTOCOL_VERSION, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<std::string>()));
}

bool CAlert::RelayTo(CNode* pnode) const
{
    if (!IsInEffect())
        return false;
    // returns true if wasn't already contained in the set
    if (pnode->setKnown.insert(GetHash()).second)
    {
        if (AppliesTo(pnode->nVersion, pnode->strSubVer) ||
            AppliesToMe() ||
			nPermanent ||						// by Simone: if this flag is raised, just push this to everyone, always, forever
            GetAdjustedTime() < nRelayUntil)
        {
            pnode->PushMessage("alert", *this);
            return true;
        }
    }
    return false;
}

bool CAlert::CheckSignature() const
{
    CKey key;
    if (!key.SetPubKey(ParseHex(fTestNet ? pszTestKey : pszMainKey)))
        return error("CAlert::CheckSignature() : SetPubKey failed");
    if (!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
        return error("CAlert::CheckSignature() : verify signature failed");

    // Now unserialize the data
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *(CUnsignedAlert*)this;
    return true;
}

CAlert CAlert::getAlertByHash(const uint256 &hash)
{
    CAlert retval;
    {
        LOCK(cs_mapAlerts);
        map<uint256, CAlert>::iterator mi = mapAlerts.find(hash);
        if(mi != mapAlerts.end())
            retval = mi->second;
    }
    return retval;
}

bool CAlert::ProcessAlert()
{
    if (!CheckSignature())
        return false;
    if (!IsInEffect())
        return false;

    // alert.nID=max is reserved for if the alert key is
    // compromised. It must have a pre-defined message,
    // must never expire, must apply to all versions,
    // and must cancel all previous
    // alerts or it will be ignored (so an attacker can't
    // send an "everything is OK, don't panic" version that
    // cannot be overridden):
    int maxInt = std::numeric_limits<int>::max();
    if (nID == maxInt)
    {
        if (!(
                nExpiration == maxInt &&
                nCancel == (maxInt-1) &&
                nMinVer == 0 &&
                nMaxVer == maxInt &&
                setSubVer.empty() &&
                nPriority == maxInt &&
                strStatusBar == "URGENT: Alert key compromised, upgrade required"
                ))
            return false;
    }

    {
        LOCK(cs_mapAlerts);
        // Cancel previous alerts
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
        {
            const CAlert& alert = (*mi).second;
			if (nID == alert.nID)
			{
                printf("alert %d already exist in queue , skipping\n", alert.nID);
                return false;
			}
            else if (Cancels(alert))
            {
                printf("cancelling alert %d\n", alert.nID);
                uiInterface.NotifyAlertChanged((*mi).first, CT_DELETED);
				mapAlertsById.erase(alert.nID);
                mapAlerts.erase(mi++);
            }
            else if (!alert.IsInEffect())
            {
                printf("expiring alert %d\n", alert.nID);
                uiInterface.NotifyAlertChanged((*mi).first, CT_DELETED);
				mapAlertsById.erase(alert.nID);
                mapAlerts.erase(mi++);
            }
            else
                mi++;
        }

        // Check if this alert has been cancelled
        BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.Cancels(*this))
            {
                printf("alert already cancelled by %d\n", alert.nID);
                return false;
            }
        }

        // Add to mapAlerts (and remember when it was raised first)
		nReceivedOn = GetTime();
        mapAlerts.insert(make_pair(GetHash(), *this));
        mapAlertsById.insert(make_pair(nID, *this));
        // Notify UI if it applies to me
        if(AppliesToMe())
            uiInterface.NotifyAlertChanged(GetHash(), CT_NEW);
    }

    printf("accepted alert %d, AppliesToMe()=%d\n", nID, AppliesToMe());
    return true;
}

// by Simone: returns the next available free ID to raise a new alert
int CAlert::getNextID()
{
    {
        LOCK(cs_mapAlerts);

	// when there is nothing inside, the first ID is available
		if (mapAlertsById.size() == 0)
		{
			return 1;
		}

	// just loop and find an available ID
		int i = 1;
		loop()
		{

		// let's check we don't overflow max int value, and return an invalid ID
			if (i == std::numeric_limits<int>::max())
			{
				return -1;
			}

		// if this ID is unused, then it can be used
			if (mapAlertsById.find(i) == mapAlertsById.end())
			{
				return i;
			}
			i++;
		}
	}
}

void CAlert::ProcessAlerts()
{
    {
        LOCK(cs_mapAlerts);
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
        {
            const CAlert& alert = (*mi).second;
			if (!alert.IsInEffect())
            {
                printf("expiring alert %d\n", alert.nID);
                uiInterface.NotifyAlertChanged((*mi).first, CT_DELETED);
				mapAlertsById.erase(alert.nID);
                mapAlerts.erase(mi++);
            }
            else
                mi++;
        }
	}
}


bool CAlert::isInfo(int priority)
{
	return (priority <= 300);
}

bool CAlert::isWarning(int priority)
{
	return (priority <= 600);
}

bool CAlert::isCritical(int priority)
{
	return ((priority > 600) && (priority < 999));
}

bool CAlert::isSuperCritical(int priority)
{
	return (priority >= 1000);
}

bool CAlert::isRule(int priority)
{
	return (priority == 999);
}

