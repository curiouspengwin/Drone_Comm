#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QVector3D>
#include <zenoh.h>

struct _PragmaPackWorkaround; // Prevents clangd from grouping the pragma into the header preamble

#pragma pack(push, 1)
struct PeerPose { // 16 Byte
    uint8_t senderId;
    float x;
    float y;
    float z;
};
#pragma pack(pop)


class DroneNode : public QObject
{
    Q_OBJECT
public:
    // DroneNode(uint8_t id, uint8_t totalDrones, QObject *parent = nullptr);
    DroneNode(uint8_t id, QObject *parent = nullptr);
    ~DroneNode();

public slots:
    void start(); //Initializes socket and timer on the worker thread
    void onPeerPoseReceived(uint8_t peerId, float px, float py, float pz);

private slots:
    void updatePhysicsAndSwarm(); // Update physics
    void sendTelemetryToGCS(); // periodic push to port 14550 (GCS)
    // void broadcastStateToPeers(); // Older broadcast model
    void publishPoseToZenoh(); // Pub-Sub model
    void readGcsDatagrams(); // Read UDP datagrams from MAVLink

private:
    uint8_t m_id;
    // uint8_t m_totalDrones;
    // bool m_isLeader;

    QVector3D m_pos;
    QVector3D m_vel;
    QMap<uint8_t, QVector3D> m_peers; // Stores latest 3D position of every neighbor

    // float m_posX = 0.0f;
    // float m_posY = 0.0f;
    // float m_posZ = 10.0f;

    int gcsPort = 50000;
    bool move = true;
    int m_tickCount = 0;

    // If these are not pointers, they are immediately created when droneNode is called (on the main thread).
    // So when we try to start the timer in the new thread, the program will not let us.
    QUdpSocket* m_socket = nullptr;
    QTimer* m_timer = nullptr;

    // Zenoh D2D Handles
    z_owned_session_t m_zSession;
    z_owned_publisher_t m_zPub;
    z_owned_subscriber_t m_zSub;
    bool m_zenohReady = false;

};

