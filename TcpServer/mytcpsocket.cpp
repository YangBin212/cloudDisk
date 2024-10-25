#include "mytcpsocket.h"
#include <QDebug>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>

MyTcpSocket::MyTcpSocket()
{
    connect(this, SIGNAL(readyRead()),
            this, SLOT(recvMsg()));
    connect(this, SIGNAL(disconnected()),
            this, SLOT(clientOffline()));

    m_bUpload = false;

    m_pTimer = new QTimer;
    connect(m_pTimer, SIGNAL(timeout()),
            this, SLOT(sendFileToClient()));
}


QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDesDir)
{
    QDir dir;
    dir.mkdir(strDesDir);

    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();

    QString srcTmp;
    QString destTmp;
    for (int i = 0; i < fileInfoList.size(); i++)
    {
        if (fileInfoList[i].isFile())
        {
            srcTmp = strSrcDir + '/' + fileInfoList[i].fileName();
            destTmp = strDesDir + '/' + fileInfoList[i].fileName();
            QFile::copy(srcTmp, destTmp);
        }
        else if (fileInfoList[i].isDir())
        {
            if (QString(".") == fileInfoList[i].fileName()
                    || QString("..") == fileInfoList[i].fileName())
            {
                continue;
            }
            srcTmp = strSrcDir + '/' + fileInfoList[i].fileName();
            destTmp = strDesDir + '/' + fileInfoList[i].fileName();
            copyDir(srcTmp, destTmp);
        }
    }
}

void MyTcpSocket::recvMsg()
{
    if (!m_bUpload)
    {
    //    qDebug() << this->bytesAvailable();
        uint uiPDULen = 0;
        this->read((char*)&uiPDULen, sizeof(uint));
        uint uiMsgLen = uiPDULen - sizeof(PDU);
        PDU* pdu = mkPDU(uiMsgLen);
        this->read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint));
    //    char caName[32] = {'\0'};
    //    char caPwd[32] = {'\0'};
    //    strncpy(caName, pdu->caData, 32);
    //    strncpy(caPwd, pdu->caData + 32, 32);
    //    qDebug() << caName << caPwd << pdu->uiMsgType;
        switch (pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, REGIST_OK);
                QDir dir;
                dir.mkdir(QString("./%1").arg(caName));
            }
            else
            {
                strcpy(respdu->caData, REGIST_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, LOGIN_OK);
                m_strName = caName;
            }
            else
            {
                strcpy(respdu->caData, LOGIN_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_CANCEL_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            int ret = OpeDB::getInstance().handleUserCancel(caName, caPwd);
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_CANCEL_RESPOND;
            if (-1 == ret)
            {
                strcpy(respdu->caData, CANCEL_USER_NOT_EXIST);
            }
            else if (0 == ret)
            {
                strcpy(respdu->caData, CANCEL_PASSWORD_ERROR);
            }
            else if (1 == ret)
            {
                strcpy(respdu->caData, CANCEL_OK);
                QDir dir(QString("./%1").arg(caName));
                dir.removeRecursively();//删除用户文件夹
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
        {
            QStringList ret = OpeDB::getInstance().handleAllOnline();
            uint uiMsgLen = ret.size()*32;
            PDU* respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for (int i = 0; i < ret.size(); i++)
            {
                memcpy((char*)(respdu->caMsg) + i * 32,
                       ret.at(i).toStdString().c_str(),
                       ret.at(i).size());
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
        {
            int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if (-1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_NO);
            }
            else if (1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_ONLINE);
            }
            else if (0 == ret)
            {
                strcpy(respdu->caData, SEARCH_USR_OFFLINE);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32);
            strncpy(caName, pdu->caData + 32, 32);
            int ret = OpeDB::getInstance().handleAddFriend(caPerName, caName);
            PDU* respdu = nullptr;
            if (-1 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, UNKNOW_ERROR);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }
            else if (0 == ret)  //该好友已经存在
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, EXISTED_FRIEND);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }
            else if (1 == ret)
            {
                MyTcpServer::getInstance().resend(caPerName, pdu);
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_OK);//表示加好友请求已经发送
                write((char*)respdu, respdu->uiMsgLen);
                free(respdu);
                respdu = nullptr;
            }
            else if (2 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_OFFLINE);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }
            else if (3 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_NOEXIST);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char addedName[32] = {'\0'};
            char sourceName[32] = {'\0'};
            //拷贝读取的信息
            strncpy(addedName, pdu->caData, 32);
            strncpy(sourceName, pdu->caData + 32, 32);

            //将新的好友关系信息写入数据库
            OpeDB::getInstance().handleAddFriendAgree(addedName, sourceName);

            //服务器需要转发给发送好友请求方其被同意的消息
            MyTcpServer::getInstance().resend(sourceName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char sourceName[32] = {'\0'};
            //拷贝读取的信息
            strncpy(sourceName, pdu->caData + 32, 32);
            //服务器需要转发给发送好友请求方其被拒绝的消息
            MyTcpServer::getInstance().resend(sourceName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList ret = OpeDB::getInstance().handleFlushFriend(caName);
            uint uiMsgLen = ret.size() * 32;
            PDU* respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for (int i = 0; i < ret.size(); i++)
            {
                memcpy((char*)(respdu->caMsg) + i * 32, ret.at(i).toStdString().c_str(), ret.at(i).size());
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caSelName[32] = {'\0'};
            char caFriendName[32] = {'\0'};
            strncpy(caSelName, pdu->caData, 32);
            strncpy(caFriendName, pdu->caData + 32, 32);
            OpeDB::getInstance().handleDelFriend(caSelName, caFriendName);

            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData, DEL_FRIEND_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            MyTcpServer::getInstance().resend(caFriendName, pdu);

            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName, pdu->caData + 32, 32);
    //        qDebug() << caPerName;
            MyTcpServer::getInstance().resend(caPerName, pdu);

            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList onlineFriend = OpeDB::getInstance().handleFlushFriend(caName);
            QString tmp;
            for (int i = 0; i < onlineFriend.size(); ++i)
            {
                tmp = onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmp.toStdString().c_str(), pdu);
            }
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
        {
            QDir dir;
            QString strCurPath = QString("%1").arg((char*)(pdu->caMsg));
    //        qDebug() << strCurPath;
            bool ret = dir.exists(strCurPath);
            PDU* respdu = nullptr;
            if (!ret)   //当前目录不存在，需要创建当前目录，同时创建新目录
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData, DIR_NO_EXIST);
            }
            else        //当前目录存在，不需要创建当前目录，但需要创建新目录
            {
                char caNewDir[32] = {'\0'};
                memcpy(caNewDir, pdu->caData + 32, 32);
                QString strNewPath = strCurPath + "/" + caNewDir;
    //            qDebug() << strNewPath;
                ret = dir.exists(strNewPath);
    //            qDebug() << "-->" << ret;
                if (ret)    //创建的文件夹已经存在
                {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, DIR_NAME_EXIST);
                }
                else        //创建的文件夹不存在
                {
                    dir.mkdir(strNewPath);  //创建新的文件夹
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, DIR_CREATE_OK);
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_DIR_REQUEST:
        {
            char* pCurPath = new char[pdu->uiMsgLen];
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);
            QDir dir(pCurPath);
            QFileInfoList fileInfoList = dir.entryInfoList();
            int iFileCount = fileInfoList.size();
            PDU* respdu = mkPDU(sizeof(FileInfo) * iFileCount);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_DIR_RESPOND;
            FileInfo* pFileInfo = nullptr;
            QString strFileName;
            for (int i = 0; i < iFileCount; i++)
            {
    //            qDebug() << fileInfoList[i].fileName()
    //                     << fileInfoList[i].size()
    //                     << " 文件夹:" << fileInfoList[i].isDir()
    //                     << " 常规文件:" << fileInfoList[i].isFile();
                pFileInfo = (FileInfo*)(respdu->caMsg) + i;
                strFileName = fileInfoList[i].fileName();

                memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                if (fileInfoList[i].isDir())
                {
                    pFileInfo->iFileType = 0;   //0表示为文件夹
                }
                else if (fileInfoList[i].isFile())
                {
                    pFileInfo->iFileType = 1;   //1表示为常规文件
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_REQUEST:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);
    //        qDebug() << strPath;
            QFileInfo fileInfo(strPath);
            bool ret = false;
            if (fileInfo.isDir())
            {
                QDir dir;
                dir.setPath(strPath);
                ret = dir.removeRecursively();
            }
            else if (fileInfo.isFile()) //常规文件
            {
                ret = false;
            }
            PDU* respdu = nullptr;
            if (ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
            }
            else
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData, DEL_DIR_FAILED, strlen(DEL_DIR_FAILED));
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
        {
            char caOldName[32] = {'\0'};
            char caNewName[32] = {'\0'};
            strncpy(caOldName, pdu->caData, 32);
            strncpy(caNewName, pdu->caData + 32, 32);

            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);

            QDir dir;
            bool ret = dir.rename(strOldPath, strNewPath);
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, RENAME_FILE_OK);
            }
            else
            {
                strcpy(respdu->caData, RENAME_FILE_FAILED);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
        {
            char caEnterName[32] = {'\0'};
            strncpy(caEnterName, pdu->caData, 32);

            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName);

            QFileInfo fileInfo(strPath);
            PDU* respdu = nullptr;
            if (fileInfo.isDir())
            {
                QDir dir(strPath);
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount = fileInfoList.size();
                PDU* respdu = mkPDU(sizeof(FileInfo) * iFileCount);
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_DIR_RESPOND;
                FileInfo* pFileInfo = nullptr;
                QString strFileName;
                for (int i = 0; i < iFileCount; i++)
                {
                    pFileInfo = (FileInfo*)(respdu->caMsg) + i;
                    strFileName = fileInfoList[i].fileName();

                    memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                    if (fileInfoList[i].isDir())
                    {
                        pFileInfo->iFileType = 0;   //0表示为文件夹
                    }
                    else if (fileInfoList[i].isFile())
                    {
                        pFileInfo->iFileType = 1;   //1表示为常规文件
                    }
                }
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }
            else if (fileInfo.isFile())
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strncpy(respdu->caData, ENTER_FILE_FAILED, strlen(ENTER_FILE_FAILED));
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
            }

            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
//            qDebug() << strPath;
            delete []pPath;
            pPath = nullptr;

            m_file.setFileName(strPath);
            //以只写的方式打开文件，若文件不存在，则会自动创建文件
            if (m_file.open(QIODevice::WriteOnly))
            {
                m_bUpload = true;
                m_iTotal = fileSize;
                m_iReceived = 0;
            }

            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);
    //        qDebug() << strPath;
            QFileInfo fileInfo(strPath);
            bool ret = false;
            if (fileInfo.isDir())   //如果是文件夹
            {
                ret = false;
            }
            else if (fileInfo.isFile()) //如果是常规文件
            {
                QDir dir;
                ret = dir.remove(strPath);
            }
            PDU* respdu = nullptr;
            if (ret)    //删除成功
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData, DEL_FILE_OK, strlen(DEL_FILE_OK));
            }
            else        //删除失败
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData, DEL_FILE_FAILED, strlen(DEL_FILE_FAILED));
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            strcpy(caFileName, pdu->caData);
            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            delete []pPath;
            pPath = nullptr;

            QFileInfo fileInfo(strPath);
            qint64 fileSize = fileInfo.size();
            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            sprintf(respdu->caData, "%s %lld", caFileName, fileSize);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            m_file.setFileName(strPath);
            m_file.open(QIODevice::ReadOnly);
            m_pTimer->start(1000);

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        {
            char caSendName[32] = {'\0'};
            int num = 0;
            sscanf(pdu->caData, "%s %d", caSendName, &num);
            int size = num * 32;
            PDU* respdu = mkPDU(pdu->uiMsgLen-size);//路径所占空间大小
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strcpy(respdu->caData, caSendName);
            memcpy(respdu->caMsg, (char*)(pdu->caMsg) + size, pdu->uiMsgLen - size);

            char caReceiveName[32] = {'\0'};
            for (int i = 0; i < num; i++)//发送给共享的对象
            {
                memcpy(caReceiveName, (char*)(pdu->caMsg) + i * 32, 32);
                MyTcpServer::getInstance().resend(caReceiveName, respdu);
            }

            free(respdu);
            respdu= nullptr;

            //回复给发送方
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(respdu->caData, "share file ok");
            write((char*)respdu, respdu->uiPDULen);

            free(respdu);
            respdu = nullptr;

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND:
        {
            QString strReceivePath = QString("./%1").arg(pdu->caData);
            QString strShareFilePath = QString("%1").arg((char*)(pdu->caMsg));
            int index = strShareFilePath.lastIndexOf('/');
            QString strFileName = strShareFilePath.right(strShareFilePath.size() - index - 1);
            strReceivePath = strReceivePath + '/' + strFileName;

            QFileInfo fileInfo(strShareFilePath);
            if (fileInfo.isFile())//拷贝常规文件
            {
                QFile::copy(strShareFilePath, strReceivePath);
            }
            else if (fileInfo.isDir())
            {
                copyDir(strShareFilePath, strReceivePath);
            }
            break;
        }
        case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            int srcLen = 0;
            int destLen = 0;
            sscanf(pdu->caData, "%d %d %s", &srcLen, &destLen, caFileName);

            char* pSrcPath = new char[srcLen + 1];
            char* pDestPath = new char[destLen + 1 + 32];
            memset(pSrcPath, '\0', srcLen + 1);
            memset(pDestPath, '\0', destLen + 1 + 32);

            memcpy(pSrcPath, pdu->caMsg, srcLen);
            memcpy(pDestPath, (char*)(pdu->caMsg) + (srcLen + 1), destLen);

            PDU* respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
            QFileInfo fileInfo(pDestPath);
            if (fileInfo.isDir())
            {
                strcat(pDestPath, "/");
                strcat(pDestPath, caFileName);

                qDebug() << pDestPath;

                bool ret = QFile::rename(pSrcPath, pDestPath);//移动文件
                if (ret)
                {
                    strcpy(respdu->caData, MOVE_FILE_OK);
                }
                else
                {
                    strcpy(respdu->caData, COMMON_ERR);
                }
            }
            else if (fileInfo.isFile())
            {
                strcpy(respdu->caData, MOVE_FILE_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            break;
        }
        default:
        {
            break;
        }
        }
        free(pdu);
        pdu = nullptr;
    }
    else    //接收上传文件
    {
        QByteArray buff = readAll();    //每次读4096
        m_file.write(buff);
        m_iReceived += buff.size();
        if (m_iTotal == m_iReceived)
        {
            m_file.close();
            m_bUpload = false;

            //只有当接收完数据时，才respond
            PDU* respdu = nullptr;
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(respdu->caData, UPLOAD_FILE_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
        }
        else if (m_iTotal < m_iReceived)
        {
            m_file.close();
            m_bUpload = false;

            //只有当接收完数据时，才respond
            PDU* respdu = nullptr;
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strcpy(respdu->caData, UPLOAD_FILE_FAILED);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
        }
    }
}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    m_pTimer->stop();
    char* pData = new char[4096];
    qint64 ret = 0;
    while (true)
    {
        ret = m_file.read(pData, 4096);
        if (ret > 0 && ret <= 4096)
        {
            write(pData, ret);
        }
        else if (0 == ret)
        {
            m_file.close();
            break;
        }
        else if (ret < 0)
        {
            qDebug() << "发送文件给客户端过程中失败";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData = nullptr;
}
