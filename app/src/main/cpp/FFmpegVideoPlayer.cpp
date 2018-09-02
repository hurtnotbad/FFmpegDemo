//
// Created by Lammy on 2018/9/2.
//

#include "FFmpegVideoPlayer.h"
#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include "unistd.h"

//#include "convert_argb.h"

extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

#define FFLOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg",FORMAT,##__VA_ARGS__);
#define FFLOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg",FORMAT,##__VA_ARGS__);


FFmpegVideoPlayer::FFmpegVideoPlayer() {

}
FFmpegVideoPlayer::~FFmpegVideoPlayer() {

}


void FFmpegVideoPlayer::pause() {

}

void FFmpegVideoPlayer::stop() {

}

void FFmpegVideoPlayer::play(const char *input ,ANativeWindow* aNativeWindow ) {


    // TODO
    //1.注册所有组件
    av_register_all();

    //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0)
    {
        FFLOGE("%s","无法打开输入视频文件");
        return;
    }

    //3.获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
    {
        FFLOGE("%s","无法获取视频文件信息");
        return;
    }

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    //number of streams
    for (; i < pFormatCtx->nb_streams; i++)
    {
        //流的类型
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            v_stream_idx = i;
            break;
        }
    }

    if (v_stream_idx == -1)
    {
        FFLOGE("%s","找不到视频流\n");
        return;
    }

    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
    //获取视频流中的编解码上下文
    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
    //4.根据编解码上下文中的编码id查找对应的解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        FFLOGE("%s","找不到解码器\n");
        return;
    }

    //5.打开解码器
    if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
    {
        FFLOGE("%s","解码器无法打开\n");
        return;
    }

    //输出视频信息
    FFLOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
    FFLOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
    FFLOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
    FFLOGI("解码器的名称：%s",pCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *pFrame = av_frame_alloc();
    //YUV420
    AVFrame *pFrameYUV = av_frame_alloc();


    AVFrame * rgb_frame = av_frame_alloc();
    ANativeWindow_Buffer outBuffer;

    AVPixelFormat src_pixfmt=AV_PIX_FMT_YUV420P;
    int src_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(src_pixfmt));
    uint8_t *temp_buffer=(uint8_t *)malloc(pCodecCtx->height*pCodecCtx->width*src_bpp/8);







    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //初始化缓冲区
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
//    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
//                                                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
//                                                SWS_BICUBIC, NULL, NULL, NULL);
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
                                                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA,
                                                SWS_BICUBIC, NULL, NULL, NULL);
//    struct SwsContext *sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
//                                      int dstW, int dstH, enum AVPixelFormat dstFormat,
//                                      int flags, SwsFilter *srcFilter,
//                                      SwsFilter *dstFilter, const double *param);

    int got_picture, ret;
    int frame_count = 0;

    //6.一帧一帧的读取压缩数据
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        //只要视频压缩数据（根据流的索引位置判断）
        if (packet->stream_index == v_stream_idx)
        {
            //7.解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0)
            {
                FFLOGE("%s","解码错误");
                return;
            }

            //为0说明解码完成，非0正在解码
            if (got_picture)
            {
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

//                FFLOGI("解码帧 w = %d h = %d",pCodecCtx->width ,  pCodecCtx->height);
//                ANativeWindow_setBuffersGeometry(aNativeWindow , pCodecCtx->width ,  pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
//                ANativeWindow_lock(aNativeWindow,&outBuffer,NULL);
//////                ANativeWindow_lock(aNativeWindow,(ANativeWindow_Buffer*)temp_buffer,NULL);
//////
////                avpicture_fill((AVPicture *)rgb_frame , (const uint8_t *) outBuffer.bits, AV_PIX_FMT_RGBA , pCodecCtx->width ,  pCodecCtx->height);
//                avpicture_fill((AVPicture *)rgb_frame , (const uint8_t *) outBuffer.bits, AV_PIX_FMT_RGBA , pCodecCtx->width , pCodecCtx->height);
//////
//                sws_scale(img_convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
//                sws_scale(sws_ctx, rgb_frame->data, pFrame->linesize, 0, pCodecCtx->height,
//                          rgb_frame->data, rgb_frame->linesize);
//
//////               // yuv to rgb
//                int src_w = pCodecCtx->width;
//                int src_h = pCodecCtx->height;
//
//
//////
//                ANativeWindow_unlockAndPost(aNativeWindow);


                frame_count++;
                FFLOGI("解码第%d帧",frame_count);
            }
        }

        //释放资源
        av_free_packet(packet);
    }


    ANativeWindow_release(aNativeWindow);
    av_frame_free(&pFrame);

    avcodec_close(pCodecCtx);

    avformat_free_context(pFormatCtx);







}
extern "C"
{

#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

// void YUV2RGB(AVFrame *pFrameYUV ,AVFrame * rgb_frame = av_frame_alloc() )
//{
////Parameters
//     FILE *src_file =fopen("sintel_480x272_yuv420p.yuv", "rb");
//     const int src_w=960,src_h=540;
//     AVPixelFormat src_pixfmt=AV_PIX_FMT_YUV420P;
//
//     int src_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(src_pixfmt));
//
//     FILE *dst_file = fopen("sintel_1280x720_rgb24.rgb", "wb");
//     const int dst_w=960,dst_h=540;
//     AVPixelFormat dst_pixfmt=AV_PIX_FMT_RGB24;
//     int dst_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(dst_pixfmt));
//
//     //Structures
//     uint8_t *src_data[4];
//     int src_linesize[4];
//
//     uint8_t *dst_data[4];
//     int dst_linesize[4];
//
//     int rescale_method=SWS_BICUBIC;
//     struct SwsContext *img_convert_ctx;
//     uint8_t *temp_buffer=(uint8_t *)malloc(src_w*src_h*src_bpp/8);
//
//     int frame_idx=0;
//     int ret=0;
//     ret= av_image_alloc(src_data, src_linesize,src_w, src_h, src_pixfmt, 1);
//     if (ret< 0) {
//         printf( "Could not allocate source image\n");
//         return ;
//     }
//     ret = av_image_alloc(dst_data, dst_linesize,dst_w, dst_h, dst_pixfmt, 1);
//     if (ret< 0) {
//         printf( "Could not allocate destination image\n");
//         return ;
//     }
//     //-----------------------------
//     //Init Method 1
//     img_convert_ctx =sws_alloc_context();
//     //Show AVOption
//     av_opt_show2(img_convert_ctx,stdout,AV_OPT_FLAG_VIDEO_PARAM,0);
//     //Set Value
//     av_opt_set_int(img_convert_ctx,"sws_flags",SWS_BICUBIC|SWS_PRINT_INFO,0);
//     av_opt_set_int(img_convert_ctx,"srcw",src_w,0);
//     av_opt_set_int(img_convert_ctx,"srch",src_h,0);
//     av_opt_set_int(img_convert_ctx,"src_format",src_pixfmt,0);
//     //'0' for MPEG (Y:0-235);'1' for JPEG (Y:0-255)
//     av_opt_set_int(img_convert_ctx,"src_range",1,0);
//     av_opt_set_int(img_convert_ctx,"dstw",dst_w,0);
//     av_opt_set_int(img_convert_ctx,"dsth",dst_h,0);
//     av_opt_set_int(img_convert_ctx,"dst_format",dst_pixfmt,0);
//     av_opt_set_int(img_convert_ctx,"dst_range",1,0);
//     sws_init_context(img_convert_ctx,NULL,NULL);
//
//     //Init Method 2
//     //img_convert_ctx = sws_getContext(src_w, src_h,src_pixfmt, dst_w, dst_h, dst_pixfmt,
//     //	rescale_method, NULL, NULL, NULL);
//     //-----------------------------
//     /*
//     //Colorspace
//     ret=sws_setColorspaceDetails(img_convert_ctx,sws_getCoefficients(SWS_CS_ITU601),0,
//         sws_getCoefficients(SWS_CS_ITU709),0,
//          0, 1 << 16, 1 << 16);
//     if (ret==-1) {
//         printf( "Colorspace not support.\n");
//         return -1;
//     }
//     */
//     while(1)
//     {
//         if (fread(temp_buffer, 1, src_w*src_h*src_bpp/8, src_file) != src_w*src_h*src_bpp/8){
//             break;
//         }
//
//         switch(src_pixfmt){
//             case AV_PIX_FMT_GRAY8:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h);
//                 break;
//             }
//             case AV_PIX_FMT_YUV420P:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
//                 memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/4);      //U
//                 memcpy(src_data[2],temp_buffer+src_w*src_h*5/4,src_w*src_h/4);  //V
//                 break;
//             }
//             case AV_PIX_FMT_YUV422P:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
//                 memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h/2);      //U
//                 memcpy(src_data[2],temp_buffer+src_w*src_h*3/2,src_w*src_h/2);  //V
//                 break;
//             }
//             case AV_PIX_FMT_YUV444P:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h);                    //Y
//                 memcpy(src_data[1],temp_buffer+src_w*src_h,src_w*src_h);        //U
//                 memcpy(src_data[2],temp_buffer+src_w*src_h*2,src_w*src_h);      //V
//                 break;
//             }
//             case AV_PIX_FMT_YUYV422:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h*2);                  //Packed
//                 break;
//             }
//             case AV_PIX_FMT_RGB24:{
//                 memcpy(src_data[0],temp_buffer,src_w*src_h*3);                  //Packed
//                 break;
//             }
//             default:{
//                 printf("Not Support Input Pixel Format.\n");
//                 break;
//             }
//         }
//
//         sws_scale(img_convert_ctx, src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
//         printf("Finish process frame %5d\n",frame_idx);
//         frame_idx++;
//
//         switch(dst_pixfmt){
//             case AV_PIX_FMT_GRAY8:{
//                 fwrite(dst_data[0],1,dst_w*dst_h,dst_file);
//                 break;
//             }
//             case AV_PIX_FMT_YUV420P:{
//                 fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
//                 fwrite(dst_data[1],1,dst_w*dst_h/4,dst_file);               //U
//                 fwrite(dst_data[2],1,dst_w*dst_h/4,dst_file);               //V
//                 break;
//             }
//             case AV_PIX_FMT_YUV422P:{
//                 fwrite(dst_data[0],1,dst_w*dst_h,dst_file);					//Y
//                 fwrite(dst_data[1],1,dst_w*dst_h/2,dst_file);				//U
//                 fwrite(dst_data[2],1,dst_w*dst_h/2,dst_file);				//V
//                 break;
//             }
//             case AV_PIX_FMT_YUV444P:{
//                 fwrite(dst_data[0],1,dst_w*dst_h,dst_file);                 //Y
//                 fwrite(dst_data[1],1,dst_w*dst_h,dst_file);                 //U
//                 fwrite(dst_data[2],1,dst_w*dst_h,dst_file);                 //V
//                 break;
//             }
//             case AV_PIX_FMT_YUYV422:{
//                 fwrite(dst_data[0],1,dst_w*dst_h*2,dst_file);               //Packed
//                 break;
//             }
//             case AV_PIX_FMT_RGB24:{
//                 fwrite(dst_data[0],1,dst_w*dst_h*3,dst_file);               //Packed
//                 break;
//             }
//             default:{
//                 printf("Not Support Output Pixel Format.\n");
//                 break;
//             }
//         }
//     }
//
//     sws_freeContext(img_convert_ctx);
//
//     free(temp_buffer);
//     fclose(dst_file);
//     av_freep(&src_data[0]);
//     av_freep(&dst_data[0]);
//
//
//
//
// }








//void play2(const char *input ,ANativeWindow* aNativeWindow ) {
//
////    const char *input = env->GetStringUTFChars(input_, 0);
////    const char *output = env->GetStringUTFChars(output_, 0);
//
//    // TODO
//    //1.注册所有组件
//    av_register_all();
//
//    //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
//    AVFormatContext *pFormatCtx = avformat_alloc_context();
//
//    //2.打开输入视频文件
//    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0)
//    {
//        FFLOGE("%s","无法打开输入视频文件");
//        return;
//    }
//
//    //3.获取视频文件信息
//    if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
//    {
//        FFLOGE("%s","无法获取视频文件信息");
//        return;
//    }
//
//    //获取视频流的索引位置
//    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
//    int v_stream_idx = -1;
//    int i = 0;
//    //number of streams
//    for (; i < pFormatCtx->nb_streams; i++)
//    {
//        //流的类型
//        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
//        {
//            v_stream_idx = i;
//            break;
//        }
//    }
//
//    if (v_stream_idx == -1)
//    {
//        FFLOGE("%s","找不到视频流\n");
//        return;
//    }
//
//    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
//    //获取视频流中的编解码上下文
//    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
//    //4.根据编解码上下文中的编码id查找对应的解码
//    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
//    if (pCodec == NULL)
//    {
//        FFLOGE("%s","找不到解码器\n");
//        return;
//    }
//
//    //5.打开解码器
//    if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
//    {
//        FFLOGE("%s","解码器无法打开\n");
//        return;
//    }
//
//    //输出视频信息
//    FFLOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
//    FFLOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
//    FFLOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
//    FFLOGI("解码器的名称：%s",pCodec->name);
//
//    //准备读取
//    //AVPacket用于存储一帧一帧的压缩数据（H264）
//    //缓冲区，开辟空间
//    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
//
//
//    //YUV420
//    AVFrame *pFrameYUV = av_frame_alloc();
//    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
////    //缓冲区分配内存
//    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
////
//    //初始化缓冲区
//    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
////
////    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
//    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
//                                                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
//                                                SWS_BICUBIC, NULL, NULL, NULL);
//
//    //AVFrame用于存储解码后的像素数据(YUV)
//    //内存分配
//    AVFrame *pFrame = av_frame_alloc();
//    AVFrame * rgb_frame = av_frame_alloc();
//    ANativeWindow_Buffer outBuffer;
//
//    AVPixelFormat src_pixfmt=AV_PIX_FMT_YUV420P;
//    int src_bpp=av_get_bits_per_pixel(av_pix_fmt_desc_get(src_pixfmt));
//    uint8_t *temp_buffer=(uint8_t *)malloc(pCodecCtx->height*pCodecCtx->width*src_bpp/8);
//
//    FFLOGI("开始解码");
//    int got_picture, ret;
////    FILE *fp_yuv = fopen(output, "wb+");
//    int frame_count = 0;
//    //6.一帧一帧的读取压缩数据
//    while (av_read_frame(pFormatCtx, packet) >= 0)
//    {
//        FFLOGI("解码中");
//        //只要视频压缩数据（根据流的索引位置判断）
//        if (packet->stream_index == v_stream_idx)
//        {
//            //7.解码一帧视频压缩数据，得到视频像素数据
//            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
//            if (ret < 0)
//            {
//                FFLOGE("%s","解码错误");
//                return;
//            }
//
//
//            //为0说明解码完成，非0正在解码
//            if (got_picture)
//            {
//                FFLOGE("%s","解码绘制");
//                // 设置缓冲区大小
//                 ANativeWindow_setBuffersGeometry(aNativeWindow , pCodecCtx->width ,  pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
//                //AVFrame转为像素格式YUV420，宽高
//                //2 6输入、输出数据
//                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
//                //4 输入数据第一列要转码的位置 从0开始
//                //5 输入画面的高度
////                sws_scale(sws_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
////                          pFrameYUV->data, pFrameYUV->linesize);
//
//
//////                ANativeWindow_lock(aNativeWindow,&outBuffer,NULL);
////                ANativeWindow_lock(aNativeWindow,(ANativeWindow_Buffer*)temp_buffer,NULL);
////
//////                avpicture_fill((AVPicture *)rgb_frame , (const uint8_t *) outBuffer.bits, AV_PIX_FMT_RGBA , pCodecCtx->width ,  pCodecCtx->height);
////                avpicture_fill((AVPicture *)rgb_frame , temp_buffer, AV_PIX_FMT_RGBA , pCodecCtx->width ,  pCodecCtx->height);
////
////               // yuv to rgb
////                int src_w = pCodecCtx->width;
////                int src_h = pCodecCtx->height;
////
////                memcpy(pFrame->data[0],temp_buffer,src_w*src_h);                    //Y
////                memcpy(pFrame->data[1],temp_buffer+src_w*src_h,src_w*src_h/4);      //U
////                memcpy(pFrame->data[2],temp_buffer+src_w*src_h*5/4,src_w*src_h/4);  //V
//
//
////                ANativeWindow_unlockAndPost(aNativeWindow);
//
//
//
//
//
////
////                //输出到YUV文件
////                //AVFrame像素帧写入文件
////                //data解码后的图像像素数据（音频采样数据）
////                //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
////                //U V 个数是Y的1/4
////                int y_size = pCodecCtx->width * pCodecCtx->height;
////                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);
////                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);
////                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);
//
//                frame_count++;
//                FFLOGI("解码第%d帧",frame_count);
//                usleep(1000*16);
//            }
//        }else{
//            FFLOGI("解码失败");
//        }
//
//        //释放资源
//        av_free_packet(packet);
//    }
//
////    fclose(fp_yuv);
//
//    av_frame_free(&pFrame);
//
////    ANativeWindow_release(aNativeWindow);
//    avcodec_close(pCodecCtx);
//
//    avformat_free_context(pFormatCtx);
//
//
//
//}








