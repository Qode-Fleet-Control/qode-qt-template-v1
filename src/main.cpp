// A minimal Qt HTTP server (Qt 6 QHttpServer), shaped like Qt's own
// "Simple HTTP Server" example: QHttpServer routes, bound to a QTcpServer.
//
// Listens on 0.0.0.0:$PORT (read at runtime, default 8080) and serves at the
// root path:
//   GET /        -> a plain-text greeting
//   GET /health  -> {"status":"ok"}, the fleet's health check
//   GET /api/hello/<name> -> {"message":"Hello, <name>!"}

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtGlobal>

#include <cmath>
#include <cstdio>

// KEEP THIS: one log line per request (status + duration) is what the workspace Monitoring tab charts.
// Each finished request prints one JSON line to stdout:
//   {"durationMs":0.42,"method":"GET","path":"/health","status":200,"ua":"curl/8.5.0"}
// The server runs on one thread and QHttpServer handles a request synchronously inside the
// socket's readyRead, so the clock is started by a readyRead slot connected before
// QHttpServer's own (RequestTimingServer below) and read when the response is ready.
static QElapsedTimer requestClock;

class RequestTimingServer : public QTcpServer
{
public:
    using QTcpServer::QTcpServer;

protected:
    void incomingConnection(qintptr descriptor) override
    {
        auto *socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(descriptor)) {
            delete socket;
            return;
        }
        connect(socket, &QTcpSocket::readyRead, socket, [] { requestClock.start(); });
        addPendingConnection(socket);
    }
};

static const char *methodName(QHttpServerRequest::Method m)
{
    using M = QHttpServerRequest::Method;
    switch (m) {
    case M::Get: return "GET";
    case M::Head: return "HEAD";
    case M::Post: return "POST";
    case M::Put: return "PUT";
    case M::Patch: return "PATCH";
    case M::Delete: return "DELETE";
    case M::Options: return "OPTIONS";
    case M::Connect: return "CONNECT";
    case M::Trace: return "TRACE";
    default: return "UNKNOWN";
    }
}

static void logRequest(const QHttpServerRequest &req, int status)
{
    const double ms = requestClock.isValid() ? requestClock.nsecsElapsed() / 1e6 : 0.0;
    const QJsonObject line{
        {"status", status},
        {"durationMs", std::round(ms * 1000.0) / 1000.0},
        {"method", methodName(req.method())},
        {"path", req.url().path()},  // path only: never the query string
        {"ua", QString::fromUtf8(req.value("User-Agent"))},
    };
    const QByteArray out = QJsonDocument(line).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(out.constData(), 1, size_t(out.size()), stdout);
    std::fflush(stdout);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    bool ok = false;
    const int envPort = qEnvironmentVariableIntValue("PORT", &ok);
    const quint16 port = (ok && envPort > 0 && envPort < 65536) ? quint16(envPort) : quint16(8080);

    QHttpServer server;
    server.route("/", []() {
        return QStringLiteral("Hello from the Qt template!\n");
    });
    server.route("/health", []() {
        return QHttpServerResponse(QJsonObject{{"status", "ok"}});
    });
    server.route("/api/hello/<arg>", [](const QString &name) {
        return QHttpServerResponse(QJsonObject{{"message", QStringLiteral("Hello, %1!").arg(name)}});
    });

    // KEEP THIS: one log line per request (status + duration) is what the workspace Monitoring tab charts.
    server.addAfterRequestHandler(&server, [](const QHttpServerRequest &req, QHttpServerResponse &resp) {
        logRequest(req, int(resp.statusCode()));
    });
    server.setMissingHandler(&server, [](const QHttpServerRequest &req, QHttpServerResponder &responder) {
        responder.write(QHttpServerResponder::StatusCode::NotFound);
        logRequest(req, 404);
    });

    auto *tcpServer = new RequestTimingServer(&app);
    if (!tcpServer->listen(QHostAddress::Any, port) || !server.bind(tcpServer)) {
        qCritical("Server failed to listen on port %d", int(port));
        return 1;
    }
    qInfo("Listening on 0.0.0.0:%d", int(tcpServer->serverPort()));

    return app.exec();
}
