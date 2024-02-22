#include "image_recognition.h"
#include "ui_image_recognition.h"
#include "worker.h"
#include<QMediaDevices>
#include<QImageCapture>
#include<qurl.h>
#include<qurlquery.h>
#include<QCameraDevice>
Image_Recognition::Image_Recognition(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Image_Recognition)
{


    ui->setupUi(this);

    cameraList =  QMediaDevices::videoInputs();

    foreach (QCameraDevice device, cameraList) {
        ui->comboBox->addItem(device.description());
    }


 resize(1200,700);
    //ui->Label_image_capture->resize(300,400);
    QVBoxLayout* vboxl = new QVBoxLayout();

    vboxl->addWidget(ui->Label_image_capture);
    QHBoxLayout* btn_hbox = new QHBoxLayout();
    btn_hbox->addWidget(ui->btn_capture);
    btn_hbox->addWidget(ui->btn_local);
    vboxl->addLayout(btn_hbox);
    //vboxl->addWidget(ui->btn_capture);
    QVBoxLayout* vboxr = new QVBoxLayout();
    vboxr->addWidget(ui->comboBox);
    vboxr->addWidget(ui->widget);
    vboxr->addWidget(ui->textBrowser);
    QHBoxLayout* hbox = new QHBoxLayout(this);
    hbox->addLayout(vboxl);
    hbox->addLayout(vboxr);
    setLayout(hbox);


    capturesession=new QMediaCaptureSession(this);
    image_capture = new QImageCapture(this);
    image_capture->setFileFormat(QImageCapture::JPEG);
    //image_capture->setQuality(QImageCapture::VeryHighQuality);


    //image_capture->captureToFile("main.jpg");
    camera = new QCamera(QMediaDevices::defaultVideoInput());
    camera->start();
    capturesession->setVideoOutput(ui->widget);
    capturesession->setCamera(camera);

    tokenManager = new QNetworkAccessManager(this);
    imgManager = new QNetworkAccessManager(this);
    connect(tokenManager,&QNetworkAccessManager::finished,this,&Image_Recognition::tokenReply);
    connect(imgManager,&QNetworkAccessManager::finished,this,&Image_Recognition::imgReply);


    //combox
    connect(ui->comboBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Image_Recognition::pickCamera);

    //利用定时器不断刷新拍照界面
    refreshTimer = new QTimer(this);
    capturesession->setImageCapture(image_capture);
    connect(image_capture,&QImageCapture::imageCaptured,this,&Image_Recognition::showCamera);
    connect(refreshTimer,&QTimer::timeout,this,&Image_Recognition::taken_capture);
    connect(ui->btn_capture,&QPushButton::clicked,this,&Image_Recognition::prePostData);
    connect(ui->btn_local,&QPushButton::clicked,this,&Image_Recognition::chooseImageFromLocal);
    refreshTimer->start(10);

    //利用定时器不断进行人脸请求
    netTimer=new QTimer(this) ;
    connect(netTimer,&QTimer::timeout,this,&Image_Recognition::prePostData);

    //拼接url
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");

    QUrlQuery query;
    query.addQueryItem("grant_type","client_credentials");
    query.addQueryItem("client_id","QtGQu5uz1uAUekeEFfwtrcHa");
    query.addQueryItem("client_secret","0qAhvSVqUwakc33c8TAbrh6pBmuKY9hQ");

    url.setQuery(query);
  //  qDebug()<<url;


    //ssl支持
    QSslSocket *socket = new QSslSocket(this);

    if(QSslSocket::supportsSsl()){
        qDebug()<<"支持";
    }else{
        qDebug()<<"不支持";
    }
    qDebug() << QSslSocket::sslLibraryBuildVersionString();


    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyDepth(QSslSocket::QueryPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2);

    //ssl配置完成
    QNetworkRequest req;
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);


    tokenManager->get(req);


}

void Image_Recognition::taken_capture()
{
    image_capture->capture();
}

void Image_Recognition::tokenReply(QNetworkReply *reply)
{
    //错误处理
    if(reply->error() != QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    //正常应答
    const QByteArray reply_data = reply->readAll();

    //json解析
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data,&jsonErr);

    if(jsonErr.error == QJsonParseError::NoError)
    {
        //解析成功
        QJsonObject obj = doc.object();
        if(obj.contains("access_token")){
            QJsonValue val = obj.take("access_token");
            access_token = val.toString();
            ui->textBrowser->setText(access_token);
        }
    }else{
        qDebug()<<"JSON ERR : "<<jsonErr.errorString();
    }

    reply->deleteLater();

   // prePostData();
    netTimer->start(500);

}

void Image_Recognition::beginFaceDetect(QByteArray postData,QThread* overThread)
{

//    image_capture->capture();

    //另一个槽函数
    //关闭子线程
    //workThread->exit();
    overThread->exit();
    overThread->wait();
    if(overThread->isFinished()){
        qDebug()<<"子线程结束了";
    }else{
        qDebug()<<"子线程没结束";
    }
    //组装图像识别请求
    QUrl url("https://aip.baidubce.com/rest/2.0/face/v3/detect");
    QUrlQuery query;
    query.addQueryItem("access_token",access_token);
    url.setQuery(query);

    //组装请求
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("applicationo/json"));
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);

    imgManager->post(req,postData);

}

void Image_Recognition::imgReply(QNetworkReply *reply)
{
    if(reply->error()!= QNetworkReply::NoError){
        qDebug()<<reply->errorString();
        return;
    }
    const QByteArray replyData = reply->readAll();
  //  qDebug()<<replyData;

    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(replyData,&jsonErr);
    if(jsonErr.error == QJsonParseError::NoError){

        QString faceinfo;

        //去除最外层的json
        QJsonObject obj = doc.object();

        if(obj.contains("timestamp")){
            int tmpTime = obj.take("timestamp").toInt();
            if(tmpTime<latestTime) //比当前时间小的话 就不处理了
                return;
            else
                latestTime=tmpTime;
        }

        if(obj.contains("result")){
            QJsonObject resultObj = obj.take("result").toObject();

            //取出人脸列表
            if(resultObj.contains("face_list")){
                QJsonArray face_array = resultObj.take("face_list").toArray();

                 //取出第一张人脸信息
                QJsonObject face_obj = face_array.at(0).toObject();

                //取出人脸定位信息
                if(face_obj.contains("location")){
                    QJsonObject locObj = face_obj.take("location").toObject();
                    if(locObj.contains("left")){
                       left = locObj.take("left").toDouble();
                    }

                    if(locObj.contains("top")){
                        top = locObj.take("top").toDouble();
                    }

                    if(locObj.contains("width")){
                        width = locObj.take("width").toDouble();
                    }

                    if(locObj.contains("height")){
                        height = locObj.take("height").toDouble();
                    }
                }


                //取出年龄
                if(face_obj.contains("age")){
                     face_age = face_obj.take("age").toDouble();
                    faceinfo.append("年龄:").append(QString::number(face_age)).append("\r\n");

                }

                //取出性别
                if(face_obj.contains("gender")){
                    QJsonObject face_gender = face_obj.take("gender").toObject();

                    if(face_gender.contains("type")){
                        gender = face_gender.take("type").toString();
                        faceinfo.append("性别:").append(gender).append("\r\n");
                    }

                }


                //取出情绪
                if(face_obj.contains("emotion")){
                    QJsonObject face_emotion = face_obj.take("emotion").toObject();

                    if(face_emotion.contains("type")){
                        QString emotion = face_emotion.take("type").toString();
                        faceinfo.append("表情:").append(emotion).append("\r\n");
                    }

                }

                //是否带口罩
                if(face_obj.contains("mask")){
                    QJsonObject face_mask = face_obj.take("mask").toObject();

                    if(face_mask.contains("type")){
                         mask = face_mask.take("type").toInt();
                        faceinfo.append("是否戴口罩:").append(mask==0?"不带":"带").append("\r\n");
                    }

                }

                //取出颜值
                if(face_obj.contains("beauty")){
                     beauty = face_obj.take("beauty").toDouble();
                    faceinfo.append("颜值:").append(QString::number(beauty)).append("\r\n");
                }


            }
        }

        ui->textBrowser->setText(faceinfo);
    }   else{
        qDebug()<<"JSON ERR : "<<jsonErr.errorString();
    }

    reply->deleteLater();

  //  prePostData();
}

void Image_Recognition::chooseImageFromLocal()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("open image"),"D://",tr("(*.jpg)\n(*.bmp)\n(*.png)"));
    img = QImage(fileName);
    ui->Label_image_capture->setPixmap(QPixmap::fromImage(img));
    //转成base64编码
    QByteArray ba; //使用QBuffer将QBytearrary变得可以像文件一样使用文件接口操作
    QBuffer buff(&ba);
    img.save(&buff,"png"); //直接将
    QString b64str = ba.toBase64();


    //请求体Body参数设置
    QJsonObject postJson;
    QJsonDocument doc;

    postJson.insert("image",b64str);
    postJson.insert("image_type","BASE64");
    postJson.insert("face_field","age,expression,face_shape,gender,glasses,emotion,face_type,mask,spoofing,beauty");
    doc.setObject(postJson);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    //组装图像识别请求
    QUrl url("https://aip.baidubce.com/rest/2.0/face/v3/detect");
    QUrlQuery query;
    query.addQueryItem("access_token",access_token);
    url.setQuery(query);

    //组装请求
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("applicationo/json"));
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);

    imgManager->post(req,postData);



}

void Image_Recognition::prePostData()
{
    image_capture->capture();
    //创建线程 把工人送进子线程 启动线程 发通知干活
    workThread = new QThread(this);
    Worker* worker = new Worker;
    worker->moveToThread(workThread);

    connect(this,&Image_Recognition::beginWork,worker,&Worker::doWork);
    connect(worker,&Worker::resultReady,this,&Image_Recognition::beginFaceDetect);
    connect(workThread,&QThread::finished,worker,&QObject::deleteLater);

    workThread->start();

    emit beginWork(img,workThread);
}

void Image_Recognition::pickCamera(int idx)
{
    refreshTimer->stop();
    camera->stop();
    camera = new QCamera(cameraList.at(idx));

    //image_capture = new QImageCapture(camera);
    capturesession->setCamera(camera);
   // connect(image_capture,&QImageCapture::imageCaptured,this,&Image_Recognition::showCamera);

    camera->start();
    refreshTimer->start(10);
}

void Image_Recognition::showCamera(int id , const QImage & image){
    Q_UNUSED(id);
    img = image;

    //绘制人脸框
    QPainter p(&img);
    p.setPen(Qt::blue);
    p.drawRect(left,top,width,height);

    QFont font = p.font();
    font.setPixelSize(30);
    p.setFont(font);
    p.setPen(Qt::red);
    p.drawText(left+width+5,top,QString("年龄：").append(QString::number(face_age)));
     p.drawText(left+width+5,top+40,QString("性别：").append(gender));
      p.drawText(left+width+5,top+80,QString("戴口罩：").append(mask==0?"没带口罩":"带了口罩"));
       p.drawText(left+width+5,top+120,QString("颜值：").append(QString::number(beauty)));

    ui->Label_image_capture->setPixmap(QPixmap::fromImage(img));
}

Image_Recognition::~Image_Recognition()
{
    delete ui;
}
