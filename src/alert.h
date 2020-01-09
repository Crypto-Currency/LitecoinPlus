// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINALERT_H_
#define _BITCOINALERT_H_ 1

#include <set>
#include <string>
#include <map>

#include "uint256.h"
#include "util.h"
#include "sync.h"

// by Simone: extern declared here for everyone...
class CAlert;
extern CCriticalSection cs_mapAlerts;
extern std::map<uint256, CAlert> mapAlerts;
extern std::map<int, CAlert> mapAlertsById;

// classes definition below
class CNode;

/** Alerts are for notifying old versions if they become too obsolete and
 * need to upgrade.  The message is displayed in the status bar.
 * Alert messages are broadcast as a vector of signed data.  Unserializing may
 * not read the entire buffer if the alert is for a newer version, but older
 * versions can still relay the original data.
 */
class CUnsignedAlert
{
public:
    int nVersion;
    int64 nRelayUntil;					// when newer nodes stop relaying to newer nodes
    int64 nExpiration;
    int nID;
    int nCancel;
    std::set<int> setCancel;
    int nMinVer;						// lowest version inclusive
    int nMaxVer;						// highest version inclusive
    std::set<std::string> setSubVer;	// empty matches all
    int nPriority;						// by Simone: since version CONTROL_PROTOCOL_VERSION, priority 999 is reserved to RULE messages
    bool nPermanent;					// by Simone: this special alert permanently reside in memory
	int64 nReceivedOn;					// by Simone: let's try to keep a date/time when this was first received, shall we...

    // Actions
    std::string strComment;
    std::string strStatusBar;
    std::string strReserved;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nRelayUntil);
        READWRITE(nExpiration);
        READWRITE(nID);
        READWRITE(nCancel);
        READWRITE(setCancel);
        READWRITE(nMinVer);
        READWRITE(nMaxVer);
        READWRITE(setSubVer);
        READWRITE(nPriority);

		if (nVersion >= CONTROL_PROTOCOL_VERSION)
		{
			READWRITE(nPermanent);
		}

        READWRITE(strComment);
        READWRITE(strStatusBar);
        READWRITE(strReserved);
    )

    void SetNull();

    std::string ToString() const;
    void print() const;
};

/** An alert is a combination of a serialized CUnsignedAlert and a signature. */
class CAlert : public CUnsignedAlert
{
public:
    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

    CAlert()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(vchMsg);
        READWRITE(vchSig);
    )

    void SetNull();
    bool IsNull() const;
    uint256 GetHash() const;
    bool IsInEffect() const;
    bool Cancels(const CAlert& alert) const;
    bool AppliesTo(int nVersion, std::string strSubVerIn) const;
    bool AppliesToMe() const;
    bool RelayTo(CNode* pnode) const;
    bool CheckSignature() const;
    bool ProcessAlert();

    /*
     * Get copy of (active) alert object by hash. Returns a null alert if it is not found.
     */
    static CAlert getAlertByHash(const uint256 &hash);

	// by Simone: task to process all alerts, that should be called periodically to cancel/expire stuff
	static void ProcessAlerts();

	// by Simone: returns the next available ID that can be used to send an alert
	static int getNextID();

	// by Simone: define a way to range the alerts
	static bool isInfo(int priority);
	static bool isWarning(int priority);
	static bool isCritical(int priority);
	static bool isSuperCritical(int priority);
	static bool isRule(int priority);
};

#endif
