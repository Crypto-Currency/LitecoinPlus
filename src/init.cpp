// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "db.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "net.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "checkpoints.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <openssl/crypto.h>

//#ifndef WIN32 && __APPLE__
#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>
#endif

using namespace std;
using namespace boost;

CWallet* pwalletMain;
CClientUIInterface uiInterface;

//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

void ExitTimeout(void* parg)
{
#ifdef WIN32
    Sleep(5000);
    ExitProcess(0);
#endif
}

void StartShutdown()
{
#ifdef QT_GUI
    // ensure we leave the Qt main loop for a clean GUI exit (Shutdown() is called in bitcoin.cpp afterwards)
    uiInterface.QueueShutdown();
#else
    // Without UI, Shutdown() can simply be started in a new thread
    NewThread(Shutdown, NULL);
#endif
}

extern CTxDB *gtxdb;
void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;

    // Make this thread recognisable as the shutdown thread
	// By Simone: changed the thread name to litecoinplus- instead of bitcoin-
    RenameThread("litecoinplus-shutoff");

    bool fFirstThread = false;
    {
        TRY_LOCK(cs_Shutdown, lockShutdown);
        if (lockShutdown)
        {
            fFirstThread = !fTaken;
            fTaken = true;
        }
    }
    static bool fExit;
    if (fFirstThread)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        bitdb.Flush(false);
        StopNode();
	    UnregisterNodeSignals(GetNodeSignals());
		if (gtxdb) {
			gtxdb->Close();
		}
        bitdb.Flush(true);
        boost::filesystem::remove(GetPidFile());
        UnregisterWallet(pwalletMain);
        delete pwalletMain;
        NewThread(ExitTimeout, NULL);
        Sleep(50);
        printf("LitecoinPlus exited\n\n");
        fExit = true;
		fShutdown = false;

	// by Simone: if the fStartOver flag is raised, we dump all the blockchain files securely after the database is close and detached
		if (fStartOver)
		{
			boost::filesystem::path p = GetDataDir();
			boost::filesystem::directory_iterator end_itr;
			for (boost::filesystem::directory_iterator itr(p / "database"); itr != end_itr; ++itr)
			{
				boost::filesystem::remove(*itr);
			}
			boost::filesystem::remove(p / "database");
			for (int i = 1; ; i++)
			{
				char s[64];
				sprintf(s, "blk%04d.dat", i);
				if (!boost::filesystem::exists(p / s))
				{
					break;
				}
				boost::filesystem::remove(p / s);
			}
			boost::filesystem::remove(p / "blkindex.dat");
			boost::filesystem::remove(p / "txindex.dat");
		}

#ifndef QT_GUI
        // ensure non-UI client gets exited here, but let Bitcoin-Qt reach 'return 0;' in bitcoin.cpp
        exit(0);
#endif
    }
    else
    {
        while (!fExit)
            Sleep(500);
        Sleep(100);
        ExitThread(0);
    }
}

void HandleSIGTERM(int)
{
    fRequestShutdown = true;
}

void HandleSIGHUP(int)
{
    fReopenDebugLog = true;
}





//////////////////////////////////////////////////////////////////////////////
//
// Start
//
#if !defined(QT_GUI)
bool AppInit(int argc, char* argv[])
{
    bool fRet = false;
    try
    {


        //
        // Parameters
        //
        // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
        ParseParameters(argc, argv);
        if (!boost::filesystem::is_directory(GetDataDir(false)))
        {
            fprintf(stderr, "Error: Specified directory does not exist\n");
            Shutdown(NULL);
        }
        ReadConfigFile(mapArgs, mapMultiArgs);

        if (mapArgs.count("-?") || mapArgs.count("--help"))
        {
            // First part of help message is specific to bitcoind / RPC client
            std::string strUsage = _("LitecoinPlus version") + " " + FormatFullVersion() + "\n\n" +
                _("Usage:") + "\n" +
                  "  LitecoinPlusd [options]                     " + "\n" +
                  "  LitecoinPlusd [options] <command> [params]  " + _("Send command to -server or LitecoinPlusd") + "\n" +
                  "  LitecoinPlusd [options] help                " + _("List commands") + "\n" +
                  "  LitecoinPlusd [options] help <command>      " + _("Get help for a command") + "\n";

            strUsage += "\n" + HelpMessage();

            fprintf(stdout, "%s", strUsage.c_str());
            return false;
        }

        // Command-line RPC
        for (int i = 1; i < argc; i++)
            if (!IsSwitchChar(argv[i][0]) && !boost::algorithm::istarts_with(argv[i], "LitecoinPlus:"))
                fCommandLine = true;

        if (fCommandLine)
        {
            int ret = CommandLineRPC(argc, argv);
            exit(ret);
        }

        fRet = AppInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "AppInit()");
    } catch (...) {
        PrintException(NULL, "AppInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

extern void noui_connect();
int main(int argc, char* argv[])
{
    bool fRet = false;

    // Connect bitcoind signal handlers
    noui_connect();

    fRet = AppInit(argc, argv);

    if (fRet && fDaemon)
        return 0;

    return 1;
}
#endif

bool static InitError(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, _("LitecoinPlus"), CClientUIInterface::OK | CClientUIInterface::MODAL);
    return false;
}

bool static InitWarning(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, _("LitecoinPlus"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
    return true;
}


bool static Bind(const CService &addr, bool fError = true) {
    if (IsLimited(addr))
        return false;
    std::string strError;
    if (!BindListenPort(addr, strError)) {
        if (fError)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI and daemon
std::string HelpMessage()
{
    string strUsage = _("Options:") + "\n" +
        "  -?                     " + _("This help message") + "\n" +
        "  -conf=<file>           " + _("Specify configuration file (default: LitecoinPlus.conf)") + "\n" +
        "  -pid=<file>            " + _("Specify pid file (default: LitecoinPlusd.pid)") + "\n" +
        "  -gen                   " + _("Generate coins") + "\n" +
        "  -gen=0                 " + _("Don't generate coins") + "\n" +
        "  -datadir=<dir>         " + _("Specify data directory") + "\n" +
        "  -dbcache=<n>           " + _("Set database cache size in megabytes (default: 25)") + "\n" +
        "  -dblogsize=<n>         " + _("Set database disk log size in megabytes (default: 100)") + "\n" +
        "  -timeout=<n>           " + _("Specify connection timeout in milliseconds (default: 5000)") + "\n" +
        "  -proxy=<ip:port>       " + _("Connect through socks proxy") + "\n" +
        "  -socks=<n>             " + _("Select the version of socks proxy to use (4-5, default: 5)") + "\n" +
        "  -tor=<ip:port>         " + _("Use proxy to reach tor hidden services (default: same as -proxy)") + "\n"
        "  -dns                   " + _("Allow DNS lookups for -addnode, -seednode and -connect") + "\n" +
        "  -port=<port>           " + _("Listen for connections on <port> (default: 44351 or testnet: 44352)") + "\n" +
        "  -maxconnections=<n>    " + _("Maintain at most <n> connections to peers (default: 125)") + "\n" +
        "  -addnode=<ip>          " + _("Add a node to connect to and attempt to keep the connection open") + "\n" +
        "  -connect=<ip>          " + _("Connect only to the specified node(s)") + "\n" +
        "  -seednode=<ip>         " + _("Connect to a node to retrieve peer addresses, and disconnect") + "\n" +
        "  -externalip=<ip>       " + _("Specify your own public address") + "\n" +
        "  -onlynet=<net>         " + _("Only connect to nodes in network <net> (IPv4, IPv6 or Tor)") + "\n" +
        "  -discover              " + _("Discover own IP address (default: 1 when listening and no -externalip)") + "\n" +
        "  -listen                " + _("Accept connections from outside (default: 1 if no -proxy or -connect)") + "\n" +
        "  -bind=<addr>           " + _("Bind to given address. Use [host]:port notation for IPv6") + "\n" +
        "  -dnsseed               " + _("Find peers using DNS lookup (default: 0)") + "\n" +
        "  -nosynccheckpoints     " + _("Disable sync checkpoints (default: 0)") + "\n" +
        "  -banscore=<n>          " + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n" +
        "  -bantime=<n>           " + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n" +
        "  -maxreceivebuffer=<n>  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") + "\n" +
        "  -maxsendbuffer=<n>     " + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") + "\n" +
#ifdef USE_UPNP
#if USE_UPNP
        "  -upnp                  " + _("Use UPnP to map the listening port (default: 1 when listening)") + "\n" +
#else
        "  -upnp                  " + _("Use UPnP to map the listening port (default: 0)") + "\n" +
#endif
#endif
        "  -detachdb              " + _("Detach block and address databases. Increases shutdown time (default: 0)") + "\n" +
        "  -paytxfee=<amt>        " + _("Fee per KB to add to transactions you send") + "\n" +
#ifdef QT_GUI
        "  -server                " + _("Accept command line and JSON-RPC commands") + "\n" +
#endif
#if !defined(WIN32) && !defined(QT_GUI)
        "  -daemon                " + _("Run in the background as a daemon and accept commands") + "\n" +
#endif
        "  -testnet               " + _("Use the test network") + "\n" +
        "  -netoffline            " + _("Starts with the wallet offline (default: starts online)") + "\n" +
        "  -debug                 " + _("Output extra debugging information. Implies all other -debug* options") + "\n" +
        "  -debugnet              " + _("Output extra network debugging information") + "\n" +
        "  -logtimestamps         " + _("Prepend debug output with timestamp") + "\n" +
        "  -shrinkdebugfile       " + _("Shrink debug.log file on client startup (default: 1 when no -debug)") + "\n" +
        "  -printtoconsole        " + _("Send trace/debug info to console instead of debug.log file") + "\n" +
#ifdef WIN32
        "  -printtodebugger       " + _("Send trace/debug info to debugger") + "\n" +
#endif
        "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n" +
        "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n" +
        "  -rpcport=<port>        " + _("Listen for JSON-RPC connections on <port> (default: 44350 or testnet: 44353)") + "\n" +
        "  -rpcallowip=<ip>       " + _("Allow JSON-RPC connections from specified IP address") + "\n" +
        "  -rpcconnect=<ip>       " + _("Send commands to node running on <ip> (default: 127.0.0.1)") + "\n" +
        "  -blocknotify=<cmd>     " + _("Execute command when the best block changes (%s in cmd is replaced by block hash)") + "\n" +
		"  -walletnotify=<cmd>    " + _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)") + "\n" +
        "  -upgradewallet         " + _("Upgrade wallet to latest format") + "\n" +
        "  -keypool=<n>           " + _("Set key pool size to <n> (default: 100)") + "\n" +
        "  -rescan                " + _("Rescan the block chain for missing wallet transactions") + "\n" +
        "  -salvagewallet         " + _("Attempt to recover private keys from a corrupt wallet.dat") + "\n" +
        "  -zapwallettxes  " + _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") + "\n" +
        "  -checkblocks=<n>       " + _("How many blocks to check at startup (default: 2500, 0 = all)") + "\n" +
        "  -checklevel=<n>        " + _("How thorough the block verification is (0-6, default: 1)") + "\n" +
        "  -loadblock=<file>      " + _("Imports blocks from external blk000?.dat file") + "\n" +

        "\n" + _("Block creation options:") + "\n" +
        "  -blockminsize=<n>      "   + _("Set minimum block size in bytes (default: 0)") + "\n" +
        "  -blockmaxsize=<n>      "   + _("Set maximum block size in bytes (default: 250000)") + "\n" +
        "  -blockprioritysize=<n> "   + _("Set maximum size of high-priority/low-fee transactions in bytes (default: 27000)") + "\n" +

        "\n" + _("SSL options: (see the Bitcoin Wiki for SSL setup instructions)") + "\n" +
        "  -rpcssl                                  " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n" +
        "  -rpcsslcertificatechainfile=<file.cert>  " + _("Server certificate file (default: server.cert)") + "\n" +
        "  -rpcsslprivatekeyfile=<file.pem>         " + _("Server private key (default: server.pem)") + "\n" +
        "  -rpcsslciphers=<ciphers>                 " + _("Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)") + "\n";

    return strUsage;
}

// by Simone: before doing anything, let's capture this status
bool txIndexFileExists = true;

// by Simone: core dump handler
//#ifndef WIN32
#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext {
 unsigned long     uc_flags;
 struct ucontext   *uc_link;
 stack_t           uc_stack;
 struct sigcontext uc_mcontext;
 sigset_t          uc_sigmask;
} sig_ucontext_t;

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext)
{
 void *             array[50];
 void *             caller_address;
 char **            messages;
 int                size, i;
 sig_ucontext_t *   uc;

 uc = (sig_ucontext_t *)ucontext;

 /* Get the address at the time the signal was raised */
#if defined(__i386__) // gcc specific
 caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
 caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
#elif defined(__arm__)   // gcc specific
 caller_address = (void *) uc->uc_mcontext.arm_ip;	// ARM specific
#else
 caller_address = NULL;
#endif

 fprintf(stderr, "signal %d (%s), address is %p from %p\n", 
  sig_num, strsignal(sig_num), info->si_addr, 
  (void *)caller_address);

 size = backtrace(array, 50);

 /* overwrite sigaction with caller's address */
 array[1] = caller_address;

 messages = backtrace_symbols(array, size);

 /* skip first stack frame (points here) */
 for (i = 1; i < size && messages != NULL; ++i)
 {
  fprintf(stderr, "[bt]: (%d) %s\n", i, messages[i]);
 }

 free(messages);

 exit(EXIT_FAILURE);
}
#endif

/** Initialize bitcoin.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInit2()
{
//#ifndef WIN32
#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
	// by Simone: adding core dumps handler, at least in Linux
	struct sigaction sigact;

	sigact.sa_sigaction = crit_err_hdlr;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0)
	{
		fprintf(stderr, "error setting signal handler for %d (%s)\n",
		SIGSEGV, strsignal(SIGSEGV));

		exit(EXIT_FAILURE);
	}
#endif

    // ********************************************************* Step 1: setup
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
    // Disable confusing "helpful" text message on abort, Ctrl-C
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef WIN32
    // Enable Data Execution Prevention (DEP)
    // Minimum supported OS versions: WinXP SP3, WinVista >= SP1, Win Server 2008
    // A failure is non-critical and needs no further attention!
#ifndef PROCESS_DEP_ENABLE
// We define this here, because GCCs winbase.h limits this to _WIN32_WINNT >= 0x0601 (Windows 7),
// which is not correct. Can be removed, when GCCs winbase.h is fixed!
#define PROCESS_DEP_ENABLE 0x00000001
#endif
    typedef BOOL (WINAPI *PSETPROCDEPPOL)(DWORD);
    PSETPROCDEPPOL setProcDEPPol = (PSETPROCDEPPOL)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetProcessDEPPolicy");
    if (setProcDEPPol != NULL) setProcDEPPol(PROCESS_DEP_ENABLE);
#endif
//#ifndef WIN32
#if (!defined(_WIN32)) && (!defined(WIN32)) && (!defined(__APPLE__))
    umask(077);

    // Clean shutdown on SIGTERM
    struct sigaction sa;
    sa.sa_handler = HandleSIGTERM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Reopen debug.log on SIGHUP
    struct sigaction sa_hup;
    sa_hup.sa_handler = HandleSIGHUP;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    sigaction(SIGHUP, &sa_hup, NULL);
#endif

    // ********************************************************* Step 2: parameter interactions
    SoftSetBoolArg("-listen", true); // just making sure

    fTestNet = GetBoolArg("-testnet");

	extern bool netOffline;
	netOffline = GetBoolArg("-netoffline", false);
	setOnlineStatus(!netOffline);

    if (mapArgs.count("-bind")) {
        // when specifying an explicit binding address, you want to listen on it
        // even when -connect or -proxy is specified
        SoftSetBoolArg("-listen", true);
    }

    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        SoftSetBoolArg("-dnsseed", false);
        SoftSetBoolArg("-listen", false);
    }

    if (mapArgs.count("-proxy")) {
        // to protect privacy, do not listen by default if a proxy server is specified
        SoftSetBoolArg("-listen", false);
    }

    if (!GetBoolArg("-listen", true)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        SoftSetBoolArg("-upnp", false);
        SoftSetBoolArg("-discover", false);
    }

    if (mapArgs.count("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        SoftSetBoolArg("-discover", false);
    }

    if (GetBoolArg("-salvagewallet")) {
        // Rewrite just private keys: rescan to find transactions
        SoftSetBoolArg("-rescan", true);
    }

    if (GetBoolArg("-zapwallettxes", false)) {
        // -zapwallettx implies a rescan
        SoftSetBoolArg("-rescan", true);
    }

    // ********************************************************* Step 3: parameter-to-internal-flags

    fDebug = GetBoolArg("-debug");

    // -debug implies fDebug*
    if (fDebug)
        fDebugNet = true;
    else
        fDebugNet = GetBoolArg("-debugnet");

    bitdb.SetDetach(GetBoolArg("-detachdb", false));

#if !defined(WIN32) && !defined(QT_GUI)
    fDaemon = GetBoolArg("-daemon");
#else
    fDaemon = false;
#endif

    if (fDaemon)
        fServer = true;
    else
        fServer = GetBoolArg("-server");

    /* force fServer when running without GUI */
#if !defined(QT_GUI)
    fServer = true;
#endif
    fPrintToConsole = GetBoolArg("-printtoconsole");
    fPrintToDebugger = GetBoolArg("-printtodebugger");
    fLogTimestamps = GetBoolArg("-logtimestamps");

    if (mapArgs.count("-timeout"))
    {
        int nNewTimeout = GetArg("-timeout", 5000);
        if (nNewTimeout > 0 && nNewTimeout < 600000)
            nConnectTimeout = nNewTimeout;
    }

    // Continue to put "/P2SH/" in the coinbase to monitor
    // BIP16 support.
    // This can be removed eventually...
    const char* pszP2SH = "/P2SH/";
    COINBASE_FLAGS << std::vector<unsigned char>(pszP2SH, pszP2SH+strlen(pszP2SH));


    if (mapArgs.count("-paytxfee"))
    {
        if (!ParseMoney(mapArgs["-paytxfee"], nTransactionFee))
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s'"), mapArgs["-paytxfee"].c_str()));
        if (nTransactionFee > 0.25 * COIN)
            InitWarning(_("Warning: -paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
    }

    // ********************************************************* Step 4: application initialization: dir lock, daemonize, pidfile, debug log

    std::string strDataDir = GetDataDir().string();

    // Make sure only a single Bitcoin process is using the data directory.
    boost::filesystem::path pathLockFile = GetDataDir() / ".lock";
    FILE* file = fopen(pathLockFile.string().c_str(), "a"); // empty lock file; created if it doesn't exist.
    if (file) fclose(file);
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock())
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s.  LitecoinPlus is probably already running."), strDataDir.c_str()));

#if !defined(WIN32) && !defined(QT_GUI)
    if (fDaemon)
    {
        // Daemonize
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
            return false;
        }
        if (pid > 0)
        {
            CreatePidFile(GetPidFile(), pid);
            return true;
        }

        pid_t sid = setsid();
        if (sid < 0)
            fprintf(stderr, "Error: setsid() returned %d errno %d\n", sid, errno);
    }
#endif


    if (GetBoolArg("-shrinkdebugfile", !fDebug))
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("LitecoinPlus version %s (%s)\n", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    printf("Using OpenSSL version %s\n", SSLeay_version(SSLEAY_VERSION));
    if (!fLogTimestamps)
        printf("Startup time: %s\n", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str());
    printf("Default data directory %s\n", GetDefaultDataDir().string().c_str());
    printf("Used data directory %s\n", strDataDir.c_str());
    //check for themes directory, and create if missing
    if (!filesystem::exists(GetDataDir() / "themes"))
    {
      boost::filesystem::path temppath;
      temppath = GetDataDir() / "themes";
      filesystem::create_directory(temppath);
      printf("created themes directory %s\n", temppath.string().c_str());
      temppath = GetDataDir() / "themes/images";
      filesystem::create_directory(temppath);
      printf("created themes directory %s\n", temppath.string().c_str());
    }

    std::ostringstream strErrors;


	// by Simone: we check if the file txindex.dat exists, otherwise we need to create it from the old index, the guts will take care of it
	boost::filesystem::path path = GetDataDir() / "txindex.dat";
	boost::filesystem::path pathBlk = GetDataDir() / "blkindex.dat";
	if ((!boost::filesystem::exists(path)) && (boost::filesystem::exists(pathBlk)))
	{
		boost::filesystem::path pathOld = GetDataDir() / "blkindex.dat";
		boost::filesystem::rename(pathOld, path);
		txIndexFileExists = false;
	}

    if (fDaemon)
        fprintf(stdout, "LitecoinPlus server starting\n");

    int64 nStart;

    // ********************************************************* Step 5: verify database integrity

    uiInterface.InitMessage(_("Verifying database integrity..."));

    if (!bitdb.Open(GetDataDir()))
    {
        string msg = strprintf(_("Error initializing database environment %s!"
                                 " To recover, BACKUP THAT DIRECTORY, then remove"
                                 " everything from it except for wallet.dat."), strDataDir.c_str());
        return InitError(msg);
    }

    if (GetBoolArg("-salvagewallet"))
    {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, "wallet.dat", true))
            return false;
    }

    if (filesystem::exists(GetDataDir() / "wallet.dat"))
    {
        CDBEnv::VerifyResult r = bitdb.Verify("wallet.dat", CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK)
        {
            string msg = strprintf(_("Warning: wallet.dat corrupt, data salvaged!"
                                     " Original wallet.dat saved as wallet.{timestamp}.bak in %s; if"
                                     " your balance or transactions are incorrect you should"
                                     " restore from a backup."), strDataDir.c_str());
            uiInterface.ThreadSafeMessageBox(msg, _("LitecoinPlus"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
        }
        if (r == CDBEnv::RECOVER_FAIL)
            return InitError(_("wallet.dat corrupt, salvage failed"));
    }

    // ********************************************************* Step 6: network initialization

    RegisterNodeSignals(GetNodeSignals());

    int nSocksVersion = GetArg("-socks", 5);

    if (nSocksVersion != 4 && nSocksVersion != 5)
        return InitError(strprintf(_("Unknown -socks proxy version requested: %i"), nSocksVersion));

    if (mapArgs.count("-onlynet")) {
        std::set<enum Network> nets;
        BOOST_FOREACH(std::string snet, mapMultiArgs["-onlynet"]) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(strprintf(_("Unknown network specified in -onlynet: '%s'"), snet.c_str()));
            nets.insert(net);
        }
        for (int n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net))
                SetLimited(net);
        }
    }
#if defined(USE_IPV6)
#if ! USE_IPV6
    else
        SetLimited(NET_IPV6);
#endif
#endif

    CService addrProxy;
    bool fProxy = false;
    if (mapArgs.count("-proxy")) {
        addrProxy = CService(mapArgs["-proxy"], 9050);
        if (!addrProxy.IsValid())
            return InitError(strprintf(_("Invalid -proxy address: '%s'"), mapArgs["-proxy"].c_str()));

        if (!IsLimited(NET_IPV4))
            SetProxy(NET_IPV4, addrProxy, nSocksVersion);
        if (nSocksVersion > 4) {
#ifdef USE_IPV6
            if (!IsLimited(NET_IPV6))
                SetProxy(NET_IPV6, addrProxy, nSocksVersion);
#endif
            SetNameProxy(addrProxy, nSocksVersion);
        }
        fProxy = true;
    }

    // -tor can override normal proxy, -notor disables tor entirely
    if (!(mapArgs.count("-tor") && mapArgs["-tor"] == "0") && (fProxy || mapArgs.count("-tor"))) {
        CService addrOnion;
        if (!mapArgs.count("-tor"))
            addrOnion = addrProxy;
        else
            addrOnion = CService(mapArgs["-tor"], 9050);
        if (!addrOnion.IsValid())
            return InitError(strprintf(_("Invalid -tor address: '%s'"), mapArgs["-tor"].c_str()));
        SetProxy(NET_TOR, addrOnion, 5);
        SetReachable(NET_TOR);
    }

    // see Step 2: parameter interactions for more information about these
    fNoListen = !GetBoolArg("-listen", true);
    fDiscover = GetBoolArg("-discover", true);
    fNameLookup = GetBoolArg("-dns", true);
#ifdef USE_UPNP
    fUseUPnP = GetBoolArg("-upnp", USE_UPNP);
#endif

    bool fBound = false;
    if (!fNoListen)
    {
        std::string strError;
        if (mapArgs.count("-bind")) {
            BOOST_FOREACH(std::string strBind, mapMultiArgs["-bind"]) {
                CService addrBind;
                if (!Lookup(strBind.c_str(), addrBind, GetListenPort(), false))
                    return InitError(strprintf(_("Cannot resolve -bind address: '%s'"), strBind.c_str()));
                fBound |= Bind(addrBind);
            }
        } else {
            struct in_addr inaddr_any;
            inaddr_any.s_addr = INADDR_ANY;
#ifdef USE_IPV6
            if (!IsLimited(NET_IPV6))
                fBound |= Bind(CService(in6addr_any, GetListenPort()), false);
#endif
            if (!IsLimited(NET_IPV4))
                fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound);
        }
        if (!fBound)
            return InitError(_("Failed to listen on any port. Use -listen=0 if you want this."));
    }

    if (mapArgs.count("-externalip"))
    {
        BOOST_FOREACH(string strAddr, mapMultiArgs["-externalip"]) {
            CService addrLocal(strAddr, GetListenPort(), fNameLookup);
            if (!addrLocal.IsValid())
                return InitError(strprintf(_("Cannot resolve -externalip address: '%s'"), strAddr.c_str()));
            AddLocal(CService(strAddr, GetListenPort(), fNameLookup), LOCAL_MANUAL);
        }
    }

    if (mapArgs.count("-reservebalance")) // ppcoin: reserve balance amount
    {
        int64 nReserveBalance = 0;
        if (!ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        {
            InitError(_("Invalid amount for -reservebalance=<amount>"));
            return false;
        }
    }

    if (mapArgs.count("-checkpointkey")) // ppcoin: checkpoint master priv key
    {
        if (!Checkpoints::SetCheckpointPrivKey(GetArg("-checkpointkey", "")))
            InitError(_("Unable to sign checkpoint, wrong checkpointkey?\n"));
    }

    BOOST_FOREACH(string strDest, mapMultiArgs["-seednode"])
        AddOneShot(strDest);

    // ********************************************************* Step 7: load blockchain

    if (!bitdb.Open(GetDataDir()))
    {
        string msg = strprintf(_("Error initializing database environment %s!"
                                 " To recover, BACKUP THAT DIRECTORY, then remove"
                                 " everything from it except for wallet.dat."), strDataDir.c_str());
        return InitError(msg);
    }

	// by Simone: start RPC server before loading the blockchain
	if (fServer)
		NewThread(ThreadRPCServer, NULL);

    if (GetBoolArg("-loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.blkDb->LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

	// by Simone: put this here, otherwise the rule file doesn't load in testnet !
    if (fTestNet)
    {
        pchMessageStart[0] = 0xcd;
        pchMessageStart[1] = 0xf1;
        pchMessageStart[2] = 0xc0;
        pchMessageStart[3] = 0xef;
	}

	// by Simone: load rules here, exit on failure
    uiInterface.InitMessage(_("Loading PALADIN rules..."));
	CDiskRules rules;
	CRulesDB rdb;
	if (!rdb.Read(rules))
		return InitError(_("Error loading PALADIN rules"));

    uiInterface.InitMessage(_("Loading block index..."));
    printf("Loading block index...\n");
    nStart = GetTimeMillis();
	if (!LoadBlockIndex())
		return InitError(_("Error loading blkindex.dat"));

	// by Simone: set generic rules once immediately after loading the index
	CRules::parseGenericRules(nBestHeight);

    // as LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill bitcoin-qt during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown)
    {
        printf("Shutdown requested. Exiting.\n");
        return false;
    }
    printf(" block index %15" PRI64d "ms\n", GetTimeMillis() - nStart);

    if (GetBoolArg("-printblockindex") || GetBoolArg("-printblocktree"))
    {
        PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-printblock"))
    {
        string strMatch = mapArgs["-printblock"];
        int nFound = 0;
        for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0)
            {
                CBlockIndex* pindex = (*mi).second;
                CBlock block;
                block.ReadFromDisk(pindex);
                block.BuildMerkleTree();
                block.print();
                printf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            printf("No blocks matching %s were found\n", strMatch.c_str());
        return false;
    }

    // ********************************************************* Step 8: load wallet

    // needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    if (GetBoolArg("-zapwallettxes", false))
    {
      //debug
      printf("running zapwallettxes from startup.\n");

        uiInterface.InitMessage(_("Zapping all transactions from wallet..."));
        printf("Zapping all transactions from wallet...\n");

        pwalletMain = new CWallet("wallet.dat");
        DBErrors nZapWalletRet = pwalletMain->ZapWalletTx(vWtx);
        if (nZapWalletRet != DB_LOAD_OK) {
            uiInterface.InitMessage(_("Error loading wallet.dat: Wallet corrupted"));
            printf("Error loading wallet.dat: Wallet corrupted\n");
            return false;
        }

        delete pwalletMain;
        pwalletMain = NULL;
    }

    uiInterface.InitMessage(_("Loading wallet..."));
    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun = true;
    pwalletMain = new CWallet("wallet.dat");
    DBErrors nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT)
            strErrors << _("Error loading wallet.dat: Wallet corrupted") << "\n";
        else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
        {
            string msg(_("Warning: error reading wallet.dat! All keys read correctly, but transaction data"
                         " or address book entries might be missing or incorrect."));
            uiInterface.ThreadSafeMessageBox(msg, _("LitecoinPlus"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
        }
        else if (nLoadWalletRet == DB_TOO_NEW)
            strErrors << _("Error loading wallet.dat: Wallet requires newer version of LitecoinPlus") << "\n";
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            strErrors << _("Wallet needed to be rewritten: restart LitecoinPlus to complete") << "\n";
            printf("%s", strErrors.str().c_str());
            return InitError(strErrors.str());
        }
        else
            strErrors << _("Error loading wallet.dat") << "\n";
    }

    if (GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            printf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = CLIENT_VERSION;
            pwalletMain->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            printf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < pwalletMain->GetVersion())
            strErrors << _("Cannot downgrade wallet") << "\n";
        pwalletMain->SetMaxVersion(nMaxVersion);
    }

    if (fFirstRun)
    {
        // Create new keyUser and set as default key
        RandAddSeedPerfmon();

        CPubKey newDefaultKey;
        if (!pwalletMain->GetKeyFromPool(newDefaultKey, false))
            strErrors << _("Cannot initialize keypool") << "\n";
        pwalletMain->SetDefaultKey(newDefaultKey);
        if (!pwalletMain->SetAddressBookName(pwalletMain->vchDefaultKey.GetID(), ""))
            strErrors << _("Cannot write default address") << "\n";
    }

    printf("%s", strErrors.str().c_str());
    printf(" wallet      %15" PRI64d "ms\n", GetTimeMillis() - nStart);

    RegisterWallet(pwalletMain);

    CBlockIndex *pindexRescan = pindexBest;
    if (GetBoolArg("-rescan"))
	{
        pindexRescan = pindexGenesisBlock;
	}
    if (pindexBest != pindexRescan && pindexBest && pindexRescan && pindexBest->nHeight > pindexRescan->nHeight)
    {
        uiInterface.InitMessage(_("Rescanning..."));
        printf("Rescanning last %i blocks (from block %i)...\n", pindexBest->nHeight - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        pwalletMain->ScanForWalletTransactions(pindexRescan, true);
        printf(" rescan      %15" PRI64d "ms\n", GetTimeMillis() - nStart);
        nWalletDBUpdated++;

        // Restore wallet transaction metadata after -zapwallettxes
        if (GetBoolArg("-zapwallettxes", false))
        {
          BOOST_FOREACH(const CWalletTx& wtxOld, vWtx)
          {
            uint256 hash = wtxOld.GetHash();
            std::map<uint256, CWalletTx>::iterator mi = pwalletMain->mapWallet.find(hash);
            if (mi != pwalletMain->mapWallet.end())
            {
              const CWalletTx* copyFrom = &wtxOld;
              CWalletTx* copyTo = &mi->second;
              copyTo->mapValue = copyFrom->mapValue;
              copyTo->vOrderForm = copyFrom->vOrderForm;
              copyTo->nTimeReceived = copyFrom->nTimeReceived;
              copyTo->nTimeSmart = copyFrom->nTimeSmart;
              copyTo->fFromMe = copyFrom->fFromMe;
              copyTo->strFromAccount = copyFrom->strFromAccount;
              copyTo->nOrderPos = copyFrom->nOrderPos;
              copyTo->WriteToDisk();
            }
          }
        }
    }

    // ********************************************************* Step 9: import blocks

    if (mapArgs.count("-loadblock"))
    {
        uiInterface.InitMessage(_("Importing blockchain data file."));

        BOOST_FOREACH(string strFile, mapMultiArgs["-loadblock"])
        {
            FILE *file = fopen(strFile.c_str(), "rb");
            if (file)
                LoadExternalBlockFile(file);
        }
    }

    filesystem::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            filesystem::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        }
    }

    // ********************************************************* Step 10: load peers

    uiInterface.InitMessage(_("Loading addresses..."));
    printf("Loading addresses...\n");
    nStart = GetTimeMillis();

    {
        CAddrDB adb;
        if (!adb.Read(addrman))
            printf("Invalid or missing peers.dat; recreating\n");
    }

    printf("Loaded %i addresses from peers.dat  %" PRI64d "ms\n",
           addrman.size(), GetTimeMillis() - nStart);

    // ********************************************************* Step 11: start node

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    //// debug print
    printf("mapBlockIndex.size() = %" PRIszu "\n",   mapBlockIndex.size());
    printf("nBestHeight = %d\n",            nBestHeight);
    printf("setKeyPool.size() = %" PRIszu "\n",      pwalletMain->setKeyPool.size());
    printf("mapWallet.size() = %" PRIszu "\n",       pwalletMain->mapWallet.size());
    printf("mapAddressBook.size() = %" PRIszu "\n",  pwalletMain->mapAddressBook.size());

	if (!NewThread(StartNode, NULL))
		InitError(_("Error: could not start node"));

	// by Simone: starting the RPC thread was here, instead we just unleash it
	enableRpcExecution = true;

    // ********************************************************* Step 12: finished

    uiInterface.InitMessage(_("Done loading"));

    printf("Done loading\n");

    if (!strErrors.str().empty())
        return InitError(strErrors.str());

     // Add wallet transactions that aren't already in a block to mapTransactions
    pwalletMain->ReacceptWalletTransactions();

#if !defined(QT_GUI)
    // Loop until process is exit()ed from shutdown() function,
    // called from ThreadRPCServer thread when a "stop" command is received.
    while (1)
        Sleep(5000);
#endif

    return true;
}
