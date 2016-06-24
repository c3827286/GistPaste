// NMTaskInfo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "NMTaskInfo.h"
#ifdef _WINDOWS_
#include <time.h>
#undef _WINDOWS_
#endif
///////////////////////////全局变量定义区/////////////////////////////////////

CThreadMutux thMutux;  //数据库连接池所用到锁
CDbPool dbPool;    //数据库连接池
SYS_INFO gSysInfo;   //全局配置变量
                     //CHttpSock httpSocket;
HANDLE handleMutex;

//typedef CDataQueue<Task_Info> TTaskInfo;
//TTaskInfo tTaskInfo(1000);
//TSendInfo *vSendInfo=new TSendInfo();
typedef CDataQueue<TSendInfo> QueueSendInfo;
QueueSendInfo m_QueueSendInfo(5);//建立5个队列
///////////////////////////END END END //////////////////////////////////////

/////////////////////////常量定义///////////////////////////////////////////

#define CFG_FILE "NMTaskInfoCfg.ini"
#define MAX_THREAD_NUM 256

/////////////////////////////////////////////////////////////////////////////

//////////////////////库文件////////////////////////////////////////////////

#pragma comment(lib, "odbc32.lib")

/////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
    printf("进入main\n");
    //预处理线程
    //HANDLE   _h_ProDataThread[MAX_THREAD_NUM];
    HANDLE   _h_ProDataThread;
    HANDLE  _h_GetDataThread;

    memset((void*) &gSysInfo, 0, sizeof(SYS_INFO));
    gSysInfo.nExitFlag = 0;

    ReadCfgInfo();

    handleMutex = ::CreateMutex(NULL, FALSE, (LPCWSTR) gSysInfo.szAppID);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        printf("应用程序已经在运行");
        return 0;
    }

    //数据库连接
    //dbPool.SetConnInfo("UID=tnm;PWD=tnmadmin;DSN=test15",gSysInfo.nDataConnNum);
    dbPool.SetConnInfo("UID=tnm;PWD=tnmadmin;DSN=test15", 10);

    if (dbPool.ConnectToPool() < 0)
    {
        printf("连接失败!");
        return 0;
    }


    //开始进行处理程序线程，该程序包含一个线程，当我们队列里面的某一个数据时间到了的时候，它就开始执行这个操作
    _h_GetDataThread = CreateThread(NULL, NULL, GetDataThread, (LPVOID) NULL, 0, NULL);
    if (_h_GetDataThread == NULL)
    {
        printf("创建get数据线程失败!\r\n");
        return 0;
    }
    //开始新建读取数据库的线程
    //for(int i=0;i<gSysInfo.nMaxThread;i++)
    //{
    //_h_ProDataThread[i] = CreateThread(NULL,NULL,ProDataThread,(LPVOID)NULL,0,NULL);
    //}
    _h_ProDataThread = CreateThread(NULL, NULL, ProDataThread, (LPVOID) NULL, 0, NULL);
    if (_h_ProDataThread == NULL)
    {
        printf("创建pro数据线程失败!\r\n");
        return 0;
    }
    while (!gSysInfo.nExitFlag)
    {
        dbPool.ReconnectDb();  //重连数据库
        Sleep(1000);
    }

    /*if(_h_GetDataThread!=NULL)
    {
    printf("close getdata \n");
    WaitForSingleObject( _h_GetDataThread, INFINITE);
    CloseHandle(_h_GetDataThread);
    }*/

    /*for(int i=0;i<gSysInfo.nMaxThread;i++)
    {
    if(_h_ProDataThread[i]!=NULL)
    {
    printf("close prodata \n");
    WaitForSingleObject( _h_ProDataThread[i], INFINITE);
    CloseHandle(_h_ProDataThread[i]);
    }
    }*/
    /*if(_h_ProDataThread!=NULL)
    {
    printf("close prodata \n");
    WaitForSingleObject( _h_ProDataThread[i], INFINITE);
    CloseHandle(_h_ProDataThread[i]);
    }*/

    dbPool.DisconnectToPool();
    return 0;
}


DWORD WINAPI GetDataThread(LPVOID pdata)
{

    char sql[1024];
    char szUpdateSql[1024];
    int nPool = -1;
    //Task_Info taskInfo;

    //1.定义两个变量.这两个应该定义得不够
    /*long nLogicID;
    char szSendContent[500];
    char szSendAddr[100];
    int  nLogicType;
    char szExecTime[32];*/

    double  ltmptask_id;    //任务ID
    double ltmpic_id;   //逻辑ID
                        //下面几个是配置时间用的
    char tmpSendType[3]; //采集类型 M按月 W按周 D按天 
    char tmpSendTime[20]; //确定是某一天（一般是按照星期，比如星期二、星期三）
    char tmpBeginTime[20];   //开始时间，如早上4:00
    char tmpEndTime[20];    //结束时间，如 早上10:00
                            //end of time config
    char tmpSendAdd[201]; //发送的地址
    char tmpContent[201]; //发送的内容
    int nLogicDataType;  //数据数据类型 10http   11FTP
    int ntmpIsCheck;   //是否校验
    int ntmpValid;   //是否有效

    while (!gSysInfo.nExitFlag)
    {
        printf("queue tail:%d\t", m_QueueSendInfo.rear);
        TSendInfo *vSendInfo = new TSendInfo();
        printf("进入getData线程\n");
        //数据提取
        /* if(tTaskInfo.GetQueueSize()>900)
        {
        Sleep(1000);
        continue;
        }*/
        if (m_QueueSendInfo.GetQueueSize() > 100)
        {
            Sleep(1000);
            continue;
        }
        otl_stream  otl_GetData;
        memset((void*) sql, 0, sizeof(sql));
        //sprintf(sql,"select logic_title from tnm_logic_data where rownum<100");

        sprintf(sql, "select tTask.Task_Id,tTask.Logic_Id,tTask.Send_Type,tTask.Send_Time,tTask.Start_Time,tTask.End_Time,tLogic.SEND_ADD,tLogic.Logic_Content,tLogic.LOGIC_DATA_TYPE,tLogic.Is_Check,tLogic.Is_Valid from tnm_logic_data tLogic, tnm_task_info tTask where tLogic.Logic_Id = tTask.logic_id and tLogic.tnm_ne_Id = tTask.ne_id and tLogic.is_valid = 1 and tTask.is_valid = 1 and tLogic.is_check = 1");

        try
        {
            __int64 nLogicID = 0;
            nPool = dbPool.GetDbFromPool();
            if (nPool == -1)
            {
                return nPool;
            }

            otl_GetData.open(100, sql, dbPool.tDbInfo[nPool].db);
            while (!otl_GetData.eof())
            {
                TSendInfo *vSendInfo = new TSendInfo();
                memset(tmpSendType, 0, sizeof(tmpSendType));
                memset(tmpSendTime, 0, sizeof(tmpSendTime));
                memset(tmpBeginTime, 0, sizeof(tmpBeginTime));
                memset(tmpEndTime, 0, sizeof(tmpEndTime));
                memset(tmpSendAdd, 0, sizeof(tmpSendAdd));
                memset(tmpContent, 0, sizeof(tmpContent));
                otl_GetData >> ltmptask_id >> ltmpic_id >> tmpSendType >> tmpSendTime >> tmpBeginTime >> tmpEndTime >> tmpSendAdd >> tmpContent >> nLogicDataType >> ntmpIsCheck >> ntmpValid;
                CUtil::trim(tmpSendType);
                CUtil::trim(tmpSendTime);
                CUtil::trim(tmpBeginTime);
                CUtil::trim(tmpEndTime);
                CUtil::trim(tmpSendAdd);
                CUtil::trim(tmpContent);
                vSendInfo->ntask_id = (__int64) ltmptask_id;
                vSendInfo->nlogic_id = (__int64) ltmpic_id;
                sprintf(vSendInfo->szSendType, "%s", tmpSendType);
                sprintf(vSendInfo->szSendTime, "%s", tmpSendTime);
                sprintf(vSendInfo->szBeginTime, "%s", tmpBeginTime);
                sprintf(vSendInfo->szEndtIime, "%s", tmpEndTime);
                sprintf(vSendInfo->szSendAdd, "%s", tmpSendAdd);
                sprintf(vSendInfo->szContent, "%s", tmpContent);
                vSendInfo->nLogicDataType = nLogicDataType;
                vSendInfo->nIsCheck = ntmpIsCheck;
                vSendInfo->nIsValid = ntmpValid;
                printf("执行完取得数据线程!\n");
                //将数据插入到队列中
                if (0 == m_QueueSendInfo.WriteDataToQueue(vSendInfo)) //写入成功，则
                {
                    printf("...正在将数据读入队列\r\n");
                    // delete vSendInfo;
                }
                else
                {
                    printf("...数据读入队列出错\r\n");
                    return -1;
                }
                //开始执行更新操作
                memset((void*) szUpdateSql, 0, sizeof(szUpdateSql));
                nLogicID = vSendInfo->nlogic_id;
                sprintf(szUpdateSql, "update tnm_task_info set is_valid=2 where logic_id=%lld", nLogicID);
                dbPool.ExecSQL(szUpdateSql); //执行更新操作
                delete vSendInfo;
            }
            //执行完成，更新数据库状态
            otl_GetData.close();
            dbPool.FreeDbToPool(nPool);
        }
        catch (otl_exception& p)
        {
            char szErrMsg[1024];
            memset((void*) szErrMsg, 0, sizeof(szErrMsg));
            sprintf(szErrMsg, "%s", (char*) p.msg);
            dbPool.tDbInfo[nPool].nConnFlag = 2;
            return (-2);
        }
        Sleep(500);
    }
    return 0;
}

DWORD WINAPI ProDataThread(LPVOID pdata)
{
    //TSendInfo *vSendInfo=new TSendInfo();
    printf("进入proData\n");
    char szUpdateSql[60];
    char szSql[600];
    int nPool = -1;
    char szTime[64];
    char szRecvData[1024];
    char szRetContent[1024]; //比对的信息
                             //各个数据段对应的值
    while (!gSysInfo.nExitFlag)
    {
        TSendInfo *pSendInfo = new TSendInfo();
        static int i = 0;
        __int64 nLogicId;
        __int64 lcheckId;
        otl_stream otl_ProData;
        int iFindRes;
        memset(szSql, 0, sizeof(szSql));
        //线程运行，不断的取出 需要执行的任务
        sprintf(szSql, "select t.task_id as counts from tnm_task_info t where (t.is_valid = 1 and to_date(t.start_time,'yyyy-mm-dd hh24:mi:ss') <= sysdate and t.start_time is not null) and(to_date(t.end_time,'yyyy-mm-dd hh24:mi:ss')  >= sysdate and  t.end_time is not null) and ((t.send_type = 'D' or (t.send_type = 'M' and t.send_time = to_char(sysdate,'DD')) or (t.send_type = 'W' and t.send_time = to_char(to_number(to_char(sysdate,'D'))-1)))) order by t.insert_dt");
        //开始执行
        try
        {
            nPool = dbPool.GetDbFromPool();
            if (nPool == -1)
            {
                return nPool;
            }
            otl_ProData.open(100, szSql, dbPool.tDbInfo[nPool].db);
            while (!otl_ProData.eof())
            {
                //如果取出的数据不为空
                otl_ProData >> nLogicId;
                if (nLogicId > 0)
                {
                    //开始进行校验
                    do
                    {
                        //m_QueueSendInfo.ReadNextDataFromQueue(pSendInfo,i++);
                        iFindRes = m_QueueSendInfo.ReadDataFromQueue(pSendInfo);
                    } while (pSendInfo->nlogic_id != nLogicId);
                    //{
                    //开始执行发送等操作
                    //判断
                    if (iFindRes == 0)
                    {
                        if (10 == (pSendInfo->nLogicDataType))
                        {
                            /*
                            httpSocket.InitEnv(NULL,vSendInfo->szSendAdd,80); //默认为80端口
                            httpSocket.SetHttpTimeOut(5000000);
                            if(httpSocket.SendPostData(vSendInfo->szContent))
                            {
                            printf("发送数据失败\n\r");
                            }
                            else
                            {
                            //取得返回的数据
                            memset(szRecvData,0,sizeof(szRecvData));
                            httpSocket.GetHeadData(szRecvData); //此时szRecvData保存服务器返回的数据
                            //有数据返回
                            if(strlen(szRecvData)>0)
                            {
                            //搜索数据库
                            otl_stream otl_logicCheck;
                            memset((void*)szSql,0,sizeof(szSql));
                            //sprintf(szSql,"select ret_content from tnm_logic_check where logic_id=%s and is_valid=1",vSendInfo->nlogic_id);
                            sprintf(szSql,"select c1.check_id from tnm_log_check c1 where check_id not in(select c2.check_id from tnm_logic_check c2  where re_content like '%%s%' and  c1.logic_id=c2.logic_id and c2.logic_id=%d and c2.is_vaild=1",szRecvData,pSendInfo->nlogic_id);
                            try
                            {
                            nPool=dbPool.GetDbFromPool();
                            if(-1==nPool)
                            {
                            Sleep(2000);
                            return nPool;
                            }
                            else
                            {
                            otl_logicCheck.open(100,szSql,dbPool.tDbInfo[nPool].db);
                            if(!otl_logicCheck.eof()) //logic_id 一定要是唯一的，不然这里就会有错误
                            {
                            //说明有出错的情况存在
                            otl_logicCheck>>lcheckId;
                            }
                            else
                            {
                            printf("检验未出错\n\r");
                            break;
                            }

                            //进行消息校验，校验通过的时候就把任务采集表的数据更新为1
                            //将数据插入到 报警接口表当中，暂时未做
                            memset(szSql,0,sizeof(szSql));
                            //返回的消息与校验库的消息一致
                            if(strcmp(szRetContent,szRecvData))
                            {
                            memset((void*)szUpdateSql,0,sizeof(szUpdateSql));
                            sprintf(szUpdateSql,"update tnm_task_info set is_valid=1,LAST_UPDATE_TIME=to_char(sysdate,'yyy-mm-dd hh24:mi:ss') where logic_id=%ld",vSendInfo->nlogic_id);
                            dbPool.ExecSQL(szUpdateSql); //执行更新操作
                            }
                            }
                            }
                            catch(otl_exception &p)
                            {
                            char szMsg[1024];
                            memset((void*)szMsg,0,sizeof(szMsg));
                            sprintf(szMsg,"%s",(char*)p.msg);
                            dbPool.tDbInfo[nPool].nConnFlag=2;
                            return -2;
                            }
                            }
                            }*/
                        }
                        else if (11 == (int) (pSendInfo->szSendType))
                        {
                            //进行Ftp的发送操作
                            printf("执行FTP操作\n");

                        }
                        //} //ZHANGXQ
                    }
                    else
                    {
                        printf("当前没有任务需要进行\n");
                        Sleep(500);
                    }
                }

            }
            ////TSendInfo *pSendInfo=new TSendInfo();
            ////第一步：取出时间，判断当前某一个时间段该执行操作，如果有，就发送并等待返回。返回的时候再把它和
            //memset(szTime,0,sizeof(szTime));
            ///*szTime=CUtil::getTime();*/
            //strcpy(szTime,CUtil::getTime());
            ////第二步：判断时间是否到了
            //m_QueueSendInfo.ReadDataFromQueue(vSendInfo); //获得队列的首元素
            ////时间到了


            //执行Sql语句判断是否有值
            //if(strcmp(vSendInfo->szExecTime,szTime))
            //{
            //为HTTP处理类型
            if (10 == (pSendInfo->nLogicDataType))
            {
                /*
                httpSocket.InitEnv(NULL,vSendInfo->szSendAdd,80); //默认为80端口
                httpSocket.SetHttpTimeOut(5000000);
                if(httpSocket.SendPostData(vSendInfo->szContent))
                {
                printf("发送数据失败\n\r");
                }
                else
                {
                //取得返回的数据
                memset(szRecvData,0,sizeof(szRecvData));
                httpSocket.GetHeadData(szRecvData); //此时szRecvData保存服务器返回的数据
                //有数据返回
                if(strlen(szRecvData)>0)
                {
                //搜索数据库
                otl_stream otl_logicCheck;
                memset((void*)szSql,0,sizeof(szSql));
                sprintf(szSql,"select ret_content from tnm_logic_check where logic_id=%s and is_valid=1",vSendInfo->nlogic_id);
                try
                {
                nPool=dbPool.GetDbFromPool();
                if(-1==nPool)
                {
                Sleep(2000);
                return nPool;
                }
                else
                {
                otl_logicCheck.open(100,szSql,dbPool.tDbInfo[nPool].db);
                while(!otl_logicCheck.eof()) //logic_id 一定要是唯一的，不然这里就会有错误
                {
                otl_logicCheck>>szRetContent;
                }
                //进行消息校验，校验通过的时候就把任务采集表的数据更新为1
                if(strlen(szRetContent))
                {
                //返回的消息与校验库的消息一致
                if(strcmp(szRetContent,szRecvData))
                {
                memset((void*)szUpdateSql,0,sizeof(szUpdateSql));
                sprintf(szUpdateSql,"update tnm_task_info set is_valid=1 where logic_id=%ld",vSendInfo->nlogic_id);
                dbPool.ExecSQL(szUpdateSql); //执行更新操作
                }
                }
                }
                }
                catch(otl_exception &p)
                {
                char szMsg[1024];
                memset((void*)szMsg,0,sizeof(szMsg));
                sprintf(szMsg,"%s",(char*)p.msg);
                dbPool.tDbInfo[nPool].nConnFlag=2;
                return -2;
                }
                }
                }*/
            }
            //} //zhangxq
            /*Sleep(10);
            otl_stream otl_getTaskData;
            memset((void*)szSql,0,sizeof(szSql));*/  //为何要变成空指针
            //sprintf(szSql,"select task_id,logic_id,ne_id,exec_time,is_valid from tnm_task_info where rownum<2");
            //开始执行
            //1.判断是否有空闲的数据库连接
            /*
            try
            {
            nPool=dbPool.GetDbFromPool();
            if((-1)==nPool)
            {
            Sleep(1000);
            return nPool;
            }
            else
            {
            //2.开始执行数据库操作
            otl_getTaskData.open(100,szSql,dbPool.tDbInfo[nPool].db);
            //3.获取数据库的值
            while(!otl_getTaskData.eof())
            { //1.取出数据的值，注意数据类型要严格匹配
            long nTmpTaskID,nTmpLogicID,ntmpNeID;
            otl_getTaskData>>nTmpTaskID;
            //otl_getTaskData>>tmpTaskID>>tmpLogicID>>tmpNeID>>tmpExecTime>>nIsValid;
            //将这些数据添加到数据单元中
            int i=0;
            //itoa(nTmpTaskID,tmpTaskID,10); //把整形转为char*
            sprintf(tmpTaskID,"%ld",nTmpTaskID);
            Task_Info *vTaskInfo=new Task_Info;
            memset(vTaskInfo->szTaskID,0,sizeof(vTaskInfo->szTaskID));
            memset(vTaskInfo->szLogicID,0,sizeof(vTaskInfo->szLogicID));
            memset(vTaskInfo->szNeID,0,sizeof(vTaskInfo->szNeID));
            memset(vTaskInfo->)
            memset(vTaskInfo->szExecTime,0,sizeof(vTaskInfo->szExecTime));
            strcpy(vTaskInfo->szTaskID,tmpTaskID);
            strcpy(vTaskInfo->szLogicID,tmpLogicID);
            strcpy(vTaskInfo->szExecTime,tmpExecTime);
            strcpy(vTaskInfo->szNeID,tmpNeID);
            vTaskInfo->nIsValid=nIsValid;

            CUtil::trim(vTaskInfo->szTaskID);
            CUtil::trim(vTaskInfo->szLogicID);
            CUtil::trim(vTaskInfo->szExecTime);
            CUtil::trim(vTaskInfo->szNeID);
            //添加节点
            tTaskInfo.WriteDataToQueue(vTaskInfo);
            delete vTaskInfo;
            }
            //把数据放置到双向队列之后，我们将有处理的那些的标示更为成2
            //printf("操作结果:%s",szTaskData);
            }
            }*/

            otl_ProData.close();
            dbPool.FreeDbToPool(nPool);
            Sleep(100);
        }
        catch (otl_exception &p)
        {

        }
        delete pSendInfo;
    }
    return 0;
}

int ReadCfgInfo()
{
    int nRetCode = 0;
    char szTmp[20];
    //DataBase
    nRetCode = ReadConfig(CFG_FILE, "[Database]", "connstr", gSysInfo.szConnData);

    //ConnNum
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[Database]", "ConnNum", szTmp);
    gSysInfo.nDataConnNum = atoi(szTmp);
    if (gSysInfo.nDataConnNum <= 0)
        gSysInfo.nDataConnNum = 1;

    //AppTitle
    nRetCode = ReadConfig(CFG_FILE, "[App]", "AppTitle", gSysInfo.szAppTitle);
    nRetCode = ReadConfig(CFG_FILE, "[App]", "AppID", gSysInfo.szAppID);

    //
    //ConnTimeOut
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[TimeOut]", "ConnTimeOut", szTmp);
    gSysInfo.nConnTimeOut = atoi(szTmp);
    if (gSysInfo.nConnTimeOut <= 0)
        gSysInfo.nConnTimeOut = 20;

    //SendTimeOut
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[TimeOut]", "SendTimeOut", szTmp);
    gSysInfo.nSendTimeOut = atoi(szTmp);
    if (gSysInfo.nSendTimeOut <= 0)
        gSysInfo.nSendTimeOut = 20;

    //SendTimeOut
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[TimeOut]", "RecvTimeOut", szTmp);
    gSysInfo.nRecvTimeOut = atoi(szTmp);
    if (gSysInfo.nRecvTimeOut <= 0)
        gSysInfo.nRecvTimeOut = 20;

    //Thread
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[Thread]", "MAX_THREAD_NUM", szTmp);
    gSysInfo.nMaxThread = atoi(szTmp);
    if (gSysInfo.nMaxThread <= 0)
        gSysInfo.nMaxThread = 1;
    if (gSysInfo.nMaxThread > MAX_THREAD_NUM)
        gSysInfo.nMaxThread = MAX_THREAD_NUM;



    nRetCode = ReadConfig(CFG_FILE, "[Log]", "LogPath", gSysInfo.szLogPath);
    //SendTimeOut
    memset((void*) szTmp, 0, sizeof(szTmp));
    nRetCode = ReadConfig(CFG_FILE, "[Log]", "LogLevel", szTmp);
    gSysInfo.nLogLevel = atoi(szTmp);
    if (gSysInfo.nLogLevel <= 0)
        gSysInfo.nLogLevel = 1;


    return 0;
}

int ReadConfig(char *file, char* title, char* token, char* value)
{
    char  buffer[1024];
    char  *readpoint, *ptr;
    char  tempstr[256];
    FILE  *fp;
    int   i;
    int   length;

    memset(tempstr, 0, 256);
    memset(buffer, 0, 1024);
    if ((fp = fopen(file, "r")) == NULL)
    {
        // printf("无法打开客户的配置文件,程序终止!");
        return -1;
    }
    fseek(fp, 0l, SEEK_SET);
    i = fread(buffer, 1024, 1, fp);
    ptr = buffer;
    readpoint = (char*) strstr((const char *) ptr, title);
    if (readpoint != NULL)
    {
        ptr = readpoint + strlen(title);
        readpoint = (char*) strstr((const char *) ptr, token);
        if (readpoint != NULL)
        {
            length = strlen(token);
            for (i = 0; i < 256; i++)
            {
                if (*(readpoint + length + i) != '\n')
                {
                    tempstr[i] = *(readpoint + length + i);
                }
                else
                {
                    tempstr[i] = '\0';
                    break;
                }
            }
        }
    }
    strcpy(value, tempstr);
    fclose(fp);
    return 0;
}
