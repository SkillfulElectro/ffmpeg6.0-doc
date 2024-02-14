#ifndef ABSTRACTPLAYER_H
#define ABSTRACTPLAYER_H

#include <QObject>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <QThread>
#include <QElapsedTimer>

class AbstractPlayer : public QObject
{
    Q_OBJECT
private:
    // deprecated algorithm
    // AVCodecContext** Codec_Contexts;

    QString FilePath;
    AVFormatContext* FormatCTX;
    // timer for syncing streams , will be provided by user
    QElapsedTimer* timer;
    AVCodecContext* CodecCTX;
    // all the decoding proccess happens here
    QThread* stream_handler;
public:
    // deprecated algorithm
    // AVFrame** Frames_interfaces;
    // AVPacket** Packets_interfaces;

    explicit AbstractPlayer(QString path = "",QObject *parent = nullptr);
    ~AbstractPlayer();

    /*
    * after decoding frames of the stream , new frame will be replaced and
    * user will be notified by the signal which contains index of stream ,
    * new frame will get the place of old one based on the pts of it
    *
    * packet interface is provided for QMediaPlayer because it needs encoded
    * files , and it will be replaced based on pts
    */
    AVFrame* Frame_inter;
    AVPacket* Packet_inter;
    /*
    * number of streams and types of them are provided for checking which
    * stream user would want to decode
    */
    int nb_streams;
    AVMediaType* stream_types;
public slots:
    // deprecated algorithm
    // bool ready_Start();
    // bool ready_start();
    // void start(){
    //     timer.start();
    // }
    // void stream_Handlers(int,AVStream*&,AVFrame*&,AVPacket*&);
    //  void stream_Handlers(int,AVCodecContext*&,AVFrame*&,AVPacket*&);

    /*
    * How to use?
    * first intialize the file path by set function , after that you
    * have to set up the timer , and after setting up the timer , you
    * have to choose the index of file you want to decode based stream_types
    * variable which is provided and after that , you just use
    * QElapsedTimer->start() ! and you get your frames at their time
    *
    * Why QElapsedTimer?
    * QElapsedTimer is used because it was the only Timer type in Qt
    * which could be used to represent time in microseconds , if you
    *  are not satisfied with it in the function implementation
    *  ill tell you which parts you have to change
    *
    * Why user has to set up timer manually?
    * for each stream type we need one AbstractPlayer to sync all of them
    * together , they need to have the same time
    */
    bool ready_start_by_index(int);
    bool set_FilePath(QString path = "");
    void setUp_Timer(QElapsedTimer*);

    // this function is only for debug purposes
    void Debug_fun();
private slots:
    bool init_FormatCTX();
    // function which will run in thread
    void stream_Handlers(int);
    // function which will handle fails of Codec and etc
    void Handle_OpenCodec_Fail(int);

    // future :
    //void Handle_Thread_terminate();
signals:
    void new_Frame(int stream_index);
    void new_Packet(int stream_index);
    void stream_ended(int stream_index);

    // signals to notify problems and errors , connect them
    // to your slots to get updated
    void err_OpenFile_Fail();
    void err_File_Info_Fail();
    void err_VoidPath();
    void err_SetUpTimer();
    void err_No_CodecFound(int stream_index);
    void err_ParametersToCTX_fail(int stream_index);
    void err_OpenCodec_fail(int stream_index);
    void err_SendPacket_fail(int stream_index);
    void err_DecodingPeriodErr(int stream_index);
};

#endif // ABSTRACTPLAYER_H
