#include "WinServerWithLibEvent.h"
#include "DataTypes.h"
#include "WinhttpAPI.h"

using std::string;
using std::vector;

//define function
void fn_Output(int msgType,const string &msg,unsigned long error);
int fn_ParseJson(const string &Json, stTgApiStruct &tgApiStruct);
void fn_ProcessTask(stTgApiStruct *tgApiStruct);
void server_cb(evhttp_request *request,void *);

int main(){
#if _DEBUG

#else
    cHttpServer httpServer{};
    fn_Output(1,"Server Start",0);
    httpServer.startServer("0.0.0.0",80,server_cb);
    httpServer.stopServer();
    fn_Output(1,"Server Done",0);
#endif
    return 0;
}
//receiving data from the server
void server_cb(evhttp_request *request,void *){
    if (evhttp_request_get_command(request) == EVHTTP_REQ_POST){
        evkeyvalq *ptr_KeyValue = evhttp_request_get_input_headers(request);
        if (strcmp(evhttp_find_header(ptr_KeyValue,"Host"),"theeiffeltower.ml") == 0){
            //read payload
            string payload;
            evbuffer *evBuffer = evhttp_request_get_input_buffer(request);
            while (evbuffer_get_length(evBuffer) > 0){
                char buf[4096] {0};
                int n = evbuffer_remove(evBuffer,buf, sizeof(buf) - 1);
                if (n > 0)
                    payload.append(buf);
            }

            //parse json
            stTgApiStruct tgApiStruct{};
            if (fn_ParseJson(payload,tgApiStruct) == -1)
                fn_Output(-1,"Json parse error",0);

            //response async
            std::future f = std::async(fn_ProcessTask,&tgApiStruct);

            evhttp_send_reply(request,HTTP_OK,"OK", nullptr);
            return;
        }
    }
    evhttp_send_reply(request,HTTP_NOTFOUND,"Not Found", nullptr);
}
//do and finish task
void fn_ProcessTask(stTgApiStruct *tgApiStruct){
    printf("UserID:%lu ,Command:%d\n",tgApiStruct->UserID,tgApiStruct->command);
    stHttpResponse httpResponse;
    stHttpRequest httpRequest;

    httpRequest.Url = "http://api.btstu.cn/sjbz/?lx=dongman";
    httpRequest.Model = "get";
    cWinHttpSimpleAPI::Winhttp_Request(httpRequest,httpResponse);
    httpRequest.Url = cWinHttpSimpleAPI::Winhttp_GetHeaders(httpResponse,"Location");
    cWinHttpSimpleAPI::Winhttp_Request(httpRequest,httpResponse);


}
int fn_ParseJson(const string &Json, stTgApiStruct &tgApiStruct){
    if (!nlohmann::json::accept(Json)){
        return -1;
    }
    nlohmann::json root = nlohmann::json::parse(Json);

    //find who send command
    if (root.contains("/message/from/id"_json_pointer)){
        tgApiStruct.UserID = root.at("/message/from/id"_json_pointer).get<unsigned long>();
    } else{
        return -1;
    }

    //get command type
    if (root.contains("/message/text"_json_pointer)){
        string command = root.at("/message/text"_json_pointer).get<string>();
        if (command == "/get_pic")
            tgApiStruct.command = COMMAND::GET_PIC;
        if (command == "/get_s_pic")
            tgApiStruct.command = COMMAND::GET_S_PIC;
    } else{
        return -1;
    }
    return 0;
}
void fn_Output(int msgType,const string &msg,unsigned long error){
    switch (msgType) {
        case -1:{
            printf("[Error]%s ,%lu\n",msg.c_str(),error);
            break;
        }
        case 0:{
            printf("[Warn]%s\n",msg.c_str());
            break;
        }
        case 1:{
            printf("[Info]%s\n",msg.c_str());
            break;
        }
        default:
            break;
    }
}