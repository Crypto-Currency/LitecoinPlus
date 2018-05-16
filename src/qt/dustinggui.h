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
	// methods
	DustingGui(QWidget *parent = 0);
	void setModel(WalletModel *model);

private slots:
	void updateBlockList();
	void compactBlocks();

protected:
	// methods
	void resizeEvent(QResizeEvent *event);

private:
	// variables
	Ui::DustingGui *ui;
	QTableWidget *blocksTable;
	WalletModel *model;
	int sortColumn;
	Qt::SortOrder sortOrder;
	QPushButton *refreshButton;
	QPushButton *dustButton;

	// methods
	void createBlockList();
	void sortView(int column, Qt::SortOrder order);
	QString strPad(QString s, int nPadLength, QString sPadding);
};

#endif // DUSTINGGUI_H
