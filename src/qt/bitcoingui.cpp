/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2012
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "wallet.h"
#include "bitcoinrpc.h"
#include "version.h"
#include "skinspage.h"
#include "dustinggui.h"
#include "splash.h"
#include "init.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QStyle>
#include <QSplashScreen>

#include <iostream>

extern CWallet *pwalletMain;
extern int64 nLastCoinStakeSearchInterval;
extern unsigned int nStakeTargetSpacing;
extern BitcoinGUI *guiref;
extern Splash *stwref;

// added by Simone
ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent) {
    
}

ClickableLabel::~ClickableLabel() {}

void ClickableLabel::mousePressEvent(QMouseEvent* event) {
    emit clicked();
}

// by Simone: used for progress around the code
// ATTENTION: be sure to be in the same thread when calling this one, is not like the ui_interface one
void updateBitcoinGUISplashMessage(char *message)
{
	if (guiref) {
		guiref-> splashMessage(_(message), true);
	}
	if (stwref) {
		stwref->setMessage(message);
	}
}

// by Simone: session commit data signal
bool isInSessionHandling = false;
extern ShutdownWindow *sdwRef;
void BitcoinGUI::walletCommitData(QSessionManager& manager)
{
// if we enter in a first phase, request a second one, and do operations there
	if (!manager.isPhase2())
	{
		isInSessionHandling = true;
		OutputDebugStringF("User attempting shutdown, waiting...\n");
		manager.requestPhase2();
		return;
	} else

// second phase, save all data and quit cleanly
	{
		isInSessionHandling = false;
		OutputDebugStringF("Shutdown invoked by session. Shutting down...\n");
		if (manager.allowsInteraction())
		{
			sdwRef->showShutdownWindow();
			QApplication::processEvents();
			Sleep(100);
			QApplication::processEvents();

		}
		Shutdown(NULL);
		manager.release();
	}
}

void BitcoinGUI::aboutToQuit()
{
	isInSessionHandling = false;
	OutputDebugStringF("Shutdown invoked by AboutToQuit(). Shutting down...\n");
	Shutdown(NULL);
}

// by Simone: expose loadSkin call
void BitcoinGUI::loadSkin()
{
	skinsPage->loadSkin();
}

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0)
{
    resize(850, 550);
    setWindowTitle(tr("LitecoinPlus - Wallet  ")+QString::fromStdString(CLIENT_BUILD));
//  setStyleSheet("");
//    statusBar()->setStyleSheet("QToolTip {background-color:rgb(255,233,142); color:black; border: 2px solid grey;}");
#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/bitcoin"));
    setWindowIcon(QIcon(":icons/bitcoin"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();
menuBar()->setNativeMenuBar(false);// menubar on form instead

    // Create the toolbars
    createToolBars();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();

    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    transactionsPage->setLayout(vbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

	skinsPage = new SkinsPage(this);
	dustingPage = new DustingGui(this);

    connect(skinsPage, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));
    connect(skinsPage, SIGNAL(information(QString,QString)), this, SLOT(information(QString,QString)));
    connect(skinsPage, SIGNAL(status(QString)), this, SLOT(status(QString)));

    centralWidget = new QStackedWidget(this);
    centralWidget->addWidget(overviewPage);
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(receiveCoinsPage);
    centralWidget->addWidget(sendCoinsPage);
	centralWidget->addWidget(skinsPage);
	centralWidget->addWidget(dustingPage);
    setCentralWidget(centralWidget);

    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setMinimumWidth(102);
    frameBlocks->setMaximumWidth(102);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelOnlineIcon = new ClickableLabel();
    labelEncryptionIcon = new QLabel();
    labelMintingIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelOnlineIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelEncryptionIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelMintingIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Set online/offline pixmap
    labelOnlineIcon->setPixmap(QIcon(":/icons/net_online").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    labelOnlineIcon->setEnabled(true);
	connect(labelOnlineIcon, SIGNAL(clicked()), this, SLOT(labelOnlineClicked()));
	onlineLabelMsg(netOffline);

    // Set minting pixmap
    labelMintingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    labelMintingIcon->setEnabled(false);

    // Add timer to update minting icon
    QTimer *timerMintingIcon = new QTimer(labelMintingIcon);
    timerMintingIcon->start(MODEL_UPDATE_DELAY);
    connect(timerMintingIcon, SIGNAL(timeout()), this, SLOT(updateMintingIcon()));
    // Add timer to update minting weights
    QTimer *timerMintingWeights = new QTimer(labelMintingIcon);
    timerMintingWeights->start(30 * 1000);
    connect(timerMintingWeights, SIGNAL(timeout()), this, SLOT(updateMintingWeights()));
    // Set initial values for user and network weights
    nWeight=0;
    nNetworkWeight = 0;

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    syncIconMovie = new QMovie(":/movies/update_spinner", "mng", this);
	// this->setStyleSheet("background-color: #ceffee;");

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // Clicking on "Verify Message" in the address book sends you to the verify message tab
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page sends you to the sign message tab
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

	connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)), this, SLOT(walletCommitData(QSessionManager&)), Qt::DirectConnection);

#ifdef WIN32
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()), Qt::DirectConnection);
#endif

    connect(openConfigAction, SIGNAL(triggered()), this, SLOT(openConfig()));

    gotoOverviewPage();
}

BitcoinGUI::~BitcoinGUI()
{
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
#endif
}

// by Simone: label to control online/offline status
void BitcoinGUI::onlineLabelMsg(bool sts)
{
	QString tooltip;
	if (sts)
	{
		tooltip = tr("The wallet is now OFFLINE");
		tooltip += "<br><br>";
		tooltip += "Click this icon to bring it online.";
	}
	else
	{
		tooltip = tr("The wallet is now ONLINE");
		tooltip += "<br><br>";
		tooltip += "Click this icon to bring it offline.";
	}
	labelOnlineIcon->setToolTip(tooltip);
}

void BitcoinGUI::labelOnlineClicked()
{
	// ask before going offline
	if (!netOffline)
	{
		QString strMessage = tr("The wallet will now go OFFLINE, meaning that syncing and staking will stop. Are you sure ?");
		QMessageBox::StandardButton retval = QMessageBox::question(
		      this, tr("Confirm offline action"), strMessage,
		      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
		if (retval == QMessageBox::Cancel)
		{
			return;
		}
	}

	// toggle the net status and save result
	bool res = setOnlineStatus(netOffline);

	// put the proper image & stuff
	if (res)
	{
		labelOnlineIcon->setPixmap(QIcon(":/icons/net_offline").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
		labelBlocksIcon->hide();
        overviewPage->showOutOfSyncWarning(true);
	}
	else
	{
		labelOnlineIcon->setPixmap(QIcon(":/icons/net_online").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        progressBarLabel->setText("");
        progressBarLabel->setVisible(false);
        labelBlocksIcon->setMovie(syncIconMovie);
		labelBlocksIcon->show();
        syncIconMovie->start();
	}
	onlineLabelMsg(res);
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setToolTip(tr("Show general overview of wallet"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send coins"), this);
    sendCoinsAction->setToolTip(tr("Send coins to a LitecoinPlus address"));
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive coins"), this);
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);


    skinsPageAction = new QAction(QIcon(":/icons/gears"), tr("&Themes"), this);
    skinsPageAction->setToolTip(tr("Change the look of your wallet"));
    skinsPageAction->setCheckable(true);
    skinsPageAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    tabGroup->addAction(skinsPageAction);

    openConfigAction = new QAction(QIcon(":/icons/edit"), tr("Open Wallet &Configuration File"), this);
    openConfigAction->setStatusTip(tr("Open wallet configuration file"));

    dustingPageAction = new QAction(QIcon(":/icons/dusting"), tr("&Dusting"), this);
    dustingPageAction->setToolTip(tr("Monitor the dusting of your wallet"));
    dustingPageAction->setCheckable(true);
    dustingPageAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    tabGroup->addAction(dustingPageAction);

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(skinsPageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(skinsPageAction, SIGNAL(triggered()), this, SLOT(gotoSkinsPage()));


    connect(dustingPageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(dustingPageAction, SIGNAL(triggered()), this, SLOT(gotoDustingPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/bitcoin"), tr("&About LitecoinPlus"), this);
    aboutAction->setToolTip(tr("Show information about LitecoinPlus"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for LitecoinPlus"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/bitcoin"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt wallet"));

    unlockWalletStakeAction = new QAction(QIcon(":/icons/lock_open"), tr("&Unlock To Stake..."), this);
    //unlockWalletStakeAction->setStatusTip(tr("Unlock wallet for Staking only"));

    checkWalletAction = new QAction(QIcon(":/icons/inspect"), tr("&Check Wallet..."), this);
    //checkWalletAction->setStatusTip(tr("Check wallet integrity and report findings"));

    repairWalletAction = new QAction(QIcon(":/icons/repair"), tr("&Repair Wallet..."), this);
    //repairWalletAction->setStatusTip(tr("Fix wallet integrity and remove orphans"));

    zapWalletAction = new QAction(QIcon(":/icons/repair"), tr("&Zap Wallet..."), this);
    //zapWalletAction->setStatusTip(tr("Zaps txes from wallet then rescans (this is slow)..."));

    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setToolTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for wallet encryption"));
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);

    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));

    connect(unlockWalletStakeAction, SIGNAL(triggered()), this, SLOT(unlockWalletStake()));

    connect(checkWalletAction, SIGNAL(triggered()), this, SLOT(checkWallet()));
    connect(repairWalletAction, SIGNAL(triggered()), this, SLOT(repairWallet()));
    connect(zapWalletAction, SIGNAL(triggered()), this, SLOT(zapWallet()));

    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(openConfigAction);
    file->addSeparator();
    file->addAction(exportAction);
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(optionsAction);

    QMenu *wallet = appMenuBar->addMenu(tr("&Wallet"));
    wallet->addAction(backupWalletAction);
    wallet->addSeparator();
    wallet->addAction(encryptWalletAction);
    wallet->addAction(changePassphraseAction);
    wallet->addAction(unlockWalletStakeAction);
    wallet->addSeparator();
    wallet->addAction(checkWalletAction);
    wallet->addAction(repairWalletAction);
    wallet->addAction(zapWalletAction);
    wallet->addSeparator();
    wallet->addAction(signMessageAction);
    wallet->addAction(verifyMessageAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(overviewAction);
    toolbar->addAction(sendCoinsAction);
    toolbar->addAction(receiveCoinsAction);
    toolbar->addAction(historyAction);
    toolbar->addAction(addressBookAction);

    QToolBar *toolbar2 = addToolBar(tr("Actions toolbar"));
    toolbar2->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolbar2->addAction(skinsPageAction);
    toolbar2->addAction(dustingPageAction);
}

void BitcoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Replace some strings and icons, when using the testnet
        if(clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/bitcoin_testnet"));
            setWindowIcon(QIcon(":icons/bitcoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("LitecoinPlus client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
                toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        receiveCoinsPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);
        dustingPage->setModel(walletModel);

        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
    }
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("LitecoinPlus client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    trayIconMenu = dockIconHandler->dockMenu();
    dockIconHandler->setMainWindow((QMainWindow *)this);
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(openConfigAction);
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon);
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to LitecoinPlus network", "", count));
}

void BitcoinGUI::setNumBlocks(int count, int nTotalBlocks)
{
    // don't show / hide progress bar and its label if we have no connection to the network
    if (!clientModel || clientModel->getNumConnections() == 0)
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        return;
    }

    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();
    QString tooltip;

	// by Simone: since this event is now fixed, let's prevent from appearing for just 1 block
    if ((nTotalBlocks - count) > 5)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (strStatusBarWarnings.isEmpty())
        {
            progressBarLabel->setText(tr("Synchronizing with network..."));
            progressBarLabel->setVisible(true);
            progressBar->setFormat(tr("~%n block(s) remaining", "", nRemainingBlocks));
            progressBar->setMaximum(nTotalBlocks);
            progressBar->setValue(count);
            progressBar->setVisible(true);
        }

        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    }
    else
    {
        if (strStatusBarWarnings.isEmpty())
            progressBarLabel->setVisible(false);

        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text and hide progress bar, when we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
    {
        progressBarLabel->setText(strStatusBarWarnings);
        progressBarLabel->setVisible(true);
        progressBar->setVisible(false);
    }

	tooltip = tr("Current difficulty is %1.").arg(clientModel->GetDifficulty()) + QString("<br>") + tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if(secs <= 0)
    {
        // Fully up to date. Leave text empty.
    }
    else if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60 && count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        overviewPage->showOutOfSyncWarning(false);
    }
    else
    {
        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        labelBlocksIcon->setMovie(syncIconMovie);
        syncIconMovie->start();

        overviewPage->showOutOfSyncWarning(true);
    }

    if(!text.isEmpty())
    {
        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void BitcoinGUI::information(const QString &title, const QString &message)
{
    // Report information from network/worker thread
    QMessageBox::information(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
}

void BitcoinGUI::status(const QString &message)
{
	bool vis = true;
	if (message == "") {
		vis = false;
	}
	progressBarLabel->setText(message);
	progressBarLabel->setVisible(vis);
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
// by Simone: during session save data stuff, we do not exit
	if (isInSessionHandling)
	{
		return;
	}
	
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("This transaction is over the size limit.  You can still send it for a fee of %1, "
          "which goes to the nodes that process your transaction and helps to support the network.  "
          "Do you want to pay the fee?").arg(
                BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    if(!walletModel || !clientModel)
        return;
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    if(!clientModel->inInitialBlockDownload())
    {
        // On new transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                        .data().toString();
        QIcon icon = qvariant_cast<QIcon>(ttm->index(start,
                            TransactionTableModel::ToAddress, parent)
                        .data(Qt::DecorationRole));

        notificator->notify(Notificator::Information,
                            (amount)<0 ? tr("Sent transaction") :
                                         tr("Incoming transaction"),
                              tr("Date: %1\n"
                                 "Amount: %2\n"
                                 "Type: %3\n"
                                 "Address: %4\n")
                              .arg(date)
                              .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                              .arg(type)
                              .arg(address), icon);
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(overviewPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoSkinsPage()
{
    skinsPageAction->setChecked(true);
    centralWidget->setCurrentWidget(skinsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoDustingPage()
{
    dustingPageAction->setChecked(true);
    centralWidget->setCurrentWidget(dustingPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveCoinsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(sendCoinsPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            if (sendCoinsPage->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            gotoSendCoinsPage();
        else
            notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid LitecoinPlus address or malformed URI parameters."));
    }

    event->acceptProposedAction();
}

void BitcoinGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (sendCoinsPage->handleURI(strURI))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
    }
    else
        notificator->notify(Notificator::Warning, tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid LitecoinPlus address or malformed URI parameters."));
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported

        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        if(pwalletMain->fWalletUnlockMintOnly)
        {
          labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for Staking only."));
          unlockWalletStakeAction->setEnabled(false);
        } 
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        if(pwalletMain->fWalletUnlockMintOnly)
        {
          labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for Staking only."));
          unlockWalletStakeAction->setEnabled(false);
        } 
//        encryptWalletAction->setChecked(true);
//        changePassphraseAction->setEnabled(true);
//        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void BitcoinGUI::encryptWallet(bool status)
{
	if (!walletModel)
		return;

	// by Simone: ASK first.......... !
	QString strMessage = tr("The wallet will now be set offline and the wallet.dat file encrypted. The operation may require some time, after which the wallet will quit and will need to be restarted. <b>Are you sure you want to do it now</b> ?");
	QMessageBox::StandardButton retval = QMessageBox::question(
	      this, tr("Encrypt Wallet warning"), strMessage,
	      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
	if (retval == QMessageBox::Cancel)
	{
		return;
	}

	// by Simone: set wallet offline first
	bool res = setOnlineStatus(false);
	if (!res) {
       error(tr("Set Offline Error"),
               tr("Error, cannot bring the wallet offline at this time. Try again in a few seconds."), true);
		return;
	}
	status = true;

	// let's encrypt it !
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

	// if we arrive here, the user has pressed cancel, let's go back online
	information(tr("Cancelled"),
		   tr("The encryption was cancelled, the wallet is still unencrypted. The software will go back to online status now."));
	res = setOnlineStatus(true);

   // setEncryptionStatus(walletModel->getEncryptionStatus());

	// by Simone: display a message and QUIT the software, before some big damage is done by clicking around !
//	QMessageBox::warning(this, tr("Encrypt Wallet Finished."), "<b>" + tr("The wallet will now exit.") + "</b><br><br>" + tr("Please restart your wallet for changes to take effect."));
//	qApp->quit();
}

void BitcoinGUI::unlockWalletStake()
{
  if(!walletModel)
    return;

  // Unlock wallet when requested by user
  if(walletModel->getEncryptionStatus() == WalletModel::Locked)
  {
    AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
    dlg.setModel(walletModel);
    dlg.exec();

    // Only show message if unlock is sucessfull.
    if(walletModel->getEncryptionStatus() == WalletModel::Unlocked)
    {
      pwalletMain->fWalletUnlockMintOnly=true;      
      information(tr("Unlock Wallet Information"),
        tr("Wallet has been unlocked. \n"
          "Proof of Stake has started.\n"));
    }
  }
}

void BitcoinGUI::checkWallet()
{

    int nMismatchSpent;
    int64 nBalanceInQuestion;
    int nOrphansFound;

    if(!walletModel)
        return;

    // Check the wallet as requested by user
    walletModel->checkWallet(nMismatchSpent, nBalanceInQuestion, nOrphansFound);

    if (nMismatchSpent == 0 && nOrphansFound == 0)
        information(tr("Check Wallet Information"),
                tr("Wallet passed integrity test!\n"
                   "Nothing found to fix."));
  else
       error(tr("Check Wallet Information"),
               tr("Wallet failed integrity test!\n\n"
                  "Mismatched coin(s) found: %1.\n"
                  "Amount in question: %2.\n"
                  "Orphans found: %3.\n\n"
                  "Please backup wallet and run repair wallet.\n")
                        .arg(nMismatchSpent)
                        .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), nBalanceInQuestion,true))
                        .arg(nOrphansFound),true);
}

void BitcoinGUI::repairWallet()
{
    int nMismatchSpent;
    int64 nBalanceInQuestion;
    int nOrphansFound;

    if(!walletModel)
        return;

    // Repair the wallet as requested by user
    walletModel->repairWallet(nMismatchSpent, nBalanceInQuestion, nOrphansFound);

    if (nMismatchSpent == 0 && nOrphansFound == 0)
       information(tr("Repair Wallet Information"),
               tr("Wallet passed integrity test!\n"
                  "Nothing found to fix."));
    else
       error(tr("Repair Wallet Information"),
               tr("Wallet failed integrity test and has been repaired!\n"
                  "Mismatched coin(s) found: %1\n"
                  "Amount affected by repair: %2\n"
                  "Orphans removed: %3\n")
                        .arg(nMismatchSpent)
                        .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), nBalanceInQuestion,true))
                        .arg(nOrphansFound),true);
}

void BitcoinGUI::zapWallet()
{
	if (!walletModel)
		return;

	// by Simone: ASK first.......... !
	QString strMessage = tr("The wallet will be set offline and the operation will require a LONG time (more than a few minutes). <b>Are you sure you want to do it now</b> ?");
	QMessageBox::StandardButton retval = QMessageBox::question(
	      this, tr("ZAP Wallet warning"), strMessage,
	      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
	if (retval == QMessageBox::Cancel)
	{
		return;
	}

	// by Simone: set wallet offline first
	bool res = setOnlineStatus(false);
	if (!res) {
       error(tr("Set Offline Error"),
               tr("Error, cannot bring the wallet offline at this time. Try again in a few seconds."), true);
		return;
	}

	//debug
	printf("running zapwallettxes from qt menu.\n");

	// by Simone: bring up the startup window
	stwref->setMessage("");
	stwref->systemOnTop();
	stwref->showSplash();

	// bring up splash screen
	/*QSplashScreen splash(QPixmap(":/images/splash"), Qt::WindowStaysOnTopHint);
	splash.setEnabled(false);
	splash.show();
	splash.setAutoFillBackground(true);
	splashref = &splash;*/

	// Zap the wallet as requested by user
	// 1= save meta data
	// 2=remove meta data needed to restore wallet transaction meta data after -zapwallettxes
	std::vector<CWalletTx> vWtx;


	splashMessage(_("Zapping all transactions from wallet..."));
	printf("Zapping all transactions from wallet...\n");

	// clear tables

	pwalletMain = new CWallet("wallet.dat");
	DBErrors nZapWalletRet = pwalletMain->ZapWalletTx(vWtx);
	if (nZapWalletRet != DB_LOAD_OK)
	{
		splashMessage(_("Error loading wallet.dat: Wallet corrupted"));
		printf("Error loading wallet.dat: Wallet corrupted\n");
		stwref->hide();
		//if (splashref)
		//	splash.close();
	    return;
	}

	delete pwalletMain;
	pwalletMain = NULL;

	splashMessage(_("Loading wallet..."));
	printf("Loading wallet...\n");

	int64 nStart = GetTimeMillis();
	bool fFirstRun = true;
	pwalletMain = new CWallet("wallet.dat");


	DBErrors nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
	if (nLoadWalletRet != DB_LOAD_OK)
	{
		if (nLoadWalletRet == DB_CORRUPT)
		{
			splashMessage(_("Error loading wallet.dat: Wallet corrupted"));
			printf("Error loading wallet.dat: Wallet corrupted\n");
		}
		else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
		{
			setStatusTip(tr("Warning: error reading wallet.dat! All keys read correctly, but transaction data or address book entries might be missing or incorrect."));
			printf("Warning: error reading wallet.dat! All keys read correctly, but transaction data or address book entries might be missing or incorrect.\n");
		}
		else if (nLoadWalletRet == DB_TOO_NEW)
		{
			setStatusTip(tr("Error loading wallet.dat: Wallet requires newer version of BitBar"));
			printf("Error loading wallet.dat: Wallet requires newer version of BitBar\n");
		}
		else if (nLoadWalletRet == DB_NEED_REWRITE)
		{
			setStatusTip(tr("Wallet needed to be rewritten: restart BitBar to complete"));
			printf("Wallet needed to be rewritten: restart BitBar to complete\n");
			stwref->hide();
			//if (splashref)
			//	splash.close();
			return;
		}
		else
		{
			setStatusTip(tr("Error loading wallet.dat"));
			printf("Error loading wallet.dat\n");
		} 
	}
  
	splashMessage(_("Wallet loaded..."));
	printf(" zap wallet  load     %15" PRI64d "ms\n", GetTimeMillis() - nStart);

	splashMessage(_("Loaded lables..."));
	printf(" zap wallet  loading metadata\n");

	// Restore wallet transaction metadata after -zapwallettxes=1
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
	splashMessage(_("scanning for transactions..."));
	printf(" zap wallet  scanning for transactions\n");

	pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true, clientModel->getNumBlocks());
	pwalletMain->ReacceptWalletTransactions();
	splashMessage(_("Please restart your wallet."));
	printf(" zap wallet  done - please restart wallet.\n");

	// by Simone: close startup window
	stwref->hide();
	//if (splashref)
	//	splash.close();

	// by Simone: display a message and QUIT the software, before some big damage is done by clicking around !
	QMessageBox::warning(this, tr("Zap Wallet Finished."), "<b>" + tr("The wallet will now exit.") + "</b><br><br>" + tr("Please restart your wallet for changes to take effect."));
	qApp->quit();
}

void BitcoinGUI::splashMessage(const std::string &message, bool quickSleep)
{
	if (stwref)
	{
		stwref->setMessage(message.c_str());
		if (quickSleep) {
			Sleep(50);
		} else {
			Sleep(500);
		}
	}

/*  if(splashref)
  {
    splashref->showMessage(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(0xfce2cc));
    QApplication::instance()->processEvents();
	if (quickSleep) {
		Sleep(50);
	} else {
		Sleep(500);
	}
  }*/
}

void BitcoinGUI::backupWallet()
{
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the wallet data to the new location."));
        }
    }
}

void BitcoinGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
	if (dlg.result() == QDialog::Accepted)
	{
		walletModel->setWalletLocked(true);
		setEncryptionStatus(walletModel->getEncryptionStatus());
	}
}

void BitcoinGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcoinGUI::updateMintingIcon()
{
      if (!walletModel)
         return;

        labelMintingIcon->setEnabled(false);
//      labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));

      if (!clientModel->getNumConnections())
         labelMintingIcon->setToolTip(tr("Not staking because wallet is offline"));
      else if (clientModel->inInitialBlockDownload() ||
               clientModel->getNumBlocks() < clientModel->getNumBlocksOfPeers())
         labelMintingIcon->setToolTip(tr("Not staking because wallet is syncing"));
      else if(walletModel->getEncryptionStatus() == WalletModel::Locked)
         labelMintingIcon->setToolTip(tr("Not staking because wallet is locked"));
      else
      {
         uint64 nMinWeight = 0, nMaxWeight = 0, nWeight = 0;

         walletModel->getStakeWeight(nMinWeight,nMaxWeight,nWeight);
         if (!nWeight)
            labelMintingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
          else
          {
            uint64 nNetworkWeight = clientModel->getPosKernalPS();
            int nEstimateTime = clientModel->getStakeTargetSpacing() * 10 * nNetworkWeight / nWeight;
            QString text;
            if (nEstimateTime < 60)
               text = tr("%n second(s)", "", nEstimateTime);
            else if (nEstimateTime < 60*60)
               text = tr("%n minute(s)", "", nEstimateTime/60);
            else if (nEstimateTime < 24*60*60)
               text = tr("%n hour(s)", "", nEstimateTime/(60*60));
            else
               text = tr("%n day(s)", "", nEstimateTime/(60*60*24));

        labelMintingIcon->setEnabled(true);
//            labelStakingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelMintingIcon->setToolTip(tr("Staking.\n Your weight is %1\n Network weight is %2\n You have 50\% chance of producing a stake within %3").arg(nWeight).arg(nNetworkWeight).arg(text));
          }
       }



}

void BitcoinGUI::updateMintingWeights()
{
    // Only update if we have the network's current number of blocks, or weight(s) are zero (fixes lagging GUI)
    if ((clientModel && clientModel->getNumBlocks() == clientModel->getNumBlocksOfPeers()) || !nWeight || !nNetworkWeight)
    {
        nWeight = 0;

        if (pwalletMain)
            pwalletMain->GetStakeWeight(*pwalletMain, nMinMax, nMinMax, nWeight);

        nNetworkWeight = GetPoSKernelPS();
    }
}

void BitcoinGUI::openConfig()
{
  boost::filesystem::path pathConfig = GetConfigFile();
  /* Open LitecoinPlus.conf with the associated application */
  if (boost::filesystem::exists(pathConfig))
    QDesktopServices::openUrl(QUrl::fromLocalFile(pathConfig.string().c_str()));
}
