/********************************************************************************
** Form generated from reading UI file 'updaterForm.ui'
**
** Created: Thu May 20 15:47:43 2021
**      by: Qt User Interface Compiler version 4.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UPDATERFORM_H
#define UI_UPDATERFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>

QT_BEGIN_NAMESPACE

class Ui_UpdaterForm
{
public:
    QWidget *centralwidget;
    QPlainTextEdit *TextEdit;
    QProgressBar *progressBar;
    QLabel *label1;
    QLabel *label2;
    QLabel *label3;
    QLabel *label4;

    void setupUi(QWidget *UpdaterForm)
    {
        if (UpdaterForm->objectName().isEmpty())
            UpdaterForm->setObjectName(QString::fromUtf8("UpdaterForm"));
        UpdaterForm->resize(282, 278);
        QFont font;
        font.setFamily(QString::fromUtf8("Ubuntu"));
        UpdaterForm->setFont(font);
        centralwidget = new QWidget(UpdaterForm);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        centralwidget->setGeometry(QRect(0, 0, 276, 271));
        TextEdit = new QPlainTextEdit(centralwidget);
        TextEdit->setObjectName(QString::fromUtf8("TextEdit"));
        TextEdit->setGeometry(QRect(10, 10, 256, 192));
        progressBar = new QProgressBar(centralwidget);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(20, 250, 241, 20));
        progressBar->setValue(0);
        label1 = new QLabel(centralwidget);
        label1->setObjectName(QString::fromUtf8("label1"));
        label1->setGeometry(QRect(20, 205, 111, 17));
        label1->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);
        label2 = new QLabel(centralwidget);
        label2->setObjectName(QString::fromUtf8("label2"));
        label2->setGeometry(QRect(20, 227, 111, 17));
        label2->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);
        label3 = new QLabel(centralwidget);
        label3->setObjectName(QString::fromUtf8("label3"));
        label3->setGeometry(QRect(140, 205, 66, 17));
        label3->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);
        label4 = new QLabel(centralwidget);
        label4->setObjectName(QString::fromUtf8("label4"));
        label4->setGeometry(QRect(140, 227, 66, 17));
        label4->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

//        retranslateUi(UpdaterForm);

        QMetaObject::connectSlotsByName(UpdaterForm);
    } // setupUi

/*
    void retranslateUi(QWidget *UpdaterForm)
    {
        UpdaterForm->setWindowTitle(QApplication::translate("UpdaterForm", "Updater", 0, QApplication::UnicodeUTF8));
        label1->setText(QApplication::translate("UpdaterForm", "Current Version", 0, QApplication::UnicodeUTF8));
        label2->setText(QApplication::translate("UpdaterForm", "New Version", 0, QApplication::UnicodeUTF8));
        label3->setText(QApplication::translate("UpdaterForm", "0", 0, QApplication::UnicodeUTF8));
        label4->setText(QApplication::translate("UpdaterForm", "0", 0, QApplication::UnicodeUTF8));
    } // retranslateUi
*/

};

namespace Ui {
    class UpdaterForm: public Ui_UpdaterForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UPDATERFORM_H
