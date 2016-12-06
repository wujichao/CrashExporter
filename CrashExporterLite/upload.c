#include <qiniu/qiniu/io.h>
#include <qiniu/qiniu/resumable_io.h>
#include <qiniu/qiniu/rs.h>
#include <qiniu/qiniu/base.h>

#include <stdio.h>

int _uploadFile(Qiniu_Client* pClient, const char *uploadName, const char *pLocalFilePath, char **result, char **err)
{
    printf("start upalod, %s => %s\n", pLocalFilePath, uploadName);

    const char* uploadtoken = "1CquS-wW66-Mf_Bg6RQv5Iz0SxSjLf82wDwNClLM:auF2ND-jQzZ0uC1QuQxTAoFUnrA=:eyJzY29wZSI6ImhwY2xpZW50IiwiZGVhZGxpbmUiOjE4MDkxODQ2ODB9";

    Qiniu_Io_PutRet putRet;
    Qiniu_Error error = Qiniu_Io_PutFile(pClient, &putRet, uploadtoken, uploadName, pLocalFilePath, NULL);
    if (error.code != 200)
    {
        printf("Upload File %s To %s error. error: %s\n", pLocalFilePath, uploadName, error.message);
        *err = strdup(error.message);
    }
    else
    {
        printf("Upload File %s To %s success. hash: %s\n", pLocalFilePath, uploadName, putRet.hash);
        *result = strdup(putRet.hash);
    }
    return error.code;
}

int uploadFile(const char *uploadName, const char *loaclFilePath, char **result, char **error)
{
    Qiniu_Client client;

    Qiniu_Global_Init(-1);                  /* 全局初始化函数，整个进程只需要调用一次 */
    Qiniu_Client_InitNoAuth(&client, 1024); /* HTTP客户端初始化。HTTP客户端是线程不安全的，不要在多个线程间共用 */

    int code = _uploadFile(&client, uploadName, loaclFilePath, result, error);

    Qiniu_Client_Cleanup(&client);          /* 每个HTTP客户端使用完后释放 */
    Qiniu_Global_Cleanup();                 /* 全局清理函数，只需要在进程退出时调用一次 */

    return code == 200 ? 0 : code;
}
