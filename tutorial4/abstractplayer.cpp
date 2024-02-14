#include "abstractplayer.h"
#include <QDebug>

/* intializing variables and connecting some signals */
AbstractPlayer::AbstractPlayer(QString path ,QObject *parent)
    : QObject{parent} , FilePath{path}
{
    if (path != ""){
        init_FormatCTX();
    }else{
        FormatCTX = nullptr;
    }
    timer = nullptr;
    CodecCTX = nullptr;
    stream_handler = nullptr;
    Frame_inter = nullptr;
    Packet_inter = nullptr;
    stream_types = nullptr;
    QObject::connect
        (this , &AbstractPlayer::err_OpenCodec_fail , this , &AbstractPlayer::Handle_OpenCodec_Fail);
    QObject::connect
        (this , &AbstractPlayer::err_SendPacket_fail , this , &AbstractPlayer::Handle_OpenCodec_Fail);
    QObject::connect
        (this , &AbstractPlayer::err_DecodingPeriodErr , this , &AbstractPlayer::Handle_OpenCodec_Fail);

}

bool AbstractPlayer::set_FilePath(QString path){
    FilePath = path;
    if (path == ""){
        emit err_VoidPath();
        return false;
    }
    return init_FormatCTX();
}

bool AbstractPlayer::init_FormatCTX(){
    if (FilePath == ""){
        emit err_VoidPath();
        return false;
    }
    // opening the file to get the AVFormatContext
    if (avformat_open_input
        (&FormatCTX, FilePath.toStdString().c_str() , NULL, NULL) != 0) {
        avformat_close_input(&FormatCTX);
        avformat_free_context(FormatCTX);
        emit err_OpenFile_Fail();
        return false;
    }
    if (avformat_find_stream_info(FormatCTX, NULL) < 0) {
        avformat_close_input(&FormatCTX);
        avformat_free_context(FormatCTX);
        emit err_File_Info_Fail();
        return false;
    }

    nb_streams = FormatCTX->nb_streams;
    stream_types = new AVMediaType[nb_streams];
    for (int i{0};i<nb_streams;++i){
        stream_types[i] = FormatCTX->streams[i]->codecpar->codec_type;
    }

    return true;
}

AbstractPlayer::~AbstractPlayer(){
    // wait for stream threads to end and terminate
    if (stream_handler->isRunning() == true || stream_handler != nullptr){
        stream_handler->wait();
        delete stream_handler;
    }
    if (Frame_inter != nullptr || Frame_inter != NULL){
        av_frame_free(&Frame_inter);
    }
    if (Packet_inter != nullptr || Packet_inter != NULL){
        av_packet_free(&Packet_inter);
    }
    if (CodecCTX != nullptr || CodecCTX != NULL){
        avcodec_free_context(&CodecCTX);
    }
    delete[] stream_types;
    if (FormatCTX != nullptr || FormatCTX != NULL){
        avformat_close_input(&FormatCTX);
        avformat_free_context(FormatCTX);
    }
}

// making everything ready to play the stream after timer starts
bool AbstractPlayer::ready_start_by_index(int index){
    if (stream_handler == nullptr){
        stream_handler = new QThread;
    }else{
        if(stream_handler->isRunning()){
            stream_handler->wait();
        }
        delete stream_handler;
        stream_handler = new QThread;
    }
    if (timer == nullptr){
        if (stream_handler->isRunning() == true){
            stream_handler->wait();
        }
        delete stream_handler;
        emit err_SetUpTimer();
        return false;
    }
    if (CodecCTX != nullptr || CodecCTX != NULL){
        avcodec_free_context(&CodecCTX);
        CodecCTX = nullptr;
    }

    const AVCodec* Codec{
        avcodec_find_decoder(FormatCTX->streams[index]->codecpar->codec_id)
    };

    if (Codec == nullptr) {
        if (stream_handler->isRunning() == true){
            stream_handler->wait();
        }
        delete stream_handler;
        emit err_No_CodecFound(index);
        return false;
    }
    CodecCTX =
        avcodec_alloc_context3(Codec);

    if (avcodec_parameters_to_context
        (CodecCTX, FormatCTX->streams[index]->codecpar) < 0) {
        if (stream_handler->isRunning() == true){
            stream_handler->wait();
        }
        delete stream_handler;
        avcodec_free_context(&CodecCTX);
        emit err_ParametersToCTX_fail(index);
        return false;
    }

    QObject::connect(stream_handler , &QThread::started , [index,this](){
        stream_Handlers
            (index);
    });
    stream_handler->start();

    return true;
}

void AbstractPlayer::Handle_OpenCodec_Fail(int index){
    if (stream_handler->isRunning()){
        stream_handler->wait();
    }
    delete stream_handler;
    stream_handler = nullptr;
}

void AbstractPlayer::stream_Handlers(int index){
    if (avcodec_open2(CodecCTX, CodecCTX->codec, NULL) < 0) {
        avcodec_free_context(&CodecCTX);
        CodecCTX = nullptr;
        emit err_OpenCodec_fail(index);
        return;
    }

    if (Packet_inter != nullptr || Packet_inter != NULL){
        av_packet_free(&Packet_inter);
        Packet_inter = nullptr;
    }
    if (Frame_inter != nullptr || Frame_inter != NULL){
        av_frame_free(&Frame_inter);
        Frame_inter = nullptr;
    }

    Packet_inter = av_packet_alloc();

    AVFrame* Frame_now{av_frame_alloc()};
    Frame_inter = av_frame_alloc();

    while (av_read_frame(FormatCTX, Packet_inter) >= 0) {
        if (Packet_inter->stream_index == index) {

            // sending packet to decoder
            int response = avcodec_send_packet(CodecCTX, Packet_inter);
            if (response < 0) {

                av_frame_free(&Frame_now);
                av_frame_free(&Frame_inter);
                av_packet_free(&Packet_inter);
                avcodec_free_context(&CodecCTX);

                Frame_inter = nullptr;
                Packet_inter = nullptr;
                CodecCTX = nullptr;

                emit err_SendPacket_fail(index);
                return;
            }

            while (response >= 0) {
                emit new_Packet(index);

                response = avcodec_receive_frame(CodecCTX, Frame_now);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    av_frame_free(&Frame_now);
                    av_frame_free(&Frame_inter);

                    av_packet_free(&Packet_inter);

                    avcodec_free_context(&CodecCTX);

                    Frame_inter = nullptr;
                    Packet_inter = nullptr;
                    CodecCTX = nullptr;

                    emit err_DecodingPeriodErr(index);
                    return;
                }


                // here is waiting phase of thread for spacific time
                // you can set up your own timer here , std::chrono and etc
                while (Frame_now->pts > timer->nsecsElapsed()/10000){}
                AVFrame* tmp = Frame_inter;
                Frame_inter = Frame_now;
                emit new_Frame(index);
                Frame_now = tmp;


                av_frame_unref(Frame_now);
            }
        }
        av_packet_unref(Packet_inter);
    }

    if (Frame_now != nullptr || Frame_now != NULL){
        av_frame_free(&Frame_now);
    }
    if (Frame_inter != nullptr || Frame_inter != NULL){
        av_frame_free(&Frame_inter);
    }

    if (Packet_inter != nullptr || Packet_inter != NULL){
        av_packet_free(&Packet_inter);
    }
    avcodec_free_context(&CodecCTX);

    CodecCTX = nullptr;
    Packet_inter = nullptr;
    Frame_inter = nullptr;
    emit stream_ended(index);
}

// deallocating timer is on users hand
void AbstractPlayer::setUp_Timer(QElapsedTimer* timer){
    this->timer = timer;
}

// just some information
void AbstractPlayer::Debug_fun(){
    if (FilePath == ""){
        qDebug() << "no path !";
    }
    qDebug() << "number of streams :";
    qDebug() << FormatCTX->nb_streams;
    qDebug() << "idk debug info :";
    qDebug() << FormatCTX->debug;
    qDebug() << "bit rate :";
    qDebug() << FormatCTX->bit_rate;
    qDebug() << "Packet size :";
    qDebug() << FormatCTX->packet_size;
    qDebug() << "Duration :";
    qDebug() << FormatCTX->duration;
    qDebug() << "start time :";
    qDebug() << FormatCTX->start_time;
    qDebug() << "flags :";
    qDebug() << FormatCTX->flags;
    qDebug() << "probsize :";
    qDebug() << FormatCTX->probesize;
    qDebug() << "AV Time base :";
    qDebug() << AV_TIME_BASE;
    qDebug() << "av time base q :";
    qDebug() << AV_TIME_BASE_Q.num
             << '/' << AV_TIME_BASE_Q.den;
    
    for (int i{0};i<FormatCTX->nb_streams;++i){
        auto stream = FormatCTX->streams[i];
        qDebug() << "stream of index " << i << ':';
        qDebug() << "stream index of stream type :";
        qDebug() << stream->index;
        qDebug() << stream->id;
        qDebug() << "duration :";
        qDebug() << stream->duration;
        qDebug() << "start time :";
        qDebug() << stream->start_time;
        qDebug() << "stream time base :";
        qDebug() << stream->time_base.num << '/' << stream->time_base.den;
        qDebug() << "number of frames :";
        qDebug() << stream->nb_frames;
        qDebug() << "avg frame rate :";
        qDebug() << stream->avg_frame_rate.num
                 << '/' << stream->avg_frame_rate.den;
        qDebug() << "r frame rate :";
        qDebug() << stream->r_frame_rate.num
                 << '/' << stream->r_frame_rate.den;
    }
    
    
    /*
    qDebug() << "interrupt call back :";
    qDebug() << FormatCTX->interrupt_callback;
    */
}

// deprecated algorithm
/*
bool AbstractPlayer::ready_start(){
    Frames_interfaces = new AVFrame*[nb_streams]{nullptr};
    Packets_interfaces = new AVPacket*[nb_streams]{nullptr};
    stream_handler = new QThread[nb_streams];
    Codec_Contexts = new AVCodecContext*[nb_streams]{nullptr};
    for (int i{0};i<nb_streams;++i){

        const AVCodec* Codec{
            avcodec_find_decoder(FormatCTX->streams[i]->codecpar->codec_id)
        };
        if (Codec == nullptr) {
            emit err_No_CodecFound(i);
            continue;
        }
        Codec_Contexts[i]=
            avcodec_alloc_context3(Codec);

        if (avcodec_parameters_to_context(Codec_Contexts[i], FormatCTX->streams[i]->codecpar) < 0) {
            avcodec_free_context(&Codec_Contexts[i]);
            emit err_ParametersToCTX_fail(i);
            continue;
        }

        QObject::connect(&stream_handler[i] , &QThread::started , [=](){
            stream_Handlers
                (i,Codec_Contexts[i] ,Frames_interfaces[i],Packets_interfaces[i]);
        });
        stream_handler[0].start();
    }

    return true;
}

bool AbstractPlayer::ready_Start(){
    Frames_interfaces = new AVFrame*[FormatCTX->nb_streams]{nullptr};
    Packets_interfaces = new AVPacket*[FormatCTX->nb_streams]{nullptr};
    stream_handler = new QThread[FormatCTX->nb_streams];
    for (int i{0};i<FormatCTX->nb_streams;++i){
        //qDebug() << "start " << i;
        QObject::connect(&stream_handler[i] , &QThread::started , [=](){
            stream_Handlers
                (i,FormatCTX->streams[i],Frames_interfaces[i],Packets_interfaces[i]);
        });
        stream_handler[i].start();
        //qDebug() << "end " << i;
    }

    return true;
}
*/
/*
void AbstractPlayer::stream_Handlers
    (int index ,AVStream*& stream , AVFrame*& Frame_interface , AVPacket*& Packet_interface){
    //qDebug() << "thread fucked : " << index  << ' ' << stream->index;
    AVCodecParameters* CodecParameters{stream->codecpar};
    const AVCodec* Codec{
        avcodec_find_decoder(CodecParameters->codec_id)
    };
    if (Codec == nullptr) {
        avcodec_parameters_free(&CodecParameters);
        emit err_No_CodecFound(stream->index);
        return;
    }
    AVCodecContext* CodecCTX{
        avcodec_alloc_context3(Codec)
    };

    if (avcodec_parameters_to_context(CodecCTX, CodecParameters) < 0) {
        avcodec_free_context(&CodecCTX);
        avcodec_parameters_free(&CodecParameters);
        emit err_ParametersToCTX_fail(stream->index);
        return;
    }

    if (avcodec_open2(CodecCTX, Codec, NULL) < 0) {
        avcodec_free_context(&CodecCTX);
        avcodec_parameters_free(&CodecParameters);
        emit err_OpenCodec_fail(stream->index);
        return;
    }

    AVPacket* Packet_now{av_packet_alloc()};
    AVFrame* Frame_now{av_frame_alloc()};
    Frame_interface = av_frame_alloc();
    //AVPacket* Packet_future{av_packet_alloc()};
    //AVFrame* Frame_now{av_frame_alloc()};
    //AVFrame* Frame_future{av_frame_alloc()};
    while (av_read_frame(FormatCTX, Packet_now) >= 0) {
        //qDebug() << Packet_now->stream_index << ' ' << stream->index;
        if (Packet_now->stream_index == stream->index) {
            // sending packet to decoder
            int response = avcodec_send_packet(CodecCTX, Packet_now);
            if (response < 0) {
                av_frame_free(&Frame_now);
                av_frame_free(&Frame_interface);
                av_packet_free(&Packet_now);
                //av_packet_free(&Packet_future);
                avcodec_free_context(&CodecCTX);
                avcodec_parameters_free(&CodecParameters);
                emit err_SendPacket_fail(stream->index);
                return;
            }

            while (response >= 0) {
                Packet_interface = Packet_now;
                emit new_Packet(stream->index);


                response = avcodec_receive_frame(CodecCTX, Frame_now);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    //if (stream->index != 0){
                    //qDebug() << "EOF";
                    //}
                    break;
                } else if (response < 0) {
                    av_frame_free(&Frame_now);
                    av_frame_free(&Frame_interface);
                    //av_frame_free(&Frame_future);
                    av_packet_free(&Packet_now);
                    //av_packet_free(&Packet_future);
                    avcodec_free_context(&CodecCTX);
                    avcodec_parameters_free(&CodecParameters);
                    emit err_DecodingPeriodErr(stream->index);
                    return;
                }

                //qDebug() << Frame_now->pts << ' ' <<  timer.nsecsElapsed()/1000;

                while (Frame_now->pts > timer.nsecsElapsed()/10000){}
                AVFrame* tmp = Frame_interface;
                Frame_interface = Frame_now;
                emit new_Frame(stream->index);
                Frame_now = tmp;

                //qDebug() << Frames_interfaces[0]->pts;
                //qDebug() << "thread fucked : " << index  << ' ' << stream->index;

                av_frame_unref(Frame_now);
            }
        }
        av_packet_unref(Packet_now);
    }

    av_frame_free(&Frame_now);
    av_frame_free(&Frame_interface);
    if (Packet_now != nullptr){
        av_packet_free(&Packet_now);
    }
    avcodec_free_context(&CodecCTX);
    avcodec_parameters_free(&CodecParameters);
    emit stream_ended(stream->index);
}
*/
