#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "../../src/smtp.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Smtp *m_smtp;

    QString m_emailSender;
    QString m_emailSenderName;
    QString m_emailSmtpServer;
    QString m_emailServerPassword;
    QStringList m_emailRecipient;
    QString m_emailSubject;
    QString m_emailMessage;
    QStringList m_emailFiles;

private slots:
    void slot_pbSendEmail();
    void slot_emailStatus(QString);
    void slot_AddFiles();
    void slot_RemoveFiles();
};

#endif // MAINWINDOW_H
