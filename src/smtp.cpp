/*
Copyright (c) 2013 Raivis Strogonovs

http://morf.lv

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
* MODIFIED by Ivan Gavrilov
* https://github.com/discover1977/QtSMTP.git
*/

#include "smtp.h"

Smtp::Smtp(const QString &user, const QString &pass, const QString &host, int port, int timeout , QObject *parent)
    :QObject(parent)
{    
    socket = new QSslSocket(this);

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(socket, SIGNAL(connected()), this, SLOT(connected() ) );
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorReceived(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));


    this->user = user;
    this->pass = pass;

    this->host = host;
    this->port = port;
    this->timeout = timeout;
}



void Smtp::sendMail(const QString &from,
                    const QString &fromName,
                    const QStringList &to,
                    /*const QStringList &copyTo,*/
                    const QString &subject,
                    const QString &body,
                    QStringList files)
{
    m_rcptNum = 0;
    m_encoding = "UTF-8";
    QString str64;

    message = "";
    str64 = QByteArray().append(fromName).toBase64();
    message.append(QString("From: =?UTF-8?B?%1=?= <%2>\r\n").arg(str64).arg(from));

    str64 = QByteArray().append(subject).toBase64();
    message.append(QString("Subject: =?UTF-8?B?%1==?=\r\n").arg(str64));

    message.append("To: ");
    for (int i = 0; i < to.size(); i++) {
        message.append(QString("<%1>").arg(to.at(i)));
        if(i < (to.size() - 1)) message.append(", ");
    }
    message.append("\r\n");

//    message.append("Cc: ");
//    for (int i = 0; i < copyTo.size(); i++) {
//        message.append(QString("%1").arg(copyTo.at(i)));
//        if(i < (copyTo.size() - 1)) message.append(", ");
//    }
//    message.append("\r\n");

    if(!files.isEmpty()) {
        message.append("Content-Type: multipart/mixed; boundary=\"boundaryFiles\"\r\n\r\n");
        message.append("--boundaryFiles\r\n");
    }

    message.append("Content-Type: multipart/alternative; boundary=\"boundaryMailText\"\r\n\r\n");

    message.append("--boundaryMailText\r\n");
    message.append("Content-Type: text/plain; charset=\"UTF-8\"\r\n");
    message.append("Content-Transfer-Encoding: base64\r\n\r\n");

    str64 = QByteArray().append(body).toBase64();
    message.append(QString("%1\r\n").arg(str64));
    message.append("--boundaryMailText--\r\n");
    if(!files.isEmpty()) {
        qDebug() << "Files to be sent: " << files.size();
        foreach(QString filePath, files)
        {
            QFile file(filePath);
            if(file.exists()) {
                if (!file.open(QIODevice::ReadOnly)) {
                    qDebug("Couldn't open the file");
                    return ;
                }
                QByteArray bytes = file.readAll();
                message.append("--boundaryFiles\r\n");
                message.append( "Content-Type: application/octet-stream\nContent-Disposition: attachment; filename="+ QFileInfo(file.fileName()).fileName() +";\nContent-Transfer-Encoding: base64\n\n" );
                message.append(bytes.toBase64());
                message.append("\r\n");
            }
        }
        message.append("--boundaryFiles--\r\n");
    }
    else qDebug() << "No attachments found";

    this->from = from;
    rcpt = to.at(0);
    m_rcptList = to;
    state = Init;
    socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS
    if (!socket->waitForConnected(timeout)) {
         qDebug() << socket->errorString();
    }

    t = new QTextStream( socket );
}

Smtp::~Smtp()
{
    delete t;
    delete socket;
}
void Smtp::stateChanged(QAbstractSocket::SocketState socketState)
{

    qDebug() <<"stateChanged " << socketState;
}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
    qDebug() << "error " <<socketError;
}

void Smtp::disconnected()
{
    qDebug() <<"disconneted";
    qDebug() << "error "  << socket->errorString();
}

void Smtp::connected()
{    
    qDebug() << "Connected ";
}

void Smtp::readyRead()
{
     qDebug() <<"readyRead";
    // SMTP is line-oriented

    QString responseLine;
    do
    {
        responseLine = socket->readLine();
        response += responseLine;
    }
    while ( socket->canReadLine() && responseLine[3] != ' ' );

    responseLine.truncate( 3 );

    qDebug() << "Server response code:" <<  responseLine;
    qDebug() << "Server response: " << response;

    if ( state == Init && responseLine == "220" )
    {
        // banner was okay, let's go on
        *t << "EHLO localhost" <<"\r\n";
        t->flush();

        state = HandShake;
    }
    //No need, because I'm using socket->startClienEncryption() which makes the SSL handshake for you
    /*else if (state == Tls && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "STarting Tls";
        *t << "STARTTLS" << "\r\n";
        t->flush();
        state = HandShake;
    }*/
    else if (state == HandShake && responseLine == "250")
    {
        socket->startClientEncryption();
        if(!socket->waitForEncrypted(timeout))
        {
            qDebug() << socket->errorString();
            state = Close;
        }


        //Send EHLO once again but now encrypted

        *t << "EHLO localhost" << "\r\n";
        t->flush();
        state = Auth;
    }
    else if (state == Auth && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "Auth";
        *t << "AUTH LOGIN" << "\r\n";
        t->flush();
        state = User;
    }
    else if (state == User && responseLine == "334")
    {
        // Trying User
        qDebug() << "Username";
        // GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
        // https://developers.google.com/gmail/xoauth2_protocol
        *t << QByteArray().append(user).toBase64()  << "\r\n";
        t->flush();

        state = Pass;
    }
    else if (state == Pass && responseLine == "334")
    {
        // Trying pass
        qDebug() << "Pass";
        *t << QByteArray().append(pass).toBase64() << "\r\n";
        t->flush();

        state = Mail;
    }
    else if ( state == Mail && responseLine == "235" )
    {
        // HELO response was okay (well, it has to be)

        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        qDebug() << "MAIL FROM:<" << from << ">";
        *t << "MAIL FROM:<" << from << ">\r\n";
        t->flush();
        state = Rcpt;
    }
    else if ( state == Rcpt && (responseLine == "250" || responseLine == "251") )
    {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        if(m_rcptNum < m_rcptList.count()-1){
            *t << "RCPT TO: <" << m_rcptList.at(m_rcptNum) << ">\r\n";
            t->flush();
            state = Rcpt;
            qDebug() << "RCPT TO:<" << m_rcptList.at(m_rcptNum) << ">";
            m_rcptNum++;
        }else{
            *t << "RCPT TO: <" << m_rcptList.at(m_rcptNum) << ">\r\n";
            t->flush();
            qDebug() << "RCPT TO:<" << m_rcptList.at(m_rcptNum) << ">";
            state = Data;
        }
    }
    else if ( state == Data && responseLine == "250" )
    {

        *t << "DATA\r\n";
        t->flush();
        state = Body;
    }
    else if ( state == Body && responseLine == "354" )
    {

        *t << message << "\r\n.\r\n";
        t->flush();
        state = Quit;
    }
    else if ( state == Quit && responseLine == "250" )
    {

        *t << "QUIT\r\n";
        t->flush();
        // here, we just close.
        state = Close;
        emit status(tr("Message sent"));
    }
    else if (state == Close)
    {
        deleteLater();
        return;
    }
    else
    {
        // something broke.
        qDebug() << "Unexpected reply from SMTP server:\n\n" << response;
        // QMessageBox::warning( 0, tr( "Qt Simple SMTP client" ), tr( "Unexpected reply from SMTP server:\n\n" ) + response );
        state = Close;
        emit status(tr("Failed to send message"));
        deleteLater();
        return;
    }
    response = "";
}
