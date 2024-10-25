#include "opedb.h"
#include <QMessageBox>
#include <QDebug>

OpeDB::OpeDB(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setHostName("localhost");
    m_db.setDatabaseName("E:\\QtProject\\TcpServer\\cloud.db");
    if (m_db.open())
    {
//        QSqlQuery query;
//        query.exec("select * from usrInfo");
//        while (query.next())
//        {
//            QString data = QString("%1,%2,%3").arg(query.value(0).toString()).arg(query.value(1).toString()).arg(query.value(2).toString());
//            qDebug() << data;
//        }
        qDebug() << "打开数据库成功！";
    }
    else
    {
        QMessageBox::critical(NULL, "打开数据库", "打开数据库失败！");
    }
}

OpeDB::~OpeDB()
{
    m_db.close();
}

bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd)
    {
//        qDebug() << "name | pwd is NULL";
        return false;
    }
    QString data = QString("insert into usrInfo(name, pwd) values(\'%1\',\'%2\')").arg(name).arg(pwd);
//    qDebug() << data;
    QSqlQuery query;
    return query.exec(data);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd)
    {
//        qDebug() << "name | pwd is NULL";
        return false;
    }
    QString data = QString("select * from usrInfo where name = \'%1\' and pwd = \'%2\' and online = 0").arg(name).arg(pwd);
//    qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        data = QString("update usrInfo set online = 1 where name = \'%1\' and pwd = \'%2\'").arg(name).arg(pwd);
        QSqlQuery query;
        query.exec(data);
        return true;
    }
    else
    {
        return false;
    }
}

int OpeDB::handleUserCancel(const char *name, const char *pwd)
{
    if (NULL == name || NULL == pwd)
    {
        return 2;
    }
    QString data = QString("select * from usrInfo where name = \'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next())//存在该用户名，继续验证密码
    {
        data = QString("select * from usrInfo where name = \'%1\' and pwd = \'%2\'").arg(name).arg(pwd);
        query.exec(data);
        if (query.next())//用户名和密码均正确
        {
            data = QString("delete from usrInfo where name = \'%1\' and pwd = \'%2\'").arg(name).arg(pwd);
            query.exec(data);//在数据库删除该用户信息
            return 1;//返回1表示注销成功
        }
        else//密码错误
        {
            return 0;//返回0表示密码错误
        }
    }
    else//用户不存在
    {
        return -1;//返回-1表示用户不存在
    }
}

void OpeDB::handleOffline(const char *name)
{
    if (NULL == name)
    {
//        qDebug() << "name is NULL";
        return;
    }
    QString data = QString("update usrInfo set online = 0 where name = \'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
}

QStringList OpeDB::handleAllOnline()
{
    QString data = QString("select name from usrInfo where online = 1");
    QSqlQuery query;
    query.exec(data);
    QStringList result;
    result.clear();
    while (query.next())
    {
        result.append(query.value(0).toString());
    }
    return result;
}

int OpeDB::handleSearchUsr(const char *name)
{
    if (nullptr == name)
    {
        return -1;
    }
    QString data = QString("select online from usrInfo where name = \'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        int ret = query.value(0).toInt();
//        if (1 == ret)
//        {
//            return 1;
//        }
//        else if (0 == ret)
//        {
//            return 0;
//        }
        return ret;
    }
    else
    {
        return -1;
    }
}

int OpeDB::handleAddFriend(const char *pername, const char *name)
{
    if (nullptr == pername || nullptr == name)
    {
        return -1;
    }
    QString data = QString("select * from friend where (id = (select id from usrInfo where name = \'%1\') and friendId = (select id from usrInfo where name = \'%2\')) "
                           "or (id = (select id from usrInfo where name = \'%3\') and friendId = (select id from usrInfo where name = \'%4\'))").arg(pername).arg(name).arg(name).arg(pername);
    qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        return 0;   //双方已经是好友
    }
    else    //查询是否在线
    {
        QString data = QString("select online from usrInfo where name = \'%1\'").arg(pername);
        QSqlQuery query;
        query.exec(data);
        if (query.next())
        {
            int ret = query.value(0).toInt();
            if (1 == ret)
            {
                return 1;   //在线
            }
            else if (0 == ret)
            {
                return 2;   //不在线
            }
        }
        else
        {
            return 3;       //不存在这个用户
        }
    }
}

int OpeDB::getIdByUserName(const char *name)
{
    if (nullptr == name)
    {
        return -1;
    }
    QString data = QString("select id from usrInfo where name = \'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
//    qDebug() << data;
    if (query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        return -1;//不存在用户
    }
}

void OpeDB::handleAddFriendAgree(const char *addedName, const char *sourceName)
{
    if (nullptr == addedName || nullptr == sourceName)
    {
        return;
    }
    int sourceUserId = -1;
    int addedUserId = -1;
    sourceUserId = getIdByUserName(sourceName);
    addedUserId = getIdByUserName(addedName);
//    qDebug() << sourceUserId << " " << addedUserId;
    QString strQuery = QString("insert into friend values(%1, %2)").arg(sourceUserId).arg(addedUserId);
    QSqlQuery query;
    query.exec(strQuery);
    return;
}

QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    if (nullptr == name)
    {
        return strFriendList;
    }
    QString strQuery = QString("select name from usrInfo where online = 1 and id in (select id from friend where friendId in (select id from usrInfo where name = \'%1\'))").arg(name);
    QSqlQuery query;
    query.exec(strQuery);
    while (query.next())
    {
        strFriendList.append(query.value(0).toString());
//        qDebug() << query.value(0).toString();
    }
    strQuery = QString("select name from usrInfo where online = 1 and id in (select friendId from friend where id in (select id from usrInfo where name = \'%1\'))").arg(name);
    query.exec(strQuery);
    while (query.next())
    {
        strFriendList.append(query.value(0).toString());
//        qDebug() << query.value(0).toString();
    }
    return strFriendList;
}

bool OpeDB::handleDelFriend(const char *name, const char *friendName)
{
    if (nullptr == name || nullptr == friendName)
    {
        return false;
    }
    QString strQuery = QString("delete from friend where id = (select id from usrInfo where name = \'%1\') and friendId = (select id from usrInfo where name = \'%2\')").arg(name).arg(friendName);
    QSqlQuery query;
    query.exec(strQuery);
    strQuery = QString("delete from friend where friendId = (select id from usrInfo where name = \'%1\') and id = (select id from usrInfo where name = \'%2\')").arg(name).arg(friendName);
    query.exec(strQuery);

    return true;
}
