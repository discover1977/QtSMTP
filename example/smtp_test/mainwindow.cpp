#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pbSendEmail, SIGNAL(clicked()), this, SLOT(slot_pbSendEmail()));
    connect(ui->pbAddFiles, SIGNAL(clicked()), this, SLOT(slot_AddFiles()));
    connect(ui->pbRemoveFiles, SIGNAL(clicked()), this, SLOT(slot_RemoveFiles()));

    ui->lePassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    ui->pbRemoveFiles->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_pbSendEmail()
{
    QString str;

    m_emailSender = ui->leSender->text();
    m_emailServerPassword = ui->lePassword->text();
    m_emailSmtpServer = ui->leSmtpServer->text();

    m_emailSenderName = ui->leSenderName->text();

    str=ui->teRecipients->toPlainText();
    m_emailRecipient = str.split('\n');

    m_emailSubject = ui->leSubject->text();

    m_emailMessage=ui->teMessage->toPlainText();

    m_smtp = new Smtp(m_emailSender,
                      m_emailServerPassword,
                      m_emailSmtpServer,
                      465,
                      30000);

   connect(m_smtp, SIGNAL(status(QString)), this, SLOT(slot_emailStatus(QString)));

    m_smtp->sendMail(m_emailSender,
                     m_emailSenderName,
                     m_emailRecipient,
                     m_emailSubject,
                     m_emailMessage,
                     m_emailFiles);
}

void MainWindow::slot_emailStatus(QString statusStr)
{
    ui->lStatus->setText(statusStr);
}

void MainWindow::slot_AddFiles()
{
    ui->pbRemoveFiles->setEnabled(true);

    m_emailFiles.append(QFileDialog::getOpenFileNames(this, tr("Add files"), ""));

    for(int i = 0; i < m_emailFiles.size(); i++) {
        ui->teFiles->append(m_emailFiles.at(i));
    }
}

void MainWindow::slot_RemoveFiles()
{
    m_emailFiles.clear();
    ui->teFiles->clear();
    ui->pbRemoveFiles->setEnabled(false);
}
