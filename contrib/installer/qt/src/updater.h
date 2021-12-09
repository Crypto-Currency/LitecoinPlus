#ifndef UPDATERFORM_H
#define UPDATERFORM_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

#include <openssl/crypto.h>

#include <QtGlobal>
#if QT_VERSION >= 0x050000
//  #include <QApplication>
#else
  #include <QtGui/QApplication>
#endif

#ifdef Q_OS_WIN32
 #include "win_ui_updaterForm.h"
#else
 #include "ui_updaterForm.h"
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>


boost::filesystem::path GetDefaultDataDir();
boost::filesystem::path GetDefaultAppDir();
std::string GetDefaultAppName();

class UpdaterForm : public QWidget
{
  Q_OBJECT

  QNetworkAccessManager manager;

  public:
    UpdaterForm(QWidget *parent = 0);
    Ui::UpdaterForm ui;
    bool saveToDisk(const QString &filename, QNetworkReply* reply);

  private slots:
    void start();
    void getlist();
    void getListFinished(QNetworkReply* reply);
    void downloadFinished(QNetworkReply *reply);
    void updateProgress(qint64 read, qint64 total);
    bool netHandleError(QNetworkReply* reply, QString urlDownload);
    void networkTimeout();
    static bool isHttpRedirect(QNetworkReply *reply);

  private:
    void download(const QUrl &downTo,QNetworkReply *reply);
    QTimer *networkTimer;

    QString latestNetError;
    QString latestFileError;
    QNetworkReply *reply;

};
#endif
