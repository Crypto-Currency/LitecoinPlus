#ifndef DUSTINGGUI_H
#define DUSTINGGUI_H

//#include <QDialog>
#include <QWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QMainWindow>
#include <boost/filesystem.hpp>

// declaring namespace and classes
namespace Ui {
  class DustingGui;
}
class WalletModel;
class CCoinControl;

// by Simone: brand new class to run automatic wallet dusting
class DustingGui : public QWidget
{
	Q_OBJECT

public:
	// properties
    static CCoinControl *coinControl;

	// methods
	DustingGui(QWidget *parent = 0);
	void setModel(WalletModel *model);

private slots:
	void updateBlockList();
	void compactBlocks();
    void on_addressBookButton_clicked();

protected:
	// methods
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent * event);

private:
	// properties
	Ui::DustingGui *ui;
	QTableWidget *blocksTable;
	WalletModel *model;
	int sortColumn;
	Qt::SortOrder sortOrder;
	QLabel *infoLabel;
	QPushButton *refreshButton;
	QPushButton *dustButton;
	int blockDivisor;
	int minimumBlockAmount;
    enum
    {
        COLUMN_AMOUNT,
        COLUMN_DATE,
        COLUMN_LABEL,
        COLUMN_ADDRESS,
        COLUMN_CONFIRMATIONS,
        COLUMN_TXHASH,
        COLUMN_AMOUNT_INT64,
        COLUMN_VOUT_INDEX,
		COLUMN_INPUT_SIZE
    };

	// methods
	void createBlockList();
	void sortView(int column, Qt::SortOrder order);
	QString strPad(QString s, int nPadLength, QString sPadding);
};

#endif // DUSTINGGUI_H
