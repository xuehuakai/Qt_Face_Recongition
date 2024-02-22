#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include<QBuffer>
#include<QImage>
#include<qjsondocument.h>
#include<qjsonobject.h>
class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject* parent =  nullptr);

public slots:
    void doWork(QImage, QThread *workThread);
signals:
    void resultReady(QByteArray,QThread* workThread);
};

#endif // WORKER_H
