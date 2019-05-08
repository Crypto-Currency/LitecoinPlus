#ifndef SPLASH_H
#define SPLASH_H

#include <QDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QCloseEvent>
#include <QTimer>

namespace Ui {
    class Splash;
}

/** "About" dialog box */
class Splash : public QDialog
{
    Q_OBJECT

public:
    explicit Splash(QWidget *parent = 0);
    ~Splash();
	void showSplash();
	void hideSplash();
	void systemOnTop();
	void setMessage(const char *message);

protected:
	void closeEvent(QCloseEvent *event);

private:
    Ui::Splash *ui;
	QTimer *timer;

	void setRandomBackground();

private slots:
	void updateTimer();
};

#endif // SPLASH_H
