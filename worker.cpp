#include "worker.h"


Worker::Worker(QObject *parent):QObject(parent) {

}

void Worker::doWork(QImage  img,QThread* workThread)
{
    //转成base64编码
    QByteArray ba; //使用QBuffer将QBytearrary变得可以像文件一样使用文件接口操作
    QBuffer buff(&ba);
    img.save(&buff,"png"); //直接将
    QString b64str = ba.toBase64();
    //qDebug()<<b64str;


    //请求体Body参数设置
    QJsonObject postJson;
    QJsonDocument doc;

    postJson.insert("image",b64str);
    postJson.insert("image_type","BASE64");
    postJson.insert("face_field","age,expression,face_shape,gender,glasses,emotion,face_type,mask,spoofing,beauty");
    doc.setObject(postJson);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    emit resultReady(postData,workThread);
}
