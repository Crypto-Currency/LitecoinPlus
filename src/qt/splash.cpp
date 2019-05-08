#include "splash.h"
#include "ui_splash.h"

#include "util.h"
#include "version.h"

Splash::Splash(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Splash)
{
// general setup
    ui->setupUi(this);
	setWindowTitle(tr("LitecoinPlus") + " - " + QString::fromStdString(CLIENT_BUILD));

// adds a timer that randomly replace background
	timer = new QTimer(this);
	timer->setInterval(59000);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateTimer()));
	timer->start();

// sets background once
	setRandomBackground();
}

void Splash::showSplash()
{
    // Center startup window in the screen
	QRect screenGeometry = QApplication::desktop()->screenGeometry();
	int x = (screenGeometry.width() - width()) / 2;
	int y = (screenGeometry.height() - height()) / 2;
	move(x, y);
    show();
}

void Splash::hideSplash()
{
	timer->stop();
	hide();
}

void Splash::systemOnTop()
{
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	setWindowModality(Qt::ApplicationModal);
}

void Splash::setMessage(const char *message)
{
// update the message
	ui->info->setText("<span style='color: white;'>" + tr(message) + "</span>");
	QApplication::instance()->processEvents();
	Sleep(1);
	QApplication::instance()->processEvents();
}

void Splash::updateTimer()
{
	setRandomBackground();
}

void Splash::setRandomBackground()
{
// sets a random background each time is called
	int v = (time(NULL)) % 4 + 1;
	char s[32];
	sprintf(s, ":/images/startup%d", v);
	QPixmap bkgnd(s);
    bkgnd = bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Background, bkgnd);
    this->setPalette(palette);
}

void Splash::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

Splash::~Splash()
{
    delete ui;
}

