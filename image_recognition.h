#ifndef IMAGE_RECOGNITION_H
#define IMAGE_RECOGNITION_H


#include <QWidget>
#include<QCamera>
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include<QSslSocket>
#include<QSslConfiguration>
#include<QImage>
#include<QMediaCaptureSession>
#include<QVideoWidget>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QJsonParseError>
#include<QJsonDocument>
#include<qjsonobject.h>
#include<QTimer>
#include<QImageCapture>
#include<QBuffer>
#include<QJsonArray>
#include<QFileDialog>
#include<QThread>
#include<qpainter.h>
QT_BEGIN_NAMESPACE
namespace Ui {
class Image_Recognition;
}
QT_END_NAMESPACE

class Image_Recognition : public QWidget
{
    Q_OBJECT

signals:
    void beginWork(QImage img,QThread* workThread);
public slots:
    void taken_capture();
    void showCamera(int id ,const QImage & image);
    void tokenReply(QNetworkReply* reply);
    void beginFaceDetect(QByteArray,QThread*);
    void imgReply(QNetworkReply* reply);
    void chooseImageFromLocal();
    void prePostData();
    void pickCamera(int idx);
public:
    Image_Recognition(QWidget *parent = nullptr);
    ~Image_Recognition();

private:
    Ui::Image_Recognition *ui;
    QMediaCaptureSession* capturesession;
    QCamera* camera;
    QNetworkAccessManager* tokenManager;
    QNetworkAccessManager* imgManager;
    QSslConfiguration sslConfig;
    QString access_token;
    QTimer* refreshTimer,* netTimer;
    QImageCapture* image_capture;
    QImage img;
    QThread * workThread;
    double top,left,width,height;

    double face_age;
    QString gender;
    int mask;
    double beauty;

    int latestTime;

    QList<QCameraDevice> cameraList;
};
#endif // IMAGE_RECOGNITION_H
