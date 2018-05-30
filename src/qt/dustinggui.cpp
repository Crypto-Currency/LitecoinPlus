#include "dustinggui.h"
#include "ui_dustinggui.h"
#include "util.h"

#include "init.h"
#include "bitcoinunits.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "coincontrol.h"
#include "addressbookpage.h"

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

CCoinControl* DustingGui::coinControl = new CCoinControl();


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
	ui->mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
	ui->dustAddress->setReadOnly(true);

	// create and add block list
	createBlockList();
	ui->mainLayout->addWidget(blocksTable, 3, 0, 1, 5);

	// some general info label
	infoLabel = new QLabel();
	ui->mainLayout->addWidget(infoLabel, 4, 0, 1, 5);

	// create and add refresh button
	QPushButton *refreshButton = new QPushButton(tr("&Refresh"));
	connect(refreshButton, SIGNAL(clicked()), this, SLOT(updateBlockList()));
	ui->mainLayout->addWidget(refreshButton, 5, 0);

	// create and add dust button
	QPushButton *dustButton = new QPushButton(tr("&Dust now"));
	connect(dustButton, SIGNAL(clicked()), this, SLOT(compactBlocks()));
	ui->mainLayout->addWidget(dustButton, 5, 4);

	// load settings
	minimumBlockAmount = 16;
	blockDivisor = 50;



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
	blocksTable = new QTableWidget(0, 9);
	blocksTable->setSelectionBehavior(QAbstractItemView::SelectRows);

	QStringList labels;
	labels << tr("Amount") << tr("Date") << tr("Label") << tr("Received with") << tr("Confirmations") << tr("_transaction_id") << tr("_amount_int_64") << tr("_vout_index") << ("_input_size");
	blocksTable->setHorizontalHeaderLabels(labels);

	blocksTable->setColumnWidth(COLUMN_AMOUNT, 120);
	blocksTable->setColumnWidth(COLUMN_DATE, 110);
	blocksTable->setColumnWidth(COLUMN_LABEL, 110);
	blocksTable->setColumnWidth(COLUMN_ADDRESS, 330);
	blocksTable->setColumnWidth(COLUMN_CONFIRMATIONS, 130);
    blocksTable->setColumnHidden(COLUMN_TXHASH, true);					// store transacton hash in this column, but dont show it
    blocksTable->setColumnHidden(COLUMN_AMOUNT_INT64, true);			// store int 64 amount in this column, but dont show it
    blocksTable->setColumnHidden(COLUMN_VOUT_INDEX, true);				// store vout index
    blocksTable->setColumnHidden(COLUMN_INPUT_SIZE, true);				// store input size

	blocksTable->verticalHeader()->hide();
	blocksTable->setShowGrid(true);
	blocksTable->setSelectionMode(QAbstractItemView::MultiSelection);

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
        int nChildren = 0;
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
			QTableWidgetItem *voutIndex = new QTableWidgetItem();
			QTableWidgetItem *inputSize = new QTableWidgetItem();

            int nInputSize = 148; // 180 if uncompressed public key
            nSum += out.tx->vout[out.i].nValue;
            nChildren++;
                            
            // address
            CTxDestination outputAddress;
            QString sAddress = "";
            if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, outputAddress))
            {
                sAddress = CBitcoinAddress(outputAddress).ToString().c_str();
				addressItem->setText(sAddress);
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
				if (ui->dustAddress->text() == "") {
					ui->dustAddress->setText(sAddress);
				}
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

            // vout index
            voutIndex->setText(QString::number(out.i));

			// input size
            inputSize->setText(QString::number(nInputSize));

			// add row
			int row = blocksTable->rowCount();
			blocksTable->insertRow(row);
			blocksTable->setItem(row, COLUMN_AMOUNT, amountItem);
			blocksTable->setItem(row, COLUMN_DATE, dateItem);
			blocksTable->setItem(row, COLUMN_LABEL, labelItem);
			blocksTable->setItem(row, COLUMN_ADDRESS, addressItem);
			blocksTable->setItem(row, COLUMN_CONFIRMATIONS, confirmationItem);
			blocksTable->setItem(row, COLUMN_TXHASH, transactionItem);
			blocksTable->setItem(row, COLUMN_AMOUNT_INT64, amountInt64Item);
			blocksTable->setItem(row, COLUMN_VOUT_INDEX, voutIndex);
			blocksTable->setItem(row, COLUMN_INPUT_SIZE, inputSize);
		}
	}
    
    // sort view to default
    sortView(6, Qt::AscendingOrder);
    blocksTable->setEnabled(true);

	// put count
	if (blocksTable->rowCount() <= minimumBlockAmount)
	{
		infoLabel->setText(tr("The wallet is clean."));
	}
	else
	{
		infoLabel->setText("<b>" + tr("Found ") + QString::number(blocksTable->rowCount()) + tr(" blocks to compact.") + "</b>");
	}
}

//  ------------------------------------------------------------------------ //
// function:     on_addressBookButton_clicked()
// what it does: execute the select dialog for receive addresses
// access:       event
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::on_addressBookButton_clicked()
{
    if (!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
		ui->dustAddress->setText(dlg.getReturnValue());
    }
}

//  ------------------------------------------------------------------------ //
// function:     compactBlocks()
// what it does: execute the compacting and optimization of all blocks as much as possible
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::compactBlocks()
{
	// check number of blocks
	if (blocksTable->rowCount() <= minimumBlockAmount)
	{
		QMessageBox::information(this, tr("Wallet Dusting"), tr("The wallet is already optimized."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	// first warn the user
	QString strMessage = tr("The wallet will now be dusted. If any block start staking, dust will stop with some error, simply start it again. Enter the passphrase only once, if your wallet is encrypted. <b>Are you sure you want to do it now</b> ?");
	QMessageBox::StandardButton retval = QMessageBox::question(
	      this, tr("Wallet Dusting"), strMessage,
	      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
	if (retval == QMessageBox::Cancel)
	{
		return;
	}

	// refresh the selection after having put it offline
	updateBlockList();
	int nOps = ((blocksTable->rowCount() - minimumBlockAmount) / blockDivisor) + 1;
	int nOdds = (blocksTable->rowCount() - minimumBlockAmount) % blockDivisor;
	if (nOdds == 1)
	{
		nOdds = blockDivisor + 1;
	}
	else if (nOdds == 2)
	{
		nOdds = blockDivisor + 2;
	}
	else if (nOdds == 0)
	{
		nOdds = blockDivisor;
	}
	if (nOdds >= (blocksTable->rowCount() - minimumBlockAmount))
	{
		nOdds = blocksTable->rowCount() - minimumBlockAmount + 2;		// optimize the last piece to the target length
	}

	// unlock the wallet for dusting only once
	WalletModel::UnlockContext ctx(model->requestUnlock());
	if (!ctx.isValid())
	{
	    QMessageBox::warning(this, tr("Send Coins"),
	        tr("Cannot unlock wallet at this time, please try again later."),
	        QMessageBox::Ok, QMessageBox::Ok);
	    return;
	}

	// now, let's select the first batch of items
    QList<SendCoinsRecipient> recipients;
	qint64 selectionSum;
	WalletModel::SendCoinsReturn sendstatus;
	while (nOps > 0) {

		// reset previous selection
		coinControl->SetNull();
		recipients.clear();
		selectionSum = 0;
		for (int i = 0; i < nOdds; i++)
		{

			// prepare this selection
			QTableWidgetItem *itemAmount = blocksTable->item(i, COLUMN_AMOUNT_INT64);
			QTableWidgetItem *itemTx = blocksTable->item(i, COLUMN_TXHASH);
			QTableWidgetItem *itemVout = blocksTable->item(i, COLUMN_VOUT_INDEX);
			COutPoint outpt(uint256(itemTx->text().toStdString()), itemVout->text().toUInt());
			coinControl->Select(outpt);
			selectionSum += itemAmount->text().toUInt();

			// select the row to show something on screen
			blocksTable->selectRow(i);
			QApplication::instance()->processEvents();
			Sleep(50);

			//fprintf(stderr, "%d --> [%ld] %s/%d\n", i, selectionSum, uint256(itemTx->text().toStdString()).ToString().c_str(), itemVout->text().toUInt());
		}
		Sleep(1000);
		for (int i = 0; i < nOdds; i++)
		{
			blocksTable->removeRow(i);
			QApplication::instance()->processEvents();
		}
		blocksTable->clearSelection();
		QApplication::instance()->processEvents();


		// append this selection
		SendCoinsRecipient rcp;
		rcp.amount = selectionSum;
		rcp.amount -= 100;				// this is safe value to not incurr in "not enough for fee" errors, in any case it will be credited back as "change"
		rcp.label = "[DUSTING]";
		rcp.address = ui->dustAddress->text();
		recipients.append(rcp);

		// launch the send coin interface
		sendstatus = model->sendCoins(recipients, DustingGui::coinControl);

		// check the return value
		bool breakCycle = true;
		switch(sendstatus.status)
		{
		case WalletModel::InvalidAddress:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("The recipient address is not valid, please recheck."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::InvalidAmount:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("The amount to pay must be larger than 0."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::AmountExceedsBalance:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("The amount exceeds your balance."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::AmountWithFeeExceedsBalance:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("The total exceeds your balance when the %1 transaction fee is included.").
		        arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::DuplicateAddress:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("Duplicate address found, can only send to each address once per send operation."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::TransactionCreationFailed:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("Error: Transaction creation failed."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::TransactionCommitFailed:
		    QMessageBox::warning(this, tr("Send Coins"),
		        tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
		        QMessageBox::Ok, QMessageBox::Ok);
		    break;
		case WalletModel::Aborted: // User aborted, nothing to do
		    break;
		case WalletModel::OK:
			breakCycle = false;
		    break;
		}

		// something went wrong, just quit
		if (breakCycle) {
			updateBlockList();
			return;
		}

		// refresh the list and recalculate values to always include the change that is left over as first item
		updateBlockList();
		nOps = (blocksTable->rowCount() - minimumBlockAmount) / blockDivisor;
		nOdds = (blocksTable->rowCount() - minimumBlockAmount) % blockDivisor;
		if (nOdds == 1)
		{
			nOdds = blockDivisor + 1;
		}
		else if (nOdds == 2)
		{
			nOdds = blockDivisor + 2;
		}
		else if (nOdds == 0)
		{
			nOdds = blockDivisor;
		}
		if (nOdds >= (blocksTable->rowCount() - minimumBlockAmount))
		{
			nOdds = blocksTable->rowCount() - minimumBlockAmount + 1;		// optimize the last piece to the target length
		}
	}
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
// what it does: run on resize
// access:       protected
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::resizeEvent(QResizeEvent* event)
{
// resize the inner table when necessary
//	int ww = width();
//	filesTable->setColumnWidth(1, ww - 278);
}

//  ------------------------------------------------------------------------ //
// function:     showEvent(QShowEvent* event)
// what it does: run on show
// access:       protected
// return:       void
//  ------------------------------------------------------------------------ //
void DustingGui::showEvent(QShowEvent* event)
{
	updateBlockList();
}

