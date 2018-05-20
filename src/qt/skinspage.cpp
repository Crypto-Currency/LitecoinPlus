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

#include <QDebug>
using namespace std;


SkinsPage::SkinsPage(QWidget *parent) : QWidget(parent), ui(new Ui::SkinsPage)
{
  
  ui->setupUi(this);
  browseButton = createButton(tr("&Browse..."), SLOT(browse()));
  findButton = createButton(tr("&Find"), SLOT(find()));
  resetButton = createButton(tr("&Reset to none"), SLOT(reset()));

// connect download button
  connect(ui->downloadButton, SIGNAL(released()), this, SLOT(getlist()));

// load settings - do before connecting signals or loading will trigger optionchanged
  QSettings settings("LitecoinPlus", "settings");
  inipath=settings.value("path", "").toString();
//  IniFile = GetDataDir() / "skins.ini";
  loadSettings();
  loadSkin();

  fileComboBox = createComboBox(tr("*"));
  textComboBox = createComboBox();
  
//qDebug() << "from settings inipath:" <<inipath;
  if(inipath!="")
    directoryComboBox = createComboBox(inipath);
  else
  {
    inipath=GetDataDir().string().c_str();
    inipath=inipath+"/skins";
    directoryComboBox=createComboBox(inipath);
    //directoryComboBox = createComboBox(GetDataDir().string().c_str()+("/skins"));skins

  }
//    directoryComboBox = createComboBox(QDir::currentPath());
//qDebug() << "from getdatadir inipath:" <<inipath;

  fileLabel = new QLabel(tr("Named:"));
  textLabel = new QLabel(tr("Description search:"));
  directoryLabel = new QLabel(tr("In directory:"));
  filesFoundLabel = new QLabel;

  createFilesTable();

//  QGridLayout *mainLayout = new QGridLayout;
  ui->mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
  ui->mainLayout->addWidget(fileLabel, 0, 0);
  ui->mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
  ui->mainLayout->addWidget(textLabel, 1, 0);
  ui->mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
  ui->mainLayout->addWidget(directoryLabel, 2, 0);
  ui->mainLayout->addWidget(directoryComboBox, 2, 1);
  ui->mainLayout->addWidget(browseButton, 2, 2);
  ui->mainLayout->addWidget(filesTable, 3, 0, 1, 3);
  ui->mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 2);
  ui->mainLayout->addWidget(findButton, 4, 2);
  ui->mainLayout->addWidget(resetButton, 5, 2);

  //force find
  find();
}

void SkinsPage::browse()
{
  QString directory=QFileDialog::getExistingDirectory(this,tr("Find Files"),inipath);

  if (!directory.isEmpty())
  {
    if (directoryComboBox->findText(directory) == -1)
      directoryComboBox->addItem(directory);
    directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
  }
  inipath=directory;
  // save settings
  saveSettings();
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

  QString fileName = fileComboBox->currentText();
  QString text = textComboBox->currentText();
  QString path = directoryComboBox->currentText();

  currentDir = QDir(path);
  QStringList files;
  if (fileName.isEmpty())
    fileName = "*";
  files = currentDir.entryList(QStringList(fileName),
                                 QDir::Files | QDir::NoSymLinks |QDir::Hidden);

  if (!text.isEmpty())
    files = findFiles(files, text);
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
//QMessageBox::information(this,tr("currentDir:"),tr("=%1").arg(inipath));
  
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

QComboBox *SkinsPage::createComboBox(const QString &text)
{
  QComboBox *comboBox = new QComboBox;
  comboBox->setEditable(false);  //  was true
  comboBox->addItem(text);
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5) || defined(Q_WS_SIMULATOR)
  comboBox->setMinimumContentsLength(3);
#endif
  return comboBox;
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
//  QMessageBox::information(this,tr("IniFile:"),tr("=%1").arg(IniFile));
  saveSettings();
  loadSettings(); // to resize the window
  loadSkin();
}

void SkinsPage::saveSettings()
{
  QSettings settings("LitecoinPlus", "settings");
  settings.setValue("path", inipath);
  settings.setValue("filename", inifname);
}

void SkinsPage::loadSettings()
{
  QSettings settings("LitecoinPlus", "settings");
  inipath=settings.value("path", "").toString();
  inifname=settings.value("filename", "").toString();
}
 
void SkinsPage::loadSkin()
{
	QFile styleFile(inipath+"/"+inifname);
	styleFile.open(QFile::ReadOnly);
	QByteArray bytes = styleFile.readAll();
	QString newStyleSheet(bytes);
	QApplication *app = (QApplication*)QApplication::instance();
	app->setStyleSheet(NULL);
	app->setStyleSheet(newStyleSheet);
}

void SkinsPage::resizeEvent(QResizeEvent* event)
{
  int ww=width();
  filesTable->setColumnWidth(1,ww-278);
}


void SkinsPage::getlist()
{
  // create dir if not
  QDir dir(qApp->applicationDirPath()+"/themes");
  if (!dir.exists())
    dir.mkpath(".");
  
  QDir imgdir(qApp->applicationDirPath()+"/themes/images");
  if (!imgdir.exists())
    imgdir.mkpath(".");

  connect(&manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(getListFinished(QNetworkReply*)));

  QNetworkRequest request;
  request.setUrl(QUrl("http://litecoinplus.co/themes/list.txt"));
  request.setRawHeader("User-Agent", "Wallet theme request");

  manager.get(request);
}

void SkinsPage::getListFinished(QNetworkReply* reply)
{
  disconnect(&manager, SIGNAL(finished(QNetworkReply*)), 0, 0);  
  connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
  QString pagelist=reply->readAll();
  QStringList list = pagelist.split('\n');
  QString line;

  for(int i=0;i<list.count();i++)
  {
    line=list.at(i).toLocal8Bit().constData();
    line.simplified(); // strip extra characters
    line.replace("\r",""); // this one too
    if(line.length())
    {  
      download("http://litecoinplus.co/themes/"+line);
    } 
  }
}

void SkinsPage::download(const QUrl &filename)
{
  QNetworkRequest request;//(filename);
  request.setUrl(filename);
  request.setRawHeader("User-Agent", "Wallet theme request");
  QNetworkReply *reply = manager.get(request);
  currentDownloads.append(reply);
}

void SkinsPage::downloadFinished(QNetworkReply *reply)
{
  //qDebug() << " downloadfinished called:\n reply = " <<reply->url();
  QUrl url = reply->url();
  if (reply->error())
  {
    fprintf(stderr, "Download of %s failed: %s\n",
      url.toEncoded().constData(),
      qPrintable(reply->errorString()));
  }
  else
  {
    if (isHttpRedirect(reply))
    {
      fputs("Request was redirected.\n", stderr);
    }
    else
    {
      QString filename = "." + url.path();
      if (saveToDisk(filename, reply))
      {
        printf("Download of %s succeeded\n (saved to %s)\n",
          url.toEncoded().constData(), qPrintable(filename));
      }
    }
  }

  currentDownloads.removeAll(reply);
  reply->deleteLater();

  if (currentDownloads.isEmpty()) 
  {
    // all downloads finished
    //qDebug() << " done downloading.\n";
    //force find
    find();
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
    fprintf(stderr, "Could not open %s for writing: %s\n",
      qPrintable(filename), qPrintable(file.errorString()));
    return false;
  }

  file.write(data->readAll());
  file.close();

  return true;
}
