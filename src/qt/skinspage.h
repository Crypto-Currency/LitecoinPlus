#ifndef SKINSPAGE_H
#define SKINSPAGE_H

//#include <QDialog>
#include <QWidget>
//#include <QComboBox>
#include <QTableWidget>
#include <QDir>
#include <QPushButton>
#include <QLabel>
#include <QMainWindow>
#include <boost/filesystem.hpp>
#include <string>

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QStringList>
#include <QTimer>

namespace Ui
{
	class SkinsPage;
}

class SkinsPage : public QWidget
{
	Q_OBJECT

	QNetworkAccessManager manager;
	QList<QNetworkReply *> currentDownloads;


public:
	SkinsPage(QWidget *parent = 0);
	bool saveToDisk(const QString &filename, QIODevice *data);
	static bool isHttpRedirect(QNetworkReply *reply);
	void loadSkin();

private slots:
	void reset();
	void find();
	void openFileOfItem(int row, int column);
	void optionChanged();
	void checkForUpdatesCore(QNetworkReply* reply);
	void getlist();
	bool netHandleError(QNetworkReply* reply, QString urlDownload);
	void getListFinished(QNetworkReply* reply);
	void downloadFinished(QNetworkReply *reply);
	void networkTimeout();

protected:
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent * event);

private:
	Ui::SkinsPage *ui;
	QStringList findFiles(const QStringList &files, const QString &text);
	void showFiles(const QStringList &files);
	QPushButton *createButton(const QString &text, const char *member);
	void createFilesTable();
	void loadSkin(QString fname);
	void loadSettings();
	void saveSettings();
	void checkForUpdates();
	void download(const QUrl &filename);
	boost::filesystem::path IniFile;
	QString inipath,inifname;
	QMainWindow fSize;

	QTimer *networkTimer;
	QLabel *filesFoundLabel;
	QLabel *statusLabel;
	QPushButton *resetButton;
	QPushButton *downloadButton;
	QTableWidget *filesTable;
	QDir currentDir;
	QNetworkReply *reply;
	QString latestNetError;
	QString latestFileError;

signals:
	void error(const QString &title, const QString &message, bool modal);
	void status(const QString &message);
	void information(const QString &title, const QString &message);
};

#endif
