# **BÁO CÁO**

[![Version](https://img.shields.io/badge/Version-1.2.0-<COLOR>.svg)](https://shields.io/)
![Size](https://img.shields.io/github/repo-size/LyQuocBinh1999/Socket_WebServer)

Báo cáo đồ án Socket, bộ môn Mạng máy tính  

<p align="center">
<img width="300" height="300" src="https://i.ibb.co/hMWfQYg/hcmus.png">
</p>

## Thành viên

Nguyễn Gia Thụy - 1712809  
Lý Quốc Bình - 1712292

## Nội dung

+ Mã nguồn
+ Demo chương trình

# Mã nguồn

Hàm main của chương trình:

``` c
void main() {
	try {
		Server server("3000");
		server.run();
	}
	catch (std::runtime_error& e) {
		std::cout << "Run-time error: " << e.what();
	}
}
```
Khai báo của lớp Server:
``` c
class Server
{
public:
    Server(const std::string& port = DEFAULT_PORT); //Constructor

    void run(); //Operate server
   
    ~Server(); //Destructor

private:
    /*---------- Socket-based methods ----------*/

    void initializeWinSock();

    void createSocket(addrinfo*& result, SOCKET& listenSocket, const char* port); 

    void bindSocket(addrinfo* result, const SOCKET& listenSocket);

    void listenOnSocket(const SOCKET& listenSocket);
    
    SOCKET acceptConnection();

    void handleRequests(SOCKET& clientSocket, const std::string* pages);

    /*---------- Utility methods ----------*/

    void handleGET(const std::string& request, const std::string* pages, std::string& response, bool authorized);

    void handlePOST(const std::string& request, const std::string* pages, std::string& response, bool& authorized);

    std::string getResponse(std::string& content, int statusCode, const std::string& message);

    bool authentify(const std::string& username, const std::string& password);

    void cleanUp();

private:
    char _port[5];
    std::string _pages[4];
    addrinfo* _result;
    SOCKET _listenSocket;
};
```

Lớp FileReader - Hỗ trợ đọc files:
``` c
class FileReader {
public:
	static void readContent(const char* path, std::string& content) {
		std::ifstream reader(path);
		if (reader.is_open()) {
			std::string buffer((std::istreambuf_iterator<char>(reader)),
				(std::istreambuf_iterator<char>()));
			reader.close();
			content = buffer;
		}
		else {
			std::cerr << "Cannot read file\n";
		}
	}
};
```

__Chi tiết xem trong [mã nguồn](source/Server.h)__

# Demo chương trình

Sau khi chạy chương trình, ta mở trình duyệt và gõ vào `localhost:[port]` (port tự chọn trong hàm main), trình duyệt sẽ mở file __index.html__ (đường dẫn: _localhost:\[port\]/index.html_) như sau:

![index](https://i.ibb.co/vYhnWrV/index.png)

Nhập `username` và `password` được quy định trong file __userinfo.dat__. Nếu nhập đúng trình duyệt sẽ mở file __info.html__ (_đường dẫn: localhost:\[port\]/info.html_) như hình:

![info](https://i.ibb.co/RTMr8Cd/info.png)

Nếu nhập sai, trình duyệt báo lỗi trên trang __index.html__ như hình:

![index_error](https://i.ibb.co/0YtgWB0/index2.png)

Nếu người dùng gõ vào địa chỉ không tồn tại (ví dụ _localhost:[port]/helloabc_) thì trình duyệt sẽ mở file __error.html__ (_đường dẫn: localhost:\[port\]/error.html_) như hình:

![error](https://i.ibb.co/PZQzwRH/error.png)

# Lưu ý

- Trình duyệt:  
  - Không hỗ trợ các trình duyệt có lõi Chromium
  - Chạy ổn định nhất trên trình duyệt Microsoft Edge Legacy
- Chức năng:  
  - Chưa có chức năng cấp quyền cho nhiều người dùng khác nhau (hiện tại chỉ 1 người dùng đã được đăng kí)
