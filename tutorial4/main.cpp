/*
i really didnt provide a clean code here , all of this file was just for
testing if the class works fine or not
*/

#include <QGuiApplication>
#include <QQmlApplicationEngine>

//#include <QMediaPlayer>
//#include <QAudioOutput>

#include <QImage>
#include <QQuickImageProvider>
#include <QQuickItem>

#include <QElapsedTimer>
#include <QThread>
#include <QTimer>

#include "abstractplayer.h"

// class for giving QImages to qml side for displaying
class MyImageProvider : public QQuickImageProvider
{
private:
    AbstractPlayer player;
    QQuickItem* item;
public:
    MyImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    // this function could be used to make code cleaner for this class
    void set_requirements(QString path , QString Objectname , QQmlApplicationEngine& engine){
        player.set_FilePath(path);
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(id);
        Q_UNUSED(size);
        Q_UNUSED(requestedSize);
        // Return your QImage here
        return myImage;
    }

    // Function to update the image
    void updateImage(const QImage &image)
    {
        myImage = image;
    }
public slots:

private:
    QImage myImage;
};
AbstractPlayer smth;
MyImageProvider video;
QQuickItem* g_item;


/*
function to convert Frame to QImage
*/
QImage getQImageFromFrame(const AVFrame* pFrame)
{
    // first convert the input AVFrame to the desired format

    SwsContext* img_convert_ctx = sws_getContext(
        pFrame->width,
        pFrame->height,
        (AVPixelFormat)pFrame->format,
        pFrame->width,
        pFrame->height,
        AV_PIX_FMT_RGB24,
        SWS_BICUBIC, NULL, NULL, NULL);
    if(!img_convert_ctx){
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    // prepare line sizes structure as sws_scale expects
    int rgb_linesizes[8] = {0};
    rgb_linesizes[0] = 3*pFrame->width;

    // prepare char buffer in array, as sws_scale expects
    unsigned char* rgbData[8];
    int imgBytesSyze = 3*pFrame->height*pFrame->width;
    // as explained above, we need to alloc extra 64 bytes
    rgbData[0] = (unsigned char *)malloc(imgBytesSyze+64);
    if(!rgbData[0]){
        qDebug() << "Error allocating buffer for frame conversion";
        free(rgbData[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }
    if(sws_scale(img_convert_ctx,
                  pFrame->data,
                  pFrame->linesize, 0,
                  pFrame->height,
                  rgbData,
                  rgb_linesizes)
        != pFrame->height){
        qDebug() << "Error changing frame color range";
        free(rgbData[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }

    // then create QImage and copy converted frame data into it

    QImage image(pFrame->width,
                 pFrame->height,
                 QImage::Format_RGB888);

    for(int y=0; y<pFrame->height; y++){
        memcpy(image.scanLine(y), rgbData[0]+y*3*pFrame->width, 3*pFrame->width);
    }

    free(rgbData[0]);
    sws_freeContext(img_convert_ctx);
    return image;
}

// class for updating images of QQuickImageProvider
class MyClass : public QObject
{
    Q_OBJECT
public:
signals:
public slots:
    void testing(int index){
        if (index == 0){
            qDebug() << smth.Frame_inter->pts;

            video.updateImage(getQImageFromFrame(smth.Frame_inter));
            g_item->setProperty("source" , "image://myprovider/myimage" + QString::number(smth.Frame_inter->pts));
        }
    }
};



int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/MediaPlayer_Qt/Main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.addImageProvider(QLatin1String("myprovider"), &video);
    engine.load(url);

    // QImage object to display video frames on
    g_item = engine.rootObjects().first()->findChild<QQuickItem*>("show");



    // change the file path to what you want
    smth.set_FilePath("F:/Media/movies/Beauty.And.The.Beast.2017.720p.BluRay.HardSub.DigiMoviez.mp4");
    // make sure to allocate QElapsedTimer dynamically
    QElapsedTimer* timeri = new QElapsedTimer;
    // and deallocate it at runtime too so dynamically
    // i didnt do it because as i said this file was just for fast testing
    smth.setUp_Timer(timeri);
    smth.ready_start_by_index(0);
    MyClass smth1;
    QObject::connect(&smth , &AbstractPlayer::new_Frame , &smth1 , &MyClass::testing);



    auto valueKeeper {g_item->property("source")};
    QTimer timer;
    QObject::connect(&timer , &QTimer::timeout , [&](){
        if (g_item->property("source") != valueKeeper){
            timeri->start();
            timer.stop();
        }
    });
    timer.start(1000);



    return app.exec();
}

#include "main.moc"

// garbage dump :
//QMediaPlayer* player = new QMediaPlayer;
//QAudioOutput* audioOutput = new QAudioOutput;

// Set the audio output for the player
//player->setAudioOutput(audioOutput);

// Set the source to the MP3 file
/*
    player->setSource
        (QUrl::fromLocalFile("F:/Media/songs/How To Make an HTTP Server in Go.mp3"));

    // Set the volume
    audioOutput->setVolume(50);

    // Start playing the MP3 file
    player->play();
    qDebug() << timer.clockType();
    qDebug() << timer.elapsed();
    qDebug() << timer.nsecsElapsed()/1000;
    */

/*
    while(timer.nsecsElapsed()/1000 <= 4764170917){
        //qDebug() << timer.nsecsElapsed()/1000;
        QThread::msleep(5000);
    }
    */

//qDebug() << timer.elapsed();
