#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"

//框架提供给外部的，可以发布rpc服务的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service)
{
    ServiceInfo service_info;
    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor* pserviceDesc = service->GetDescriptor();
    //获取服务的名字
    std::string service_name = pserviceDesc->name();
    //获取服务对象service方法数量
    int methodCnt = pserviceDesc->method_count();

    LOG_INFO("service_name: %s",service_name.c_str());

    for(int i=0;i<methodCnt;i++)
    {
        //获取服务对象指定下标的服务方法的描述(抽象描述)
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name,pmethodDesc});

        LOG_INFO("method_name: %s",method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name,service_info});
}

//启动rpc服务节点，开始提供rpc远程调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port);
    
    muduo::net::TcpServer server(&m_eventloop,address,"RpcProvider");

    server.setConnectionCallback(
        std::bind(&RpcProvider::onConnection,this,std::placeholders::_1)
    );

    server.setMessageCallback(
        std::bind(&RpcProvider::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
    );

    server.setThreadNum(4);

    //把当前rpc节点想要发布的服务全部注册到zk上，让rpc client可以可以从zk上发现服务
    zkClient zkCli;
    zkCli.Start();

    //sevice_name为永久性节点，method_name为临时性节点
    for(auto& sp : m_serviceMap)
    {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(),nullptr,0);
        for(auto& mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    LOG_INFO("RpcProvider start server at ip: %s ,port: %hu",ip.c_str(),port);
    
    server.start();
    m_eventloop.loop();
}

void RpcProvider::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if(!conn->connected())
    {
        //rpc和client的连接断开
        conn->shutdown();
    }
}

/*
在框架内部RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
name_size service_name method_name args_size args   定义proto的message类型，进行数据头的序列化和反序列化
*/
void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buffer,
                            muduo::Timestamp time)
{
    //网络上接收的远程rpc服务调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    //从字符流中读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size,4,0);

    //进行反序列化，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcheader;

    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcheader.ParseFromString(rpc_header_str))
    {
        //反序列化成功
        service_name = rpcheader.service_name();
        method_name = rpcheader.method_name();
        args_size = rpcheader.args_size();
    }
    else
    {
        //反序列化失败
        //std::cout << "rpc_header_str: " << rpc_header_str << "parse error!" << std::endl;
        LOG_ERROR("rpc_header_str: %s  parse error!",rpc_header_str.c_str());
        return;
    }

    //获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4+header_size,args_size);

    //打印调试信息
    // std::cout << "=======================" << std::endl;
    // std::cout << "header_size:  " << header_size << std::endl;
    // std::cout << "rpc_header_str:  " << rpc_header_str << std::endl;
    // std::cout << "service_name:  " << service_name << std::endl;
    // std::cout << "method_name:  " << method_name << std::endl;
    // std::cout << "args_str:  " << args_str << std::endl;
    // std::cout << "=======================" << std::endl;

    //获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end())
    {
        //std::cout << service_name << "is not exist!" << std::endl;
        LOG_ERROR("%s is not exist!",service_name.c_str());
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        //std::cout << service_name << " : " << method_name << "is not exist!" << std::endl;
        LOG_ERROR("%s : %s is not exist!",service_name.c_str(),method_name.c_str());
        return;
    }

    google::protobuf::Service* service = it->second.m_service;  //获取service对象
    const google::protobuf::MethodDescriptor* method = mit->second; //获取method对象

    //生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        //std::cout << "request parse error, content: " << args_str << std::endl;
        LOG_ERROR("request parse error, content: %s",args_str.c_str());
        return;
    }

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    //给下面的method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,&RpcProvider::sendRpcResponse,conn,response);

    //在框架上根据远端的rpc请求，调用当前rpc节点上发布的方法
    service->CallMethod(method,nullptr,request,response,done);
}

//Closure的回调操作，用于序列化rpc的响应和发送
void RpcProvider::sendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{
    std::string response_str;
    if(response->SerializeToString(&response_str))  //response序列化
    {
        //序列化成功，发送
        conn->send(response_str);
        conn->shutdown();   //模拟http的短链接服务，由rpcprovider主动断开连接
    }
    else
    {
        //std::cout << "serialize response_str error!" << std::endl;
        LOG_ERROR("serialize response_str error!");
    }
    conn->shutdown();
}