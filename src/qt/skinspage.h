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

namespace Ui {
  class SkinsPage;
}

class SkinsPage : public QWidget
{
  Q_OBJECT

public:
  SkinsPage(QWidget *parent = 0);
//  static SSettings *Settings;

private slots:
  void browse();
  void find();
  void openFileOfItem(int row, int column);
  void optionChanged();

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

  boost::filesystem::path IniFile;
  QString inipath,inifname;
  bool inib1,inib2,inib3;
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
  QPushButton *findButton;
  QTableWidget *filesTable;
  QDir currentDir;

};

#endif // SKINSPAGE_H
