/*
 * Hello so basically ffmpeg 6.0 , doesnt really have good doc for libav or i didnt find it so lets
 * try to write one , in tutorial 1 we going to open a file , find its video stream , decode it to packets
 * and then save it as .pgm file
 * try to write one , thanks to ppl who gonna help me in this path
 * so just follow the main function and read the comments
 *
 * by the way , there is Garbage dump Area in the code , and thats because i didnt want to fully
 * remove my mistakes in ffmpeg6.0 , they might come in handy one day
*/

// here im just adding header files for Qt framework i/o system of cpp
#include <QCoreApplication>
#include <iostream>

// adding required libraries of libav
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, const char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}


int main(int argc, char *argv[])
{
    // declaring Qt Console app , if you dont use this framework , dw abt this line
    QCoreApplication a(argc, argv);

    std::string filePath {
        "F:/Media/movies/9.2009.720p.BluRay.HardSub.DigiMoviez.mp4"
    };

    /*
     * its time to start working with our video , so how do we that?
     *
     * so video files are based on containers and each container has its own header
     * so we create AVFormatContext and allocate memory for it to hold information about
     * our video file
    */
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    /*
     * to open it in version ffmpeg6.0 , we use
     *
     * avformat_open_input(address of our AVFormatContext var , path of our video file in const char* ,
     *                      AVInputFormat : it is used to pass the format of file to ffmpeg ,
     *                      AVDictionary : which are options to the demuxer)
     *
     * if this function gives zero , it means everything went well
    */
    if (avformat_open_input(&pFormatCtx, filePath.c_str() , NULL, NULL) != 0) {
        std::cerr << "Error: Couldn't open file" << std::endl;
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return -1;
    }

    /*
     * now we have information about our file , saved on pFormatCtx and can use them , ex : duration , ...
     *
     * its time to access streams of the media , for accessing the streams of media we have to get that information
     * via
     *
     *  avformat_find_stream_info(AVFormatContext , AVDictionary struct containing options for the demuxer)
     *
     *  if this function returns number 0 or bigger , it means everything is fine
    */

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        std::cerr << "Error: Couldn't find stream information" << std::endl;
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return -1;
    }

    /*
     * so what are streams ?
     *
     *      so basically streams are data of one type like video , subtitles , audio and etc
     *      which are encoded based on Codecs
    */

    /*
     * so we gonna work with video stream type , so we loop through all the streams and check
     * their type , if the type is AVMEDIA_TYPE_VIDEO , we found our stream which we were looking for
    */
    // the videoStream var here will keep the index of the stream
    int videoStream = -1;
    /*
     * so as we said before each stream is encoded with different Codec , so how do we find out which
     * and another information abt it?
     *
     *            thats where AVCodecParameters struct comes in handy which will save info about codec ,
     *            bitrate , resolution and etc
    */
    // so we create a AVCodeParameters ptr here to save our video stream info in it
    AVCodecParameters *pCodecParams = nullptr;
    // nb_streams var here which lives in AVFormatContext , keeps the numbers of streams in our file
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        /*
         * streams is array of AVStream pointers , for each stream we use it to access the
         * stream info , and codecpar which lives in AVStream struct is AVCodecParameters and one of its
         * members is codec_type which is enum and shows the type of codec used to encode the stream
        */
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // after finding the stream which we wanted , we save its index and parameters of it for future use
            videoStream = i;
            pCodecParams = pFormatCtx->streams[i]->codecpar;
            break;
        }
    }

    // if videoStream is -1 , means there is no video type stream
    if (videoStream == -1) {
        std::cerr << "Error: Couldn't find video stream" << std::endl;
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return -1;
    }

    /*
     * now we have to find the codec for decoding the file , for that we use
     * avcodec_find_decoder(AVCodecID)
     * AVCodecID is a enum type which shows the codec which used for encoding data
     * there is a member which lives in AVCodecParameters which gives us that the codec_id
     *
     *
     * AVCodec is a struct in FFmpeg that represents a media codec.
     * It contains information about the codec, such as its name, type, and capabilities
    */
    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    if (pCodec == nullptr) {
        std::cerr << "Error: Couldn't find codec" << std::endl;
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return -1;
    }

    /*
     * AVCodecContext is a struct in FFmpeg that represents a context for a media codec.
     * It contains information about the codec, the stream that is being decoded or encoded,
     * and the current state of the codec
    */
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    /*
     * now we have to move the parameters of the codec to the Context
    */
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParams) < 0) {
        std::cerr << "Error: Couldn't copy codec parameters to codec context" << std::endl;
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        return -1;
    }

    // opening Codec for decoding , the last para is AVDictionary
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        std::cerr << "Error: Couldn't open codec" << std::endl;
        return -1;
    }

    /*
     *
    In FFmpeg and videos, packets and frames are two different concepts.

    A packet is a chunk of data that is sent or received over a network or stored in a file. Packets can contain different types of data, such as video frames, audio frames, and text subtitles.

    A frame is a single image or audio sample. Frames are the basic building blocks of video and audio streams.

    The main difference between packets and frames is that packets are used to transport data, while frames are the actual data.

    In FFmpeg, packets are represented by the AVPacket struct. Frames are represented by the AVFrame struct.

    The AVPacket struct contains information about the packet, such as its size, timestamp, and stream index. The AVFrame struct contains the actual data for the frame, such as the video pixels or audio samples.

    FFmpeg uses packets to decode and encode media streams. FFmpeg also uses packets to mux and demux media streams.

    When demuxing a media stream, FFmpeg reads packets from the input file and decodes them into frames. When muxing a media stream, FFmpeg encodes frames into packets and writes them to the output file.

    Here is a diagram that illustrates the relationship between packets and frames:

    Packet -> Frame -> Video/Audio
    Packets are used to transport frames, and frames are used to decode/encode video and audio.
    */
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();

    int frameNumber = 0;

    while (av_read_frame(pFormatCtx, pPacket) >= 0) {
        if (pPacket->stream_index == videoStream) {
            int response = avcodec_send_packet(pCodecCtx, pPacket);
            if (response < 0) {
                qCritical() << "Error sending a packet for decoding";
                return -1;
            }

            while (response >= 0) {
                response = avcodec_receive_frame(pCodecCtx, pFrame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    qCritical() << "Error during decoding";
                    return -1;
                }

                std::cout << "at frame : " << frameNumber << '\n';
                std::cout << "frame format : " << pFrame->format << '\n';


                QString fileName {
                    QString("H:/projects_codes/Video_tests/frame_") + QString::number(frameNumber) + QString(".pgm")
                };

                save_gray_frame(pFrame->data[0] , pFrame->linesize[0] , pFrame->width , pFrame->height , fileName.toStdString().c_str());


                frameNumber++;
            }
        }
        av_packet_unref(pPacket);
    }


    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);


    return a.exec();
}



// Garbage Dump :
/*
                FILE *outputFile = fopen(fileName.toStdString().c_str(), "w");
                //FILE *f;
                int i;
                //f = fopen(filename,"w");
                // writing the minimal required header for a pgm file format
                // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
                fprintf(outputFile, "P5\n%d %d\n%d\n", pFrame->width, pFrame->height, 255);

                // writing line by line
                for (i = 0; i < pFrame->height; i++)
                    fwrite(img.constBits() + i * pFrame->linesize[0], 1, pFrame->width, outputFile);
                fclose(outputFile);

                /*
                fwrite(img.constBits(), 1 , img.sizeInBytes()*8 , outputFile);
                fclose(outputFile);
                */

//img.constBits()

//QImage img()
// Save the decoded frame as a .png file
//std::cout << "calling the function ! \n";
//saveFrameAsPNG(pFrame, pCodecCtx->width, pCodecCtx->height, frameNumber);
//std::cout << frameNumber << '\n';
//QImage img(pFrame->data[0], pFrame->width, pFrame->height, QImage::Format_RGB888);
//std::cout << img.pixelColor(20 , 10).green() << '\n';
/*
    AVFormatContext *pFormatContext = avformat_alloc_context();
    int Err_check;
    Err_check = avformat_open_input(&pFormatContext , filePath.c_str() , NULL , NULL);
    if (Err_check != 0){
        std::cout << "the result of error checking : " << Err_check << '\n';
        return -1;
    }

    if (pFormatContext->duration < 0){
        std::cout << "format : " << pFormatContext->iformat->long_name << " , duration : unknown" << '\n';
    }else{
        std::cout << "format : " << pFormatContext->iformat->long_name << " , duration : " << pFormatContext->duration << '\n';
    }

    std::cout << "used size : " << pFormatContext->probesize << '\n';

    Err_check = avformat_find_stream_info(pFormatContext , NULL);
    if (Err_check != 0){
        std::cout << "the result of error checking : " << Err_check << '\n';
        return -1;
    }

    std::cout << "number of streams : " << pFormatContext->nb_streams << '\n';

    std::cout << "starting to dig into streams\n\n";
    for (int i{0};i<pFormatContext->nb_streams;++i){
        AVStream* stream {
            pFormatContext->streams[i]
        };

        std::cout << "index of stream : " << stream->index << '\n';
        if (stream->duration < 0 ){
            std::cout << "duration is unknown !\n";
        }else{
            std::cout << "the duration of stream : " << stream->duration << '\n';
        }
        std::cout << "the codec which it needs : " << stream->codecpar->profile << '\n';
        std::cout << "Resolution : " << stream->codecpar->height << 'x' << stream->codecpar->width << '\n';

        std::cout << "Codec type number : " << stream->codecpar->codec_type << '\n';
        std::cout << "Id of the codec : " << stream->codecpar->codec_id << '\n';
        switch (stream->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            printf("Stream type : Video\n");
            break;
        case AVMEDIA_TYPE_AUDIO:
            printf("Stream type : Audio\n");
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            printf("Stream type : Subtitle\n");
            break;
        case AVMEDIA_TYPE_DATA:
            printf("Stream type : Data\n");
            break;
        case AVMEDIA_TYPE_ATTACHMENT:
            printf("Stream type : Attachment\n");
            break;
        default:
            printf("Stream type : Unknown type\n");
            break;
        }

        AVCodecParameters* pLocalCodecPar {pFormatContext->streams[i]->codecpar};
        const AVCodec* pLocalCodec{avcodec_find_decoder(pLocalCodecPar->codec_id)};
        printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecPar->bit_rate);

        AVCodecContext* pCodecContext{
            avcodec_alloc_context3(pLocalCodec)
        };
        avcodec_parameters_to_context(pCodecContext , pLocalCodecPar);
        avcodec_open2(pCodecContext , pLocalCodec , NULL);

        AVPacket *pPacket = av_packet_alloc();
        AVFrame *pFrame = av_frame_alloc();

        // Read a packet from the video file.
        while (av_read_frame(pFormatContext, pPacket) >= 0) {
            // Send the packet to the codec for decoding.
            avcodec_send_packet(pCodecContext, pPacket);

            // Receive a decoded frame from the codec.
            while (avcodec_receive_frame(pCodecContext, pFrame) >= 0) {
                // Do something with the decoded frame here.
                // For example, you could display the frame or write it to a new video file.

                std::cout << "Key frame : " << pFrame->key_frame;

                av_frame_free(&pFrame);
                pFrame = av_frame_alloc();
                int stop;
                std::cin >> stop;
            }
        }
        // Free the packet.
        av_packet_free(&pPacket);


        std::cout << "\n\n";
    }

    std::cout << "done!";
    */
/*
void saveFrameAsPNG(AVFrame *pFrame, int width, int height, int frameNumber) {
    QString fileName {
        QString("H:/projects_codes/Video_tests/frame_") + QString::number(frameNumber) + QString(".png")
    };
    // Create a QFile object for the output file.
    QFile outFile(fileName);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Error opening output file";
        return;
    }

    // Create a SwsContext object for scaling the frame.
    SwsContext *pCtx = sws_getContext((AVPixelFormat)width, (AVPixelFormat)height, pFrame->format, width, height, AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (pCtx == nullptr) {
        qCritical() << "Error creating SwsContext";
        return;
    }

    // Allocate a buffer for the scaled frame.
    uint8_t *pScaledFrame = av_malloc(width * height * 4);
    if (pScaledFrame == nullptr) {
        qCritical() << "Error allocating buffer for scaled frame";
        return;
    }

    // Scale the frame.
    sws_scale(pCtx, pFrame->data, pFrame->linesize, 0, height, &pScaledFrame, &width);

    // Write the scaled frame to the output file.
    outFile.write(reinterpret_cast<const char*>(pScaledFrame), width * height * 4);

    // Close the output file.
    outFile.close();

    // Free the scaled frame buffer.
    av_free(pScaledFrame);

    // Free the SwsContext object.
    sws_freeContext(pCtx);
}
*/
