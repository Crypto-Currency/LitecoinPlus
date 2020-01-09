#ifndef ALERTGUI_H
#define ALERTGUI_H

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
  class AlertGui;
}

// by Simone: brand new class to list information alerts, warnings etc.
class AlertGui : public QWidget
{
	Q_OBJECT

public:
	// methods
	AlertGui(QWidget *parent = 0);

private slots:
	void updateAlertsList();

protected:
	// methods
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent * event);
	void hideEvent(QHideEvent * event);

private:
	// properties
	Ui::AlertGui *ui;
	QTableWidget *alertsTable;
	int sortColumn;
	Qt::SortOrder sortOrder;
	QLabel *infoLabel;
    enum
    {
        COLUMN_ID,
        COLUMN_DATE,
        COLUMN_TYPE,
        COLUMN_PRIORITY,
        COLUMN_MESSAGE
    };

	// methods
	void createAlertsList();
	void sortView(int column, Qt::SortOrder order);
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public slots:
    void alertMapUpdated();
};

#endif // ALERTGUI_H
