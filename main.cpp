#include <string>
#include <iostream>
#include <fstream>

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::ThreadPool;
using Poco::Timestamp;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;

/**
 * @brief Обработчик запросов на загрузку веб-страниц
 *
 */
class WebPageHandler : public HTTPRequestHandler
{
public:
    WebPageHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest(HTTPServerRequest & request,
                       HTTPServerResponse & response)
    {
        std::cout << "Handle web page" << std::endl;
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        std::ostream &ostr = response.send();
        std::ifstream file;

        auto pos = request.getURI().find('?');
        std::string uri = request.getURI();
        if (pos != std::string::npos)
            uri = uri.substr(0, pos);
        std::string name = "content" + uri;
        file.open(name, std::ifstream::binary);

        if (file.is_open())
            while (file.good())
            {
                int sign = file.get();
                if (sign > 0)
                    ostr << (char)sign;
            }

        file.close();
    }

private:
    std::string _format;
};

/**
 * @brief Обработчик запросов на возвращение данных по session_id
 */
#include "Poco/JSON/Object.h"
#include "Poco/Net/HTMLForm.h"
class RequestHandler : public HTTPRequestHandler
{
public:
    RequestHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest(HTTPServerRequest &request,
                       HTTPServerResponse &response)
    {
        std::cout << "Handle request" << std::endl;

        Poco::Net::HTMLForm form(request);      
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        std::ostream &ostr = response.send();
        if (form.has("session_id"))
        {
            std::string session_str = form.get("session_id");
            try
            {
                Poco::JSON::Array arr;
                Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                root->set("id", session_str);
                root->set("some_text", "Some Text");
                arr.add(root);
                root = new Poco::JSON::Object();
                root->set("id", session_str);
                root->set("some_text", "Another Text");
                arr.add(root);
                Poco::JSON::Stringifier::stringify(arr, ostr);
            }
            catch (...)
            {
                std::cout << "exception" << std::endl;
            }
        }
        response.setStatus(Poco::Net::HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
    }

private:
    std::string _format;
};

static bool startsWith(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

class HTTPRequestFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestFactory(const std::string &format) : _format(format)
    {
    }

    HTTPRequestHandler *createRequestHandler([[maybe_unused]] const HTTPServerRequest &request)
    {
        std::string math = "/request";
        if (startsWith(request.getURI(), math))
            return new RequestHandler(_format);
        else
            return new WebPageHandler(_format);
    }

private:
    std::string _format;
};

class HTTPWebServer : public Poco::Util::ServerApplication
{
public:
    HTTPWebServer() : _helpRequested(false)
    {
    }

    ~HTTPWebServer()
    {
    }

protected:
    void initialize(Application &self)
    {
        loadConfiguration();
        ServerApplication::initialize(self);
    }

    void uninitialize()
    {
        ServerApplication::uninitialize();
    }

    void defineOptions(OptionSet &options)
    {
        ServerApplication::defineOptions(options);
    }

    int main([[maybe_unused]] const std::vector<std::string> &args)
    {
        if (!_helpRequested)
        {
            unsigned short port = (unsigned short)
                                      config()
                                          .getInt("HTTPWebServer.port", 8080);
            std::string format(
                config().getString("HTTPWebServer.format",
                                   DateTimeFormat::SORTABLE_FORMAT));

            ServerSocket svs(Poco::Net::SocketAddress("0.0.0.0", port));
            HTTPServer srv(new HTTPRequestFactory(format),
                           svs, new HTTPServerParams);

            std::cout << "Started server on port: 8080" << std::endl;
            srv.start();

            waitForTerminationRequest();
            srv.stop();
        }
        return Application::EXIT_OK;
    }

private:
    bool _helpRequested;
};

int main(int argc, char *argv[])
{
    HTTPWebServer app;

    std::cout << "Starting server at port 8080 ..." << std::endl;
    return app.run(argc, argv);
    
}