// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "clientversion.h"
#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

// as of version 3.3.2.10, we do not accept anymore older version of clients, as they are forked and likely messing the network up
static const int MIN_CLIENT_VERSION =
                           1000000 * 3
                         +   10000 * 3
                         +     100 * 2
                         +       1 * 10;

// as of version 5.0.0.1, we have dynamicized older version acceptance of clients, with PALADIN system capabilities, define a constant here
static const int PALADIN_CLIENT_VERSION =
                           1000000 * 5
                         +   10000 * 0
                         +     100 * 0
                         +       1 * 1;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

//
// network protocol versioning
//

static const int PROTOCOL_VERSION = 60007;

// by Simone: this is the new version of alert system, they apply only to this one, to avoid messing up old wallets
static const int CONTROL_PROTOCOL_VERSION = 60008;

// earlier versions not supported as of Feb 2012, and are disconnected
static const int MIN_PROTO_VERSION = 209;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 60002;
static const int NOBLKS_VERSION_END = 60004;

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

#define DISPLAY_VERSION_MAJOR    CLIENT_VERSION_MAJOR
#define DISPLAY_VERSION_MINOR    CLIENT_VERSION_MINOR
#define DISPLAY_VERSION_REVISION CLIENT_VERSION_REVISION
#define DISPLAY_VERSION_BUILD    CLIENT_VERSION_BUILD

#endif
