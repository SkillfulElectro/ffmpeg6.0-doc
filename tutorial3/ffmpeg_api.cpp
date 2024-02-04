#include "ffmpeg_api.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

ffmpeg_API::ffmpeg_API(QObject *parent)
    : QObject{parent}
{
    Codec_parameters_arr = nullptr;
    Frame_arr = nullptr;
    FormatContext = nullptr;
    Packet = nullptr;
    Codec_CTX_arr = nullptr;
    Codec_arr = nullptr;
    Head_NotFounds = nullptr;
}

void ffmpeg_API::delete_NoCodecFound(){
    Node_NoCodec_Found* tmp = Head_NotFounds;
    while(tmp->next != nullptr){
        Head_NotFounds = tmp->next;
        delete tmp;
        tmp = Head_NotFounds;
    }
}

void ffmpeg_API::delete_LLFrames(Node_Frame* Head){
    if (Head != nullptr){
        Node_Frame* tmp = Head;
        while (tmp->next != nullptr){
            Head = tmp->next;
            av_frame_unref(tmp->Frame);
            av_frame_free(&tmp->Frame);
            delete tmp;
            tmp = Head;
        }
    }
}

void ffmpeg_API::delete_ArrFrames(){
    if (Frame_arr != nullptr){
        for (int i{0};i<Number_Of_Streams;++i){
            delete_LLFrames(Frame_arr[i]);
        }
        delete[] Frame_arr;
    }
}

void ffmpeg_API::delete_CodecCTX_Arr(){
    if (Codec_CTX_arr != nullptr){
        for (int i{0};i<Number_Of_Streams;++i){
            if (Codec_CTX_arr[i].Codec_Context != nullptr){
                avcodec_free_context(&Codec_CTX_arr[i].Codec_Context);
            }
        }
        delete[] Codec_CTX_arr;
    }
}

void ffmpeg_API::delete_CodecPara_Arr(){
    if (Codec_parameters_arr != nullptr){
        for (int i{0};i<Number_Of_Streams;++i){
            if (Codec_parameters_arr[i].stream_CodecParameters != nullptr){
                avcodec_parameters_free
                    (&Codec_parameters_arr[i].stream_CodecParameters);
            }
        }
        delete[] Codec_parameters_arr;
    }
}

ffmpeg_API::~ffmpeg_API(){
    if (Head_NotFounds != nullptr){
        delete_NoCodecFound();
    }
    delete_ArrFrames();
    if (Packet != nullptr){
        av_packet_unref(Packet);
        av_packet_free(&Packet);
    }
    delete[] Codec_arr;
    delete_CodecCTX_Arr();
    delete_CodecPara_Arr();

    avformat_close_input(&FormatContext);
    avformat_free_context(FormatContext);
}

void ffmpeg_API::set_FilePath(QString path){
    filePath = path;
}

bool ffmpeg_API::init_FormatCTX(){
    FormatContext = avformat_alloc_context();
    if (avformat_open_input(&FormatContext, filePath.toStdString().c_str() , NULL, NULL) != 0) {
        avformat_close_input(&FormatContext);
        avformat_free_context(FormatContext);
        emit err_File_NotOpened();
        return false;
    }
    if (avformat_find_stream_info(FormatContext, NULL) < 0) {
        avformat_close_input(&FormatContext);
        avformat_free_context(FormatContext);
        emit err_StreamInfo_fail();
        return false;
    }
    return true;
}

void ffmpeg_API::init_CodecParameters(){
    Number_Of_Streams = FormatContext->nb_streams;
    Codec_parameters_arr = new Stream_Parameters[Number_Of_Streams];
    for (int i{0};i<Number_Of_Streams;++i){
        Codec_parameters_arr[i].stream_type = FormatContext->streams[i]->codecpar->codec_type;
        Codec_parameters_arr[i].stream_CodecParameters = FormatContext->streams[i]->codecpar;
    }
}

void ffmpeg_API::init_Codecs(){
    Codec_arr = new Codecs[Number_Of_Streams];
    for (int i{0};i<Number_Of_Streams;++i){
        Codec_arr[i].Codec = avcodec_find_decoder(Codec_parameters_arr[i].stream_CodecParameters->codec_id);
        Codec_arr[i].stream_Type = Codec_parameters_arr[i].stream_type;
        if (Codec_arr[i].Codec == nullptr){
            Add_NoCodec_Found(i);
        }
    }
}

void ffmpeg_API::init_CodecCTX(){
    Codec_CTX_arr = new Codec_Contexts[Number_Of_Streams];
    for (int i{0};i<Number_Of_Streams;++i){
        if (!Search_NoCodec_Found(i)){
            Codec_CTX_arr[i].Codec_Context = avcodec_alloc_context3(Codec_arr[i].Codec);
            Codec_CTX_arr[i].stream_type = Codec_arr[i].stream_Type;
            if (avcodec_parameters_to_context(Codec_CTX_arr[i].Codec_Context,
                                              Codec_parameters_arr[i].stream_CodecParameters) < 0){
                Add_NoCodec_Found(i);
            }
        }
    }
}

void ffmpeg_API::Add_NoCodec_Found(int index){
    if (!Head_NotFounds){
        Head_NotFounds = new Node_NoCodec_Found;
        Head_NotFounds->stream_index = index;
        Head_NotFounds->next = nullptr;
        Head_NotFounds->prev = nullptr;
    }else{
        Node_NoCodec_Found* NewNode{new Node_NoCodec_Found};
        NewNode->stream_index = index;
        NewNode->next = nullptr;
        NewNode->prev = nullptr;
        Node_NoCodec_Found* tmp = Head_NotFounds;
        for(;tmp->next != nullptr;){
            tmp = tmp->next;
        }
        tmp->next = NewNode;
        NewNode->prev = tmp;
    }
}

bool ffmpeg_API::Search_NoCodec_Found(int index){
    Node_NoCodec_Found* tmp = Head_NotFounds;
    if (tmp == nullptr){
        return false;
    }else{
        while(tmp->next != nullptr){
            if (tmp->stream_index == index){
                return true;
            }
            tmp = tmp->next;
        }
        return false;
    }
}

bool ffmpeg_API::prep_decode(){
    if (init_FormatCTX()){
        init_CodecParameters();
        init_Codecs();
        init_CodecCTX();
        return true;
    }
    return false;
}

bool ffmpeg_API::open_Codec(int Stream_index){
    if (Search_NoCodec_Found(Stream_index)){
        return false;
    }
    if (Frame_arr == nullptr){
        Frame_arr = new Node_Frame*[Number_Of_Streams];
    }
    if (Codec_arr[Stream_index].Codec != nullptr
        && Codec_CTX_arr[Stream_index].Codec_Context != nullptr){
        if (avcodec_open2(Codec_CTX_arr[Stream_index].Codec_Context
                          , Codec_arr[Stream_index].Codec, NULL) < 0) {
            emit err_CodecOpen_fail();
            return false;
        }else{
            return true;
        }
    }else{
        emit err_Codec_CodecCTX_fail();
    }
    return false;
}

bool ffmpeg_API::fill_Frame(int stream_index){
    Node_Frame* tmp;
    if (Frame_arr[stream_index] != nullptr){
        tmp = Frame_arr[stream_index];
        while(tmp->next != nullptr){
            tmp = tmp->next;
        }
    }else{
        Frame_arr[stream_index] = new Node_Frame;
        tmp = Frame_arr[stream_index];
        tmp->Frame = nullptr;
        tmp->next = nullptr;
        tmp->prev = nullptr;
    }
    while (av_read_frame(FormatContext , Packet)>0){
        int response = avcodec_send_packet(Codec_CTX_arr[stream_index].Codec_Context
                                           , Packet);
        if (response < 0){
            emit err_Sending_Packet_fail();
            return false;
        }
        Packet = av_packet_alloc();

        while(response >= 0){
            if (tmp->Frame != nullptr){
                tmp->next = new Node_Frame;
                tmp->next->prev = tmp;
                tmp->next->Frame = nullptr;
                tmp = tmp->next;
            }
            tmp->Frame = av_frame_alloc();
            response = avcodec_receive_frame(Codec_CTX_arr[stream_index].Codec_Context,
                                             tmp->Frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            }
        }
        av_packet_unref(Packet);
    }
    return true;
}
