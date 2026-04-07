#include <iostream>
#include <string>

#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"

//UserService原来是一个本地服务，提供了两个进程内的本地方法Login和GetFriendLists
class UserService : public fixbug::UserServiceRpc   //rpc服务提供者
{
public:
    bool Login(std::string name,std::string pwd)
    {
        std::cout << "do local service login: " << std::endl;
        std::cout << "name: " << name << "  password: " << pwd << std::endl;
        return true;
    }

    //重写基类UserServiceRpc基类的虚函数，下面这些方法都是框架直接调用的
    /*
    1.caller -> Login(LoginRequest) -> muduo -> callee
    2.callee -> 下面的方法
    */
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        //框架给业务上报了请求参数LoninRequest，应用获取响应的数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        //做本地业务
        bool login_result = Login(name,pwd);

        //把响应写入
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调操作
        done->Run();
    }
};

int main(int argc,char** argv)
{
    //LOG_INFO("first log message!");
    //LOG_ERROR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);

    //调用框架的初始化操作
    MprpcApplication::Init(argc,argv);

    //把UserService发布到rpc节点
    RpcProvider provider;
    provider.NotifyService(new UserService());

    //启动一个rpc发布节点 Run以后进入阻塞状态，等待远程的rpc调用请求
    provider.Run();
    return 0;
}