#include "skinspage.h"
#include "ui_skinspage.h"
#include "util.h"
//#include "settings.h"

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
#include "ui_interface.h"
#include <QVector>
#include <QRegExp>
#include <QHeaderView>
#include <QCryptographicHash>

#include <QDebug>
using namespace std;


SkinsPage::SkinsPage(QWidget *parent) : QWidget(parent), ui(new Ui::SkinsPage)
{
  
  networkTimer = new QTimer(this);
  networkTimer->setInterval(10000);
  connect(networkTimer, SIGNAL(timeout()), this, SLOT(networkTimeout()));
  ui->setupUi(this);
  resetButton = createButton(tr("&Reset to none"), SLOT(reset()));
  downloadButton = createButton(tr("&Download Themes"), SLOT(getlist()));

  QPixmap downloadPixmap(":/icons/gears");
  QIcon downloadButtonIcon(downloadPixmap);
  downloadButton->setIcon(downloadButtonIcon);
  QSettings settings("LitecoinPlus", "settings");
  inipath=GetDataDir().string().c_str();
  inipath=inipath+"/themes/";
  loadSettings();

  filesFoundLabel = new QLabel;
  statusLabel = new QLabel;

  createFilesTable();

  ui->mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
  ui->mainLayout->addWidget(filesTable, 3, 0, 1, 3);
  ui->mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 2);
  ui->mainLayout->addWidget(statusLabel, 5, 0, 1, 2);
  ui->mainLayout->addWidget(resetButton, 6, 2);
  ui->mainLayout->addWidget(downloadButton, 6, 0);

  //force find
  find();
}

void SkinsPage::reset()
{
	QApplication *app = (QApplication*)QApplication::instance();
	app->setStyleSheet(NULL);
	inifname = "";
	saveSettings();
}

void SkinsPage::find()
{
  filesTable->setRowCount(0);

  currentDir = QDir(inipath);
  QStringList files;
  files = currentDir.entryList(QStringList("*"),QDir::Files | QDir::NoSymLinks |QDir::Hidden);

  showFiles(files);
}

QStringList SkinsPage::findFiles(const QStringList &files, const QString &text)
{
  QProgressDialog progressDialog(this);
  progressDialog.setCancelButtonText(tr("&Cancel"));
  progressDialog.setRange(0, files.size());
  progressDialog.setWindowTitle(tr("Find Files"));

  QStringList foundFiles;
  for (int i = 0; i < files.size(); ++i)
  {
    progressDialog.setValue(i);
    progressDialog.setLabelText(tr("Searching file number %1 of %2...")
                                    .arg(i).arg(files.size()));
    qApp->processEvents();
    QFile file(currentDir.absoluteFilePath(files[i]));

    if (file.open(QIODevice::ReadOnly))
    {
      QString line;
      QTextStream in(&file);
      while (!in.atEnd())
      {
        if (progressDialog.wasCanceled())
          break;
        line = in.readLine();
        if (line.contains(text))
        {
          foundFiles << files[i];
          break;
        }
      }
    }
  }
  return foundFiles;
}

void SkinsPage::showFiles(const QStringList &files)
{
  inipath=currentDir.absolutePath();
  
  QString line="";// first line of file;
  QString desc="";// descrition goes here;
  QString vers="";// version info;
  int fcount=0;
  for (int i = 0; i < files.size(); ++i)
  {
    line="";
    desc="";
    vers="";
    QFile file(currentDir.absoluteFilePath(files[i]));

    QTableWidgetItem *fileNameItem = new QTableWidgetItem(files[i]);
    fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);

// read first line of file to get desc, version
    QFile inputFile(currentDir.absoluteFilePath(files[i]));
    QFileInfo fi(inputFile);

    if(fi.suffix() == "qss")
    {
      if (inputFile.open(QIODevice::ReadOnly))
      {
        QTextStream in(&inputFile);
        if(!in.atEnd())
          line = in.readLine();
      }
      inputFile.close();

// parse line
    int x=line.indexOf("desc=");
    int e=0;
    if(x>0)
    {
      e=line.indexOf(QChar('"'),x);
      if(e>0) // if we found a "
      {
        x=e+1;
        e=line.indexOf(QChar('"'),x); //find the next "
        e=e-x; 
        desc=line.mid(x,e);
      }
    }

    x=line.indexOf("vers=");
    e=0;
    if(x>0)
    {
      e=line.indexOf(QChar('"'),x);
      if(e>0) // if we found a "
      {
        x=e+1;
        e=line.indexOf(QChar('"'),x); //find the next "
        e=e-x; 
        vers=line.mid(x,e);
      }
    }

    QTableWidgetItem *descriptionItem = new QTableWidgetItem(desc);
	descriptionItem->setFlags(descriptionItem->flags() ^ Qt::ItemIsEditable);
    QTableWidgetItem *versionItem = new QTableWidgetItem(vers);
	versionItem->setFlags(versionItem->flags() ^ Qt::ItemIsEditable);

    fcount++;
    int row = filesTable->rowCount();
    filesTable->insertRow(row);
    filesTable->setItem(row, 0, fileNameItem);
    filesTable->setItem(row, 1, descriptionItem);
    filesTable->setItem(row, 2, versionItem);
  }
    }
  filesFoundLabel->setText(tr("%1 file(s) found").arg(fcount) +
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5)
    (" (Select file to load it)"));
#else
    (" (Double click on a file to load it)"));
#endif
  filesFoundLabel->setWordWrap(true);
}

QPushButton *SkinsPage::createButton(const QString &text, const char *member)
{
  QPushButton *button = new QPushButton(text);
  connect(button, SIGNAL(clicked()), this, member);
  return button;
}

void SkinsPage::createFilesTable()
{
  filesTable = new QTableWidget(0, 3);
  filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  QStringList labels;
  labels << tr("Filename") << tr("Description") << tr("Version");
  filesTable->setHorizontalHeaderLabels(labels);

  filesTable->setColumnWidth(0,160);// last column get resized automatically by qt
  filesTable->setColumnWidth(2,100);
  filesTable->setColumnWidth(1,250);
  filesTable->verticalHeader()->hide();
  filesTable->setShowGrid(false);

  connect(filesTable, SIGNAL(cellActivated(int,int)),
            this, SLOT(openFileOfItem(int,int)));
}

void SkinsPage::openFileOfItem(int row, int /* column */)
{
  QTableWidgetItem *item = filesTable->item(row, 0);
  
  inifname=item->text();
  saveSettings();
  loadSkin();
}


void SkinsPage::optionChanged()
{
  saveSettings();
  loadSettings(); // to resize the window
  loadSkin();
}

void SkinsPage::saveSettings()
{
  QSettings settings("LitecoinPlus", "settings");
  settings.setValue("filename", inifname);
}

void SkinsPage::loadSettings()
{
  QSettings settings("LitecoinPlus", "settings");
  inifname=settings.value("filename", "").toString();
}
 
void SkinsPage::loadSkin()
{
// load skin ONLY if actually one is set...
	if (inifname != "")
	{
		QFile styleFile(inipath + "/" + inifname);
		styleFile.open(QFile::ReadOnly);
		QByteArray bytes = styleFile.readAll();
		QString newStyleSheet(bytes);
		newStyleSheet.replace("myimages", inipath + "/images"); // deal with relative path
		QApplication *app = (QApplication*)QApplication::instance();
		app->setStyleSheet(NULL);
		app->setStyleSheet(newStyleSheet);
	}
}

void SkinsPage::resizeEvent(QResizeEvent* event)
{
  int ww=width();
  filesTable->setColumnWidth(1,ww-278);
}


void SkinsPage::getlist()
{
  // show a downloading message in status bar
  statusLabel->setText("<b>" + tr("Downloading themes from http://litecoinplus.co...") + "</b>");
  latestNetError = "";

  // first, let's disable the download button (triple-clicks fanatics !)
  downloadButton->setEnabled(false);

  connect(&manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(getListFinished(QNetworkReply*)));

  QNetworkRequest request;
  request.setUrl(QUrl("http://litecoinplus.co/themes/list.v2.txt"));
  request.setRawHeader("User-Agent", "Wallet theme request");

  networkTimer->start();
  manager.get(request);
}

//  ------------------------------------------------------------------------ //
// function:     checkForUpdates()
// what it does: check if there are available updates
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void SkinsPage::checkForUpdates()
{
// show a downloading message in status bar
	statusLabel->setText("<b>" + tr("Checking for theme updates...") + "</b>");
	latestNetError = "";

// first, let's disable the download button (triple-clicks fanatics !)
	downloadButton->setEnabled(false);

// connect the event and launch it
	connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkForUpdatesCore(QNetworkReply*)));
	QNetworkRequest request;
	request.setUrl(QUrl("http://litecoinplus.co/themes/list.v2.txt"));
	request.setRawHeader("User-Agent", "Wallet theme request");
	networkTimer->start();
	manager.get(request);
}

//  ------------------------------------------------------------------------ //
// function:     checkForUpdatesCore(QNetworkReply* reply)
// what it does: check if there are available updates (CORE)
// access:       private
// return:       void
//  ------------------------------------------------------------------------ //
void SkinsPage::checkForUpdatesCore(QNetworkReply* reply)
{
	QCryptographicHash md5_r(QCryptographicHash::Md5);
	QCryptographicHash md5_l(QCryptographicHash::Md5);

// calculate the md5 of remote and local files
	if (netHandleError(reply, "http://litecoinplus.co/themes/list.v2.txt"))
	{

	// get the remote MD5
		disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
		md5_r.addData(reply->readAll());

	// get the local MD5
	    QString filename = inipath + "/list.v2.txt";
		QFile file(filename);
		file.open(QIODevice::ReadOnly);
		md5_l.addData(file.readAll());
		file.close();

	// compare
		if (md5_r.result().toHex() != md5_l.result().toHex())
		{
			statusLabel->setText("<b>" + tr("A new theme pack is available, click Download Themes to update.") + "</b>");
		}
		else
		{
			statusLabel->setText("<b>" + tr("Theme pack is up to date.") + "</b>");
		}
		latestNetError = "";
	}
	else
	{
		reply->abort();
	}
	downloadButton->setEnabled(true);
}

//  ------------------------------------------------------------------------ //
// function:     showEvent(QShowEvent* event)
// what it does: run on show
// access:       protected
// return:       void
//  ------------------------------------------------------------------------ //
void SkinsPage::showEvent(QShowEvent* event)
{
// check for updates in the theme pack
	checkForUpdates();
}

bool SkinsPage::netHandleError(QNetworkReply* reply, QString urlDownload)
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
	networkTimeout();
	return(false);
}

void SkinsPage::getListFinished(QNetworkReply* reply)
{
	if (netHandleError(reply, "http://litecoinplus.co/themes/list.v2.txt"))
	{
		disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
		connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
		QString pagelist = reply->readAll();
		QStringList list = pagelist.split('\n');
		QString line;

	// saves also the descriptor, will be used to check for an new version
		download((QString)"http://litecoinplus.co/themes/list.v2.txt");
		for (int i = 0; i < list.count(); i++)
		{
			line=list.at(i).toLocal8Bit().constData();
			line = line.simplified(); // strip extra characters
			line.replace("\r",""); // this one too
			if (line.length())
			{
				if (line.startsWith("createDir::"))		// by Simone: which subfolders need to be created, are declared in the file
				{
					line.replace("createDir::", "");	

				// create dir if it doesn't exist yet
					QDir imgdir(inipath + line);
					if (!imgdir.exists())
					{
						imgdir.mkpath(".");
					}
				}
				else if (!line.startsWith("#"))			// by Simone: added comment lines, skip them
				{  
					download("http://litecoinplus.co/themes/" + line);
				}
			}
		}
	}
	else
	{
		reply->abort();
	}
}

void SkinsPage::download(const QUrl &filename)
{
  QNetworkRequest request;//(filename);
  request.setUrl(filename);
  request.setRawHeader("User-Agent", "Wallet theme request");
  networkTimer->start();
  reply = manager.get(request);
  currentDownloads.append(reply);
}

void SkinsPage::downloadFinished(QNetworkReply *reply)
{
  QUrl url = reply->url();
  if (netHandleError(reply, url.toEncoded()))
  {
    QString filename = inipath + url.path().replace("/themes/","/");
    if (!saveToDisk(filename, reply))
    {
      QString fError = tr("Could not open ") + filename + " for writing: " + latestFileError;
      emit error(tr("File Saving Error"), fError, false);
    }
    currentDownloads.removeAll(reply);
    reply->deleteLater();

    // when finish all, re-enable the download button and force a find
    if (currentDownloads.isEmpty()) 
    {
	  statusLabel->setText("");
      downloadButton->setEnabled(true);
      disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
      find();
      emit information(tr("Themes Download"), tr("Themes were successfully downloaded and installed."));
    }
  }
}

bool SkinsPage::isHttpRedirect(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303
           || statusCode == 305 || statusCode == 307 || statusCode == 308;
}

bool SkinsPage::saveToDisk(const QString &filename, QIODevice *data)
{
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly))
  {
    latestFileError = file.errorString();
    return false;
  }

  file.write(data->readAll());
  file.close();

  return true;
}

void SkinsPage::networkTimeout()
{
	// signal error and preset for next operation
	networkTimer->stop();
	if (!currentDownloads.isEmpty()) {
		QList<QNetworkReply *>::iterator i;
		for (i = currentDownloads.begin(); i != currentDownloads.end(); ++i)
		{
			(*i)->abort();		// abort here all and any pending reply, to avoid mess if anything comes back
		}
	}
	if (latestNetError == "")
	{
		latestNetError = tr("Network timeout. Please check your network and try again.");
	}
	statusLabel->setText("");
	downloadButton->setEnabled(true);
	disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
	find();
	emit error(tr("Themes Download Error"), latestNetError, true);
}

