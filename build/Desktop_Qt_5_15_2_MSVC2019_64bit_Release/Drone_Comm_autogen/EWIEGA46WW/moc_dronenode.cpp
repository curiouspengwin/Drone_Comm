/****************************************************************************
** Meta object code from reading C++ file 'dronenode.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../dronenode.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dronenode.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DroneNode_t {
    QByteArrayData data[12];
    char stringdata0[112];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DroneNode_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DroneNode_t qt_meta_stringdata_DroneNode = {
    {
QT_MOC_LITERAL(0, 0, 9), // "DroneNode"
QT_MOC_LITERAL(1, 10, 5), // "start"
QT_MOC_LITERAL(2, 16, 0), // ""
QT_MOC_LITERAL(3, 17, 18), // "onPeerPoseReceived"
QT_MOC_LITERAL(4, 36, 7), // "uint8_t"
QT_MOC_LITERAL(5, 44, 6), // "peerId"
QT_MOC_LITERAL(6, 51, 2), // "px"
QT_MOC_LITERAL(7, 54, 2), // "py"
QT_MOC_LITERAL(8, 57, 2), // "pz"
QT_MOC_LITERAL(9, 60, 18), // "sendTelemetryToGCS"
QT_MOC_LITERAL(10, 79, 18), // "publishPoseToZenoh"
QT_MOC_LITERAL(11, 98, 13) // "readDatagrams"

    },
    "DroneNode\0start\0\0onPeerPoseReceived\0"
    "uint8_t\0peerId\0px\0py\0pz\0sendTelemetryToGCS\0"
    "publishPoseToZenoh\0readDatagrams"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DroneNode[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x0a /* Public */,
       3,    4,   40,    2, 0x0a /* Public */,
       9,    0,   49,    2, 0x08 /* Private */,
      10,    0,   50,    2, 0x08 /* Private */,
      11,    0,   51,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4, QMetaType::Float, QMetaType::Float, QMetaType::Float,    5,    6,    7,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DroneNode::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DroneNode *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->start(); break;
        case 1: _t->onPeerPoseReceived((*reinterpret_cast< uint8_t(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4]))); break;
        case 2: _t->sendTelemetryToGCS(); break;
        case 3: _t->publishPoseToZenoh(); break;
        case 4: _t->readDatagrams(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DroneNode::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DroneNode.data,
    qt_meta_data_DroneNode,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DroneNode::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DroneNode::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DroneNode.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DroneNode::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
