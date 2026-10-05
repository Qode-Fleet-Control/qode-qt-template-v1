// A minimal Qt HTTP server (Qt 6 QHttpServer), shaped like Qt's own
// "Simple HTTP Server" example: QHttpServer routes, bound to a QTcpServer.
//
// Listens on 0.0.0.0:$PORT (read at runtime, default 8080) and serves at the
// root path:
//   GET /        -> a plain-text greeting
//   GET /health  -> {"status":"ok"}, the fleet's health check
//   GET /api/hello/<name> -> {"message":"Hello, <name>!"}

#include <QCoreApplication>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QTcpServer>
#include <QtGlobal>

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

    auto *tcpServer = new QTcpServer(&app);
    if (!tcpServer->listen(QHostAddress::Any, port) || !server.bind(tcpServer)) {
        qCritical("Server failed to listen on port %d", int(port));
        return 1;
    }
    qInfo("Listening on 0.0.0.0:%d", int(tcpServer->serverPort()));

    return app.exec();
}
