#ifndef FFMPEG_API_H
#define FFMPEG_API_H

#include <QObject>
/*
* including headers
*/
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// Node for linked list of undefined indexes
struct Node_NoCodec_Found{
    int stream_index;
    Node_NoCodec_Found* next;
    Node_NoCodec_Found* prev;
    Node_NoCodec_Found(){
        next = nullptr;
        prev = nullptr;
    }
};

// type for array of AVCodec of each stream
struct Codecs{
    const AVCodec* Codec;
    int stream_Type;
    Codecs(){
        Codec = nullptr;
    }
};

// type for array of AVCodecParameters of each stream
struct Stream_Parameters{
    int stream_type;
    AVCodecParameters* stream_CodecParameters;
    Stream_Parameters(){
        stream_CodecParameters = nullptr;
    }
};

// type for array of AVCodecContext of each stream
struct Codec_Contexts{
    AVCodecContext* Codec_Context;
    int stream_type;
    Codec_Contexts(){
        Codec_Context = nullptr;
    }
};

// type for linked list of frames of each stream
struct Node_Frame{
    AVFrame* Frame;
    Node_Frame* next;
    Node_Frame* prev;
    Node_Frame(){
        Frame = nullptr;
        next = nullptr;
        prev = nullptr;
    }
};

class ffmpeg_API : public QObject
{
    Q_OBJECT
public:
    explicit ffmpeg_API(QObject *parent = nullptr);
    ~ffmpeg_API();

    // public for getting info like stream index and etc
    Stream_Parameters* Codec_parameters_arr;
    // public for knowing how many streams are available
    int Number_Of_Streams;
    // Array of linked list sort of frames of each stream
    Node_Frame** Frame_arr;
public slots :
    // init file path
    void set_FilePath(QString Path);
    // preparing ffmpeg for decoding
    bool prep_decode();
    // opening codec for spacific stream
    bool open_Codec(int Stream_index);
    // filling linked list array of spacific stream
    bool fill_Frame(int stream_index);
private slots:
    // ffmpeg initialization
    bool init_FormatCTX();
    void init_CodecParameters();
    void init_Codecs();
    void Add_NoCodec_Found(int index);
    bool Search_NoCodec_Found(int index);
    void init_CodecCTX();

    // destruction of object
    void delete_NoCodecFound();
    void delete_ArrFrames();
    void delete_LLFrames(Node_Frame* Head);
    void delete_CodecCTX_Arr();
    void delete_CodecPara_Arr();
private:
    QString filePath;
    AVFormatContext* FormatContext;
    AVPacket* Packet;


    Codec_Contexts* Codec_CTX_arr;
    Codecs* Codec_arr;

    Node_NoCodec_Found* Head_NotFounds;
signals:
    void Decoded();
    void err_File_NotOpened();
    void err_StreamInfo_fail();
    void err_CodecOpen_fail();
    void err_Codec_CodecCTX_fail();
    void err_fill_Frame_fail();
    void err_Sending_Packet_fail();
};

#endif // FFMPEG_API_H
