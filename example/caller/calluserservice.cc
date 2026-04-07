#include <iostream>

#include "mprpcapplication.h"
#include "user.pb.h"

int main(int argc,char** argv)
{
    //启动程序后，想使用mprpc框架享受rpc调用服务，要先调用框架初始化函数，(只用调用一次)
    MprpcApplication::Init(argc,argv);

    //演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    //rpc方法的响应
    fixbug::LoginResponse response;
    //发起rpc方法的调用 同步的rpc调用过程
    MprpcController controller;
    stub.Login(&controller,&request,&response,nullptr);   //RpcChannel -> RpcChannel::CallMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    //一次rpc调用完成，读调用的结果
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if(response.result().errcode() == 0)
        {
            std::cout << "rpc login response: " << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}
