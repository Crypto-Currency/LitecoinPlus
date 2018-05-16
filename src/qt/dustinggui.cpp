#include "dustinggui.h"
#include "ui_dustinggui.h"
#include "util.h"

#include "init.h"
#include "bitcoinunits.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "coincontrol.h"

#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QFileDialog>
#include <QProgressDialog>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QSize>
#include <boost/filesystem.hpp>
#include <string>
#include <QMainWindow>
#include <QDateTime>

#include <QDebug>
using namespace std;

//  ------------------------------------------------------------------------ //
// function:     DustingGui(QWidget *parent)
// what it does: class contructor
// access:       implicit
// return:       nothing
//  ------------------------------------------------------------------------ //
DustingGui::DustingGui(QWidget *parent) : QWidget(parent), ui(new Ui::DustingGui)
{
	// build the UI
	ui->setupUi(this);

	// create and add block list
	createBlockList();
	ui->mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
	ui->mainLayout->addWidget(blocksTable, 3, 0, 1, 5);

	// create and add refresh button
	QPushButton *refreshButton = new QPushButton(tr("&Refresh"));
	connect(refreshButton, SIGNAL(clicked()), this, SLOT(updateBlockList()));
	ui->mainLayout->addWidget(refreshButton, 4, 0);

	// create and add dust button
	QPushButton *dustButton = new QPushButton(tr("&Compact now"));
	connect(refreshButton, SIGNAL(clicked()), this, SLOT(compactBlocks()));
	ui->mainLayout->addWidget(dustButton, 4, 4);



//	ui->mainLayout->addWidget(fileLabel, 0, 0);
//	ui->mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
//	ui->mainLayout->addWidget(textLabel, 1, 0);
//	ui->mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
//	ui->mainLayout->addWidget(directoryLabel, 2, 0);
//	ui->mainLayout->addWidget(directoryComboBox, 2, 1);
//	ui->mainLayout->addWidget(browseButton, 2, 2);
//	ui->mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 2);
//	ui->mainLayout->addWidget(findButton, 4, 2);
//	ui->mainLayout->addWidget(resetButton, 5, 2);

//	updateBlockList();
}

//  ------------------------------------------------------------------------ //
// function:     setModel(WalletModel *model)
// what it does: assign wallet model pointer
// access:       public
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::setModel(WalletModel *model)
{
    this->model = model;
}

//  ------------------------------------------------------------------------ //
// function:     createBlockList()
// what it does: creates the table view for the blocks on screen
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::createBlockList()
{
	blocksTable = new QTableWidget(0, 7);
	blocksTable->setSelectionBehavior(QAbstractItemView::SelectRows);

	QStringList labels;
	labels << tr("Amount") << tr("Date") << tr("Label") << tr("Received with") << tr("Confirmations") << tr("_transaction_id") << tr("_amount_int_64");
	blocksTable->setHorizontalHeaderLabels(labels);

	blocksTable->setColumnWidth(0, 120);
	blocksTable->setColumnWidth(1, 110);
	blocksTable->setColumnWidth(2, 110);
	blocksTable->setColumnWidth(3, 330);
	blocksTable->setColumnWidth(4, 130);
    blocksTable->setColumnHidden(5, true);			// store transacton hash in this column, but dont show it
    blocksTable->setColumnHidden(6, true);			// store int 64 amount in this column, but dont show it

	blocksTable->verticalHeader()->hide();
	blocksTable->setShowGrid(true);

//	connect(filesTable, SIGNAL(cellActivated(int,int)),
//		this, SLOT(openFileOfItem(int,int)));
}

//  ------------------------------------------------------------------------ //
// function:     updateBlockList()
// what it does: creates the table view for the blocks on screen
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::updateBlockList()
{
	// prepare to refresh
    blocksTable->setRowCount(0);
    blocksTable->setEnabled(false);
    blocksTable->setAlternatingRowColors(true);

    int nDisplayUnit = BitcoinUnits::BTC;
    if (model && model->getOptionsModel())
	{
		nDisplayUnit = model->getOptionsModel()->getDisplayUnit();
	}


    //QFlags<Qt::ItemFlag> flgCheckbox=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    //QFlags<Qt::ItemFlag> flgTristate=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;    
    

	// loop own coins    
	map<QString, vector<COutput> > mapCoins;
	model->listCoins(mapCoins);

    BOOST_FOREACH(PAIRTYPE(QString, vector<COutput>) coins, mapCoins)
    {
        QString sWalletAddress = coins.first;
        QString sWalletLabel = "";
        if (model->getAddressTableModel())
		{
            sWalletLabel = model->getAddressTableModel()->labelForAddress(sWalletAddress);
		}
        if (sWalletLabel.length() == 0) {
            sWalletLabel = tr("(no label)");
		}

        int64 nSum = 0;
        double dPrioritySum = 0;
        int nChildren = 0;
        int nInputSum = 0;
        BOOST_FOREACH(const COutput& out, coins.second)
        {

			// create cell objects and associate values
			QTableWidgetItem *amountItem = new QTableWidgetItem();
			amountItem->setFlags(amountItem->flags() ^ Qt::ItemIsEditable);
			QTableWidgetItem *dateItem = new QTableWidgetItem();
			dateItem->setFlags(dateItem->flags() ^ Qt::ItemIsEditable);
			QTableWidgetItem *labelItem = new QTableWidgetItem();
			labelItem->setFlags(labelItem->flags() ^ Qt::ItemIsEditable);
			QTableWidgetItem *addressItem = new QTableWidgetItem();
			addressItem->setFlags(addressItem->flags() ^ Qt::ItemIsEditable);
			QTableWidgetItem *confirmationItem = new QTableWidgetItem();
			confirmationItem->setFlags(confirmationItem->flags() ^ Qt::ItemIsEditable);
			QTableWidgetItem *transactionItem = new QTableWidgetItem();
			QTableWidgetItem *amountInt64Item = new QTableWidgetItem();

            int nInputSize = 148; // 180 if uncompressed public key
            nSum += out.tx->vout[out.i].nValue;
            nChildren++;
                            
            // address
            CTxDestination outputAddress;
            QString sAddress = "";
            if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, outputAddress))
            {
                sAddress = CBitcoinAddress(outputAddress).ToString().c_str();
                
                // if listMode or change => show bitcoin address. In tree mode, address is not shown again for direct wallet address outputs
                //if (!(sAddress == sWalletAddress))
				//{
                    addressItem->setText(sAddress);
				//}
                    
                CPubKey pubkey;
                CKeyID *keyid = boost::get< CKeyID >(&outputAddress);
                if (keyid && model->getPubKey(*keyid, pubkey) && !pubkey.IsCompressed())
                    nInputSize = 180;
            }

            // label
            if (!(sAddress == sWalletAddress)) // change
            {
                // tooltip from where the change comes from
                labelItem->setToolTip(tr("change from %1 (%2)").arg(sWalletLabel).arg(sWalletAddress));
                labelItem->setText(tr("(change)"));
            }
            else
            {
                QString sLabel = "";
                if (model->getAddressTableModel())
                    sLabel = model->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.length() == 0)
                    sLabel = tr("(no label)");
                labelItem->setText(sLabel); 
            }

            // amount
            amountItem->setText(BitcoinUnits::format(nDisplayUnit, out.tx->vout[out.i].nValue));
            amountInt64Item->setText(strPad(QString::number(out.tx->vout[out.i].nValue), 15, " ")); // padding so that sorting works correctly

            // date
            dateItem->setText(QDateTime::fromTime_t(out.tx->GetTxTime()).toUTC().toString("yy-MM-dd hh:mm"));
            
            // confirmations
            confirmationItem->setText(strPad(QString::number(out.nDepth), 8, " "));
            
            // transaction hash
            uint256 txhash = out.tx->GetHash();
            transactionItem->setText(txhash.GetHex().c_str());

			// add row
			int row = blocksTable->rowCount();
			blocksTable->insertRow(row);
			blocksTable->setItem(row, 0, amountItem);
			blocksTable->setItem(row, 1, dateItem);
			blocksTable->setItem(row, 2, labelItem);
			blocksTable->setItem(row, 3, addressItem);
			blocksTable->setItem(row, 4, confirmationItem);
			blocksTable->setItem(row, 5, transactionItem);
			blocksTable->setItem(row, 6, amountInt64Item);
		}
	}
    
    // sort view to default
    sortView(6, Qt::AscendingOrder);
    blocksTable->setEnabled(true);
}

//  ------------------------------------------------------------------------ //
// function:     compactBlocks()
// what it does: execute the compacting and optimization of all blocks as much as possible
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::compactBlocks()
{
}

//  ------------------------------------------------------------------------ //
// function:     strPad(QString s, int nPadLength, QString sPadding)
// what it does: pad helper
// access:       private
// return:       QString
//  ------------------------------------------------------------------------ //
QString DustingGui::strPad(QString s, int nPadLength, QString sPadding)
{
	while (s.length() < nPadLength)
	{
		s = sPadding + s;
	}
    return s;
}

//  ------------------------------------------------------------------------ //
// function:     sortView(int column, Qt::SortOrder order)
// what it does: sort view as desired
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::sortView(int column, Qt::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
    blocksTable->sortItems(column, order);
    blocksTable->horizontalHeader()->setSortIndicator((sortColumn == 5 ? 0 : sortColumn), sortOrder);
}

//  ------------------------------------------------------------------------ //
// function:     resizeEvent(QResizeEvent* event)
// what it does: class contructor
// access:       protected
// return:       nothing
//  ------------------------------------------------------------------------ //
void DustingGui::resizeEvent(QResizeEvent* event)
{
// resize the inner table when necessary
//	int ww = width();
//	filesTable->setColumnWidth(1, ww - 278);
}
