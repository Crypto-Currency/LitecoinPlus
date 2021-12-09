#include <QProcess>

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QApplication>
 #include <QtWidgets>
#else
#include <QtGui>
 #include <QtGui/QApplication>
#endif


#include <QUrl>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include "updater.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#ifdef Q_OS_WIN32
 #include <shlobj.h>
#endif

#include <iostream> // needed for cout - can be removed
using namespace std;
using namespace boost;

bool sucess=false;

string initicker="LCP";// check GetDefaultDataDir() for lowercase
string DirName="LitecoinPlus";
string downlocation="https://LitecoinPlus.co/downloads";
QString downloadLocation;
string AppName="litecoinplus-qt";
//string apppath="C:\\"+DirName+"\\";

std::string FileName;
std::string strDataDir = GetDefaultDataDir().string();
std::string strAppDir = GetDefaultAppDir().string();


//-------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  UpdaterForm uf;
  QDesktopWidget widget;
  QRect screenGeometry = widget.screenGeometry();
  int height = screenGeometry.height();
  int width = screenGeometry.width();
  int x=(width - uf.width()) / 2.0;
  int y=(height - uf.height()) / 2.0;
  uf.setGeometry(x,y,uf.width(),uf.height());
  Qt::WindowFlags flags = uf.windowFlags();
  uf.setWindowFlags(flags | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
  uf.setWindowTitle(QString::fromStdString(DirName+" Launcher-Updater"));
  uf.show();
  return app.exec();
}


//-------------------------------------------------------------------------------------
UpdaterForm::UpdaterForm(QWidget *parent) : QWidget(parent)
{
  networkTimer = new QTimer(this);
  networkTimer->setInterval(10000);
  connect(networkTimer, SIGNAL(timeout()), this, SLOT(networkTimeout()));
  ui.setupUi(this);
  ui.TextEdit->appendPlainText("working ...");

#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":images/"+QString::fromStdString(initicker)));
    setWindowIcon(QIcon(":images/"+QString::fromStdString(initicker)));
#else
//    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

// give some time for the form to show up
  QTimer::singleShot(1000, this, SLOT(start()));
}


//-------------------------------------------------------------------------------------
void UpdaterForm::start()
{
  string temp="checking path "+strDataDir;
  ui.TextEdit->appendPlainText(temp.c_str());

  try
  {
    QDir dir(strDataDir.c_str());
    if (!dir.exists())
    {
      temp="creating "+strDataDir;
      ui.TextEdit->appendPlainText(temp.c_str());
      if(!dir.mkpath(strDataDir.c_str()))
      {
        temp="can not create "+strDataDir;
        ui.TextEdit->appendPlainText(temp.c_str());
        throw runtime_error("Can not create "+strDataDir);
      }
    }
    else
    {
      temp="path exists.";
      ui.TextEdit->appendPlainText(temp.c_str());
    }
  }
  catch (std::exception& e)
  {
    QMessageBox::information(this,tr("creating path"),tr(e.what()) );
  }

// get local version.txt and read it
  string filename = strDataDir + "/version.txt";
  ifstream file;
  file.open(filename.c_str());
  string localtx;
  getline(file, localtx);
  file.close();
  boost::trim_right(localtx);
  if(!localtx.length())
  {
    localtx="0000";
  }

  ui.label3->setText(localtx.c_str());

// get remote version.txt and read it
  getlist();
  while(networkTimer->isActive())
  {// wait for download to finish
    qApp->processEvents();
  }

  int iLocalVer=ui.label3->text().toInt();
  int iRemoteVer=ui.label4->text().toInt();

  if(iRemoteVer > iLocalVer)
  {
    temp="Found new version";
    ui.TextEdit->appendPlainText(temp.c_str());

    download(QString::fromStdString(strAppDir),NULL);
  }  
  else
  { // launch wallet
    string strAppFilename=GetDefaultAppName();
    string strPathApp=strAppDir+strAppFilename;
    QProcess::startDetached(strPathApp.c_str());
    exit(0);
  }
}

//-------------------------------------------------------------------------------------
void UpdaterForm::getlist()
{
  string temp="checking "+downlocation;
  ui.TextEdit->appendPlainText(temp.c_str());

// get remote version
// connect the event and launch it
  connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(getListFinished(QNetworkReply*)));

  QNetworkRequest request;
  downloadLocation = QString::fromStdString(downlocation+"/version.txt");

  request.setUrl(QUrl(downloadLocation));
  request.setRawHeader("User-Agent", "Wallet update request");

QSslConfiguration conf = request.sslConfiguration();
conf.setPeerVerifyMode(QSslSocket::VerifyNone);
request.setSslConfiguration(conf);

  networkTimer->start();
  manager.get(request);
}

//-------------------------------------------------------------------------------------
void UpdaterForm::getListFinished(QNetworkReply* reply)
{
  if (netHandleError(reply, downloadLocation))
  {
    disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
    connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
    QString versionlist=reply->readAll();

  string line=versionlist.toStdString();//at(0).toLocal8Bit().constData();
  boost::trim_right(line);
  ui.label4->setText(line.c_str());
  }
  else
  {
    reply->abort();
  } 
}

//-------------------------------------------------------------------------------------
void UpdaterForm::download(const QUrl &downTo,QNetworkReply *reply)
{
  networkTimer->stop();
  QString temp=downTo.toString();
  string appFilename=GetDefaultAppName();

// get app
  disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
  connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
  QNetworkRequest request;//(filename);

  string tempurl=downlocation+"/"+appFilename;
  request.setUrl(QString::fromStdString(tempurl));
  request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
  string mess="requesting "+tempurl;
  ui.TextEdit->appendPlainText(mess.c_str());

  request.setRawHeader("User-Agent", "app request");
  FileName=temp.toStdString()+appFilename;

  networkTimer->start();
  reply = manager.get(request);
  connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(updateProgress(qint64, qint64)));
  while(networkTimer->isActive())
  {// wait for download to finish
    qApp->processEvents();
  }
  QFile myfile(QString::fromStdString(FileName));
  myfile.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup| QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);

  mess="saved "+FileName;
  ui.TextEdit->appendPlainText(mess.c_str());

// get version.txt
  disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
  connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
  request.setUrl(downloadLocation);
  request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
  request.setRawHeader("User-Agent", "version.txt request");

  FileName=strDataDir+"/version.txt";
  networkTimer->start();
  reply = manager.get(request);
  connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(updateProgress(qint64, qint64)));

  while(networkTimer->isActive())
  {// wait for download to finish
    qApp->processEvents();
  }

  // launch wallet
  string strAppFilename=GetDefaultAppName();
  string strPathApp=strAppDir+strAppFilename;
  QProcess::startDetached(strPathApp.c_str());
  exit(0);

}

//-------------------------------------------------------------------------------------
void UpdaterForm::downloadFinished(QNetworkReply *reply)
{
  networkTimer->stop();
  string mess;

  if (!saveToDisk(FileName.c_str(), reply))
  {
    QString fError=tr("Could not open ")+FileName.c_str()+" for writing: "+latestFileError;
    mess="File Saving Error"+fError.toStdString();
    ui.TextEdit->appendPlainText(mess.c_str());
  }

  disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
  mess="downloadFinished done\n";
  ui.TextEdit->appendPlainText(mess.c_str());
}

//-------------------------------------------------------------------------------------
bool UpdaterForm::saveToDisk(const QString &filename, QNetworkReply *reply)
{
  string mess="saving "+filename.toStdString();
  ui.TextEdit->appendPlainText(mess.c_str());
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly))
  {
    latestFileError = file.errorString();
    mess="file open error "+file.errorString().toStdString();
    ui.TextEdit->appendPlainText(mess.c_str());

    return false;
  }

  file.write(reply->readAll());
  file.close();

  ui.progressBar->setValue(0);
  return true;
}

//-------------------------------------------------------------------------------------
void UpdaterForm::updateProgress(qint64 read, qint64 total)
{
  ui.progressBar->setMaximum(total);
  ui.progressBar->setValue(read);
  networkTimer->start(); // restart the timer so it doesn't timeout on long downloads
}

//-------------------------------------------------------------------------------------
bool UpdaterForm::netHandleError(QNetworkReply* reply, QString urlDownload)
{
  networkTimer->stop();
  if (reply->error())
  {
    latestNetError = tr("Download of ") + urlDownload + tr(" failed: ") + reply->errorString();
  }
  else if (isHttpRedirect(reply))
  {
    latestNetError = tr("HTTP redirect while attempting download of ") + urlDownload;
  }
  else
 {
  // signal no errors here
  return(true);
  }

  // execute the same function, displaying latest occured error
  string mess="network timeout.";
  ui.TextEdit->appendPlainText(mess.c_str());
  networkTimeout();
  return(false);
}

//-------------------------------------------------------------------------------------
void UpdaterForm::networkTimeout()
{
  // signal error and preset for next operation
  networkTimer->stop();
  if (latestNetError == "")
  {
    latestNetError = tr("Network timeout. Please check your network and try again.");
  }
  disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
  QMessageBox::information(this,tr("Network Error"),tr(latestNetError.toStdString().c_str()));
}

//-------------------------------------------------------------------------------------
bool UpdaterForm::isHttpRedirect(QNetworkReply *reply)
{
  int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  return statusCode == 301 || statusCode == 302 || statusCode == 303
      || statusCode == 305 || statusCode == 307 || statusCode == 308;
}

//-------------------------------------------------------------------------------------
boost::filesystem::path GetDefaultDataDir()
{
    namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\DirName
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\DirName
    // Mac: ~/Library/Application Support/DirName
    // Unix: ~/.DirName
#ifdef WIN32
    // Windows
//    return GetSpecialFolderPath(CSIDL_APPDATA) / DirName.c_str();
  char* appdata = getenv("APPDATA");
  string datapath=appdata;
  datapath+="\\"+DirName;
//  String path=apppath+appname;
return datapath;
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    pathRet /= "Library/Application Support";
    fs::create_directory(pathRet);
    return pathRet / DirName.c_str();
#else
    // Unix
  string dname="."+DirName;

//  litecoinplus does not use lowercase
//  boost::algorithm::to_lower(dname);  // some coins use all lower case
  return pathRet / dname.c_str();
#endif
#endif
}

//-------------------------------------------------------------------------------------
boost::filesystem::path GetDefaultAppDir()
{
    namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\DirName
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\DirName
    // Mac: ~/Library/Application Support/DirName
    // Unix: ~/.DirName
#ifdef WIN32
    // Windows
    return "C:/"+DirName;
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    pathRet = "/Applications/"+GetDefaultAppName()+".app/Contents/MacOS/";
downlocation=downlocation+"/MAC";
//    fs::create_directory(pathRet);
    return pathRet;
#else

    // Unix
  string dname=DirName;

//  LitecoinPlus does not use lowercase
//  boost::algorithm::to_lower(dname);  // some coins use all lower case

////// make a folder on the desktop
dname="Desktop/"+dname+"/";
fs::create_directory(pathRet / dname.c_str());

/////  don't make a folder
//dname="Desktop/";

std::cout<<"unix location "<<pathRet / dname.c_str()<<"\n";

  return pathRet / dname.c_str();
#endif
#endif
}

//-------------------------------------------------------------------------------------
string GetDefaultAppName()
{
#ifdef WIN32
  // Windows
  return AppName+".exe";
#endif

#ifdef MAC_OSX
  // MAC
  return AppName;
#endif

  // Unix
  return AppName;
}

