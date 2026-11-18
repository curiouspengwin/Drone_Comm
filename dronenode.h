#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QTimer>

class DroneNode : public QObject
{
    Q_OBJECT
public:
    DroneNode(uint8_t id, uint8_t totalDrones, QObject *parent = nullptr);

public slots:
    void start(); //Initializes socket and timer on the wokrer thread

private slots:
    void sendTelemetryToGCS(); // periodic push to port 14550 (GCS)
    void broadcastStateToPeers(); // Pub-Sub model
    void readDatagrams(); // Follower only: listen for leader coordinates

private:
    uint8_t m_id;
    uint8_t m_totalDrones;
    // bool m_isLeader;
    float m_posX = 0.0f;
    float m_posY = 0.0f;
    float m_posZ = 10.0f;
    int gcsPort = 50000;
    bool move = true;

    // If these are not pointers, they are immediately created when droneNode is called (on the main thread).
    // So when we try to start the timer in the new thread, the program will not let us.
    QUdpSocket* m_socket = nullptr;
    QTimer* m_timer = nullptr;
};

