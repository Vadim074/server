#include <uwebsockets/App.h>
#include <string_view>
#pragma comment (lib, "zlib.lib")
#pragma comment (lib, "uv.lib")
#pragma comment (lib, "uSockets.lib")

using namespace std;

const string SET_NAME = "SET_NAME::";//пользователь сообщает свое имя
const string DIRECT = "DIRECT::";//пользователь может кому-то написать

bool isSetNameCommand(string_view message) {
    return message.find(SET_NAME) == 0;
    //true если это команда установки имени
}
string parseName(string_view message) {
    return string(message.substr(SET_NAME.length()));
}
string parseRecieverId(string_view message) {
    string_view rest = message.substr(DIRECT.length());
    int pos = rest.find("::");
    string_view id = rest.substr(0, pos);
    return string(id);
}
string parseDirectMessage(string_view message) {
    string_view rest = message.substr(DIRECT.length());
    int pos = message.find("::");
    string_view text = rest.substr(pos+2);
    return string(text);
}
bool isDirectCommand(string_view message) {
    return message.find("DIRECT::") == 0;
}
int main() {
    struct PerSocketData {
        int user_id;//номер пользователя
        string name;//имя пользователя
        //структура данных, которая будет хранить 
        //всю информацию об одном пользователе чата
    };

    int last_user_id = 10;
    uWS::App() // создание сервера
           .ws<PerSocketData>("/*", 
           {
            .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
            .maxPayloadLength = 100 * 1024 * 1024,
            .idleTimeout = 600,
            .maxBackpressure = 100 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = false,
            .sendPingsAutomatically = true,
            .upgrade = nullptr,
            .open = [&last_user_id](auto* connection) {
                   cout << "New connection created\n";
                   PerSocketData* userData = (PerSocketData*)connection->getUserData();
                   userData->user_id = last_user_id++;
                   userData->name = "UNNAMED";
                   connection->subscribe("broadcast");
                   connection->subscribe("user#" + to_string(userData->user_id));
            },
            .message = [](auto* connection, string_view message, uWS::OpCode opCode) {
                cout << "New message recieved\n" << message << "\n";
                PerSocketData* userData = (PerSocketData*)connection->getUserData();
                if (isSetNameCommand(message)) 
                {
                    cout << "User set their name\n";
                    userData->name = parseName(message);
                }
                if (isDirectCommand(message)) 
                {
                    string id = parseRecieverId(message);
                    string text = parseDirectMessage(message);
                    cout << "User sent direct message\n";
                    connection->publish("user#" + id, text);
                }
            },
            .close = [](auto*, int, std::string_view) {
                cout << "Connection closed\n";
            }
            }).listen(8080, [](auto* listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 8080 << std::endl;
                }
                }).run();
}