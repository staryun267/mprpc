#pragma once

#include <google/protobuf/service.h>
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include <functional>
#include <google/protobuf/descriptor.h>
#include <unordered_map>

//框架提供的专门发布rpc服务的网路对象类
class RpcProvider
{
public:
    //框架提供给外部的，可以发布rpc服务的函数接口
    void NotifyService(google::protobuf::Service* service);

    //启动rpc服务节点，开始提供rpc远程调用服务
    void Run();
private:
    void onConnection(const muduo::net::TcpConnectionPtr&);
    void onMessage(const muduo::net::TcpConnectionPtr&,muduo::net::Buffer*,muduo::Timestamp);

    //Closure的回调操作，用于序列化rpc的响应和发送
    void sendRpcResponse(const muduo::net::TcpConnectionPtr&,google::protobuf::Message*);

    //服务信息
    struct ServiceInfo
    {
        google::protobuf::Service* m_service;   //保存服务对象
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;  //保存服务方法
    };
    //存储注册成功的服务对象的所有信息
    std::unordered_map<std::string,ServiceInfo> m_serviceMap;
    //组合EventLoop
    muduo::net::EventLoop m_eventloop;
};