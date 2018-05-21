#ifndef SKINSPAGE_H
#define SKINSPAGE_H

//#include <QDialog>
#include <QWidget>
#include <QComboBox>
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

namespace Ui
{
  class SkinsPage;
}

class SkinsPage : public QWidget
{
  Q_OBJECT

  QNetworkAccessManager manager;
  QList<QNetworkReply *> currentDownloads;
  //QVector<QNetworkReply *> currentDownloads;


public:
  SkinsPage(QWidget *parent = 0);
//  static SSettings *Settings;
//    static QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);
    static bool isHttpRedirect(QNetworkReply *reply);

private slots:
  void browse();
  void reset();
  void find();
  void openFileOfItem(int row, int column);
  void optionChanged();
  void getlist();
//  void ReadList();

void getListFinished(QNetworkReply* reply);
//void replyFinished(QNetworkReply* reply);
void downloadFinished(QNetworkReply *reply);

protected:
  void resizeEvent(QResizeEvent *event);

private:
  Ui::SkinsPage *ui;
  QStringList findFiles(const QStringList &files, const QString &text);
  void showFiles(const QStringList &files);
  QPushButton *createButton(const QString &text, const char *member);
  QComboBox *createComboBox(const QString &text = QString());


  void createFilesTable();
  void loadSkin(QString fname);
  void loadSettings();
  void saveSettings();
  void loadSkin();
  void download(const QUrl &filename);
  boost::filesystem::path IniFile;
  QString inipath,inifname;
//  QDesktopWidget fSize;
  QMainWindow fSize;

  QComboBox *fileComboBox;
  QComboBox *textComboBox;
  QComboBox *directoryComboBox;
  QLabel *fileLabel;
  QLabel *textLabel;
  QLabel *directoryLabel;
  QLabel *filesFoundLabel;
  QPushButton *browseButton;
  QPushButton *resetButton;
  QPushButton *findButton;
  QTableWidget *filesTable;
  QDir currentDir;
};

#endif
