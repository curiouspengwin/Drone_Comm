#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include "GcsManager.h"
#include "dronenode.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    qmlRegisterType<GcsManager>("com.app.gcsmanager", 1, 0, "GcsManager");

    // Loop to generate drones

    const uint8_t NUM_DRONES = 4;
    QList<QThread*> droneThreads;

    for (uint8_t id = 1; id <= NUM_DRONES; ++id){
        QThread *thread = new QThread(&app);
        // A child cannot exist on a different thread from a parent.
        DroneNode *drone = new DroneNode(id, true);
        // parent = nullptr is required for moveToThread.
        drone->moveToThread(thread);

        // Starting drone node when thread starts
        QObject::connect(thread, &QThread::started, drone, &DroneNode::start);
        // Quitting thread when core application quits
        QObject::connect(&app, &QCoreApplication::aboutToQuit, thread, &QThread::quit);
        // Deleteding DroneNode when thread is finished
        // Because this drone has no parent, we need to delete it explicitly.
        QObject::connect(thread, &QThread::finished, drone, &QObject::deleteLater);

        droneThreads.append(thread);
        thread->start();
    }

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    // UI starts execution here.
    // Stays in its own loop until user closes window
    // Quitting this triggers the thread quitting and deletion
    int ret = QGuiApplication::exec();

    // Wait for worker threads to finish cleanly before exiting main()
    for(QThread* thread : droneThreads){
        thread->wait();
    }

    return ret;
}

/*
Features to add
*1. Multithreading
*2.1 GCS sending commands to the drone
*2.2 D2D sending data to each other
*3. Multiple Drones
4. Use mavlink protocol (mavlink.h)
5. Zenoh

Lets say the d1 drone commands the follower to halt the,
how does the position of follower change with time. (Basically physics implementation)
*/