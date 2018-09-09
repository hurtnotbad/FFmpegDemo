//
// Created by Lammy on 2018/9/2.
//

#include "FFmpegVideoPlayer.h"
#include <jni.h>
//#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "unistd.h"
#include "convert_argb.h"



extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg",FORMAT,##__VA_ARGS__);


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
        LOGE("%s","无法打开输入视频文件");
        return;
    }

    //3.获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
    {
        LOGE("%s","无法获取视频文件信息");
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
        LOGE("%s","找不到视频流\n");
        return;
    }

    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
    //获取视频流中的编解码上下文
    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
    //4.根据编解码上下文中的编码id查找对应的解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        LOGE("%s","找不到解码器\n");
        return;
    }

    //5.打开解码器
    if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
    {
        LOGE("%s","解码器无法打开\n");
        return;
    }

    //输出视频信息
    LOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
    LOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
    LOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
    LOGI("解码器的名称：%s",pCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *pFrame = av_frame_alloc();


    AVFrame * rgb_frame = av_frame_alloc();

    ANativeWindow_Buffer outBuffer;


    ANativeWindow * nativeWindow = aNativeWindow;

    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height));



    int got_picture, ret;
//    int frame_count = 0;

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
                LOGE("%s","解码错误");
                return;
            }

            //为0说明解码完成，非0正在解码
            if (got_picture)
            {
                LOGE("%s","解码成功");
                ANativeWindow_lock(aNativeWindow,&outBuffer,NULL);
//				////
                LOGE("%s","解码成功2");
                ANativeWindow_setBuffersGeometry(aNativeWindow,pCodecCtx->width,pCodecCtx->height,WINDOW_FORMAT_RGBA_8888);
                avpicture_fill((AVPicture *)rgb_frame,out_buffer, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height);
                LOGE("%s","解码成功3");
//

                libyuv::I420ToARGB(pFrame->data[0],pFrame->linesize[0],
                           pFrame->data[2],pFrame->linesize[2],
                           pFrame->data[1],pFrame->linesize[1],
                           rgb_frame->data[0], rgb_frame->linesize[0],
                           pCodecCtx->width,pCodecCtx->height);
                LOGE("%s","解码成功4");
                int h = 0;
//                for (h = 0; h < pCodecCtx->height; h++) {
//                    memcpy((outBuffer.bits + h * outBuffer.stride*4), out_buffer + h * rgb_frame->linesize[0], rgb_frame->linesize[0]);
//                }
                memcpy(outBuffer.bits,out_buffer,pCodecCtx->width*pCodecCtx->height*4);
                LOGE("%s","解码成功5");
                ANativeWindow_unlockAndPost(aNativeWindow);


//                frame_count++;
//                LOGI("解码第%d帧",frame_count);
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
//        LOGE("%s","无法打开输入视频文件");
//        return;
//    }
//
//    //3.获取视频文件信息
//    if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
//    {
//        LOGE("%s","无法获取视频文件信息");
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
//        LOGE("%s","找不到视频流\n");
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
//        LOGE("%s","找不到解码器\n");
//        return;
//    }
//
//    //5.打开解码器
//    if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
//    {
//        LOGE("%s","解码器无法打开\n");
//        return;
//    }
//
//    //输出视频信息
//    LOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
//    LOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
//    LOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
//    LOGI("解码器的名称：%s",pCodec->name);
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
//    LOGI("开始解码");
//    int got_picture, ret;
////    FILE *fp_yuv = fopen(output, "wb+");
//    int frame_count = 0;
//    //6.一帧一帧的读取压缩数据
//    while (av_read_frame(pFormatCtx, packet) >= 0)
//    {
//        LOGI("解码中");
//        //只要视频压缩数据（根据流的索引位置判断）
//        if (packet->stream_index == v_stream_idx)
//        {
//            //7.解码一帧视频压缩数据，得到视频像素数据
//            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
//            if (ret < 0)
//            {
//                LOGE("%s","解码错误");
//                return;
//            }
//
//
//            //为0说明解码完成，非0正在解码
//            if (got_picture)
//            {
//                LOGE("%s","解码绘制");
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
//                LOGI("解码第%d帧",frame_count);
//                usleep(1000*16);
//            }
//        }else{
//            LOGI("解码失败");
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


void FFmpegVideoPlayer::play2(const char *input ,ANativeWindow* aNativeWindow ){




    av_register_all();

    // 封装格式上下文结构体，也是统领全局的结构体，保存了视频文件封装格式相关信息。
    AVFormatContext * pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx,input,NULL,NULL)!=0){
        LOGE("NDK>>>%s","avformat_open_input打开失败");
        return;
    }

    if(	avformat_find_stream_info(pFormatCtx,NULL)<0){
        LOGE("NDK>>>%s","avformat_find_stream_info失败");
        return ;
    }
    LOGE("NDK>>>%s","成功");

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    //遍历封装格式中所有流
    for (; i < pFormatCtx->nb_streams; ++i) {

        //获取视频流pFormatCtx->streams[i]
        //pFormatCtx->streams[i]->codec获取编码器
        //codec_type获取编码器类型
        //当前流等于视频 记录下标
        if (pFormatCtx->streams[i]->codec->codec_type ==AVMEDIA_TYPE_VIDEO) {
            v_stream_idx = i;
            break;
        }
    }
    if (v_stream_idx==-1) {
        LOGE("没有找视频流")
    }else{
        LOGE("找到视频流")
    }

    //编码器上下文结构体，保存了视频（音频）编解码相关信息
    //得到视频流编码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;

    //	 每种视频（音频）编解码器(例如H.264解码器)对应一个该结构体。
    AVCodec *pCodec =avcodec_find_decoder(pCodecCtx->codec_id);

    //（迅雷看看，找不到解码器，临时下载一个解码器）
    if (pCodec == NULL)
    {
        LOGE("%s","找不到解码器\n");
        return;
    }else{
        LOGE("%s","找到解码器\n");
    }

    if(avcodec_open2(pCodecCtx,pCodec,NULL)==0){
        LOGE("%s","打开编码器成功\n");
    }else{
        LOGE("%s","打开编码器失败\n");
        return;
    }
    //输出视频信息
    LOGE("视频的文件格式：%s",pFormatCtx->iformat->name);
    //得到视频播放时长
    if(pFormatCtx->duration != AV_NOPTS_VALUE){
        int hours, mins, secs, us;
        int64_t duration = pFormatCtx->duration + 5000;
        secs = duration / AV_TIME_BASE;
        us = duration % AV_TIME_BASE;
        mins = secs / 60;
        secs %= 60;
        hours = mins/ 60;
        mins %= 60;
        LOGE("%02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);

    }
    LOGE("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
    LOGE("解码器的名称：%s",pCodec->name);

    //
    //	//存储一帧压缩编码数据。
    AVPacket *packet =(AVPacket*)av_malloc(sizeof(AVPacket));
    //
    //	//输出转码文件地址
    //	FILE *fp_yuv = fopen(output_cstr,"wb+");
    //
    //	//AVFrame用于存储解码后的像素数据(YUV)
    //	//内存分配
    AVFrame *pFrame = av_frame_alloc();
    //
    //	//YUV420转码用
    //	AVFrame *pFrameYUV = av_frame_alloc();


    int got_picture, ret;
    //
//	//	//返回和java surface关联的ANativeWindow通过本地本地方法交互
    ANativeWindow * nativeWindow = aNativeWindow;
    ////	//缓存
    ANativeWindow_Buffer outBuffer;
    ////	//设置缓存的几何信息
    AVFrame *rgb_frame = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,pCodecCtx->width,pCodecCtx->height));
    //
    //
    //读取每一帧
    /**
     *返回下一帧的流
     * 此函数返回存储在文件中的内容，并且不会验证解码器有什么有效帧。
     * 函数将存储在文件中的帧进行分割 并且返回给每一个调用者。
     *
     * 函数不会删除在有效帧之间的无效数据 以便在可能解码过程中提供解码器最大的信息帮助
     * 如果 pkt->buf 是空的,那么这个对应数据包是有效的直到下一次调用av_read_frame()
     * 或者直到使用avformat_close_input().否则包无期限有效
     * 在这两种情况下 这个数据包当你不在需要的时候,你必须使用使用av_free_packet释放它
     * 对于视屏,数据包刚好只包含一帧.对于音频,如果它每一帧是一个已知固定大小的,那么他包含整数帧(如. PCM or ADPCM data)
     * 如果音频帧具有可变大小(如. MPEG audio),那么他只包含一帧
     * pkt->pts, pkt->dts and pkt->duration 始终在AVStream.time_base 单位设置正确的数值
     *(如果这个格式无法提供.那么进行猜测)
     * 如果视频格式有B帧那么pkt->pts可以是 AV_NOPTS_VALUE.如果你没有解压他的有效部分那么最好依靠pkt->dts
     *
     * @return 0表示成功, < 0 错误或者文结束
     */

    while(av_read_frame(pFormatCtx,packet)>=0){

        //一个包里有很多种类型如音频视频等 所以判断 这个包对应的流的在封装格式的下表
        //如果这个包是视频频包那么得到压缩的视频包
        if (packet->stream_index==v_stream_idx) {
            LOGE("测试");

            ret=avcodec_decode_video2(pCodecCtx,pFrame,&got_picture,packet);
            if(ret>=0){
                LOGE("解压成功");
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                //				sws_scale(sws_ctx,pFrame->data,pFrame->linesize,0,pCodecCtx->height,pFrameYUV->data,pFrameYUV->linesize);

                //输出到YUV文件
                //AVFrame像素帧写入文件
                //data解码后的图像像素数据（音频采样数据）
                //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
                //U V 个数是Y的1/4
                //				int y_size = pCodecCtx->width * pCodecCtx->height;
                ANativeWindow_lock(nativeWindow,&outBuffer,NULL);
//				////
                ANativeWindow_setBuffersGeometry(nativeWindow,pCodecCtx->width,pCodecCtx->height,WINDOW_FORMAT_RGBA_8888);
                avpicture_fill((AVPicture *)rgb_frame,out_buffer, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height);
//
//
                libyuv::I420ToARGB(pFrame->data[0],pFrame->linesize[0],
                           pFrame->data[2],pFrame->linesize[2],
                           pFrame->data[1],pFrame->linesize[1],
                           rgb_frame->data[0], rgb_frame->linesize[0],
                           pCodecCtx->width,pCodecCtx->height);
                int h = 0;
//                for (h = 0; h < pCodecCtx->height; h++) {
//                    memcpy(outBuffer.bits + h * outBuffer.stride*4, out_buffer + h * rgb_frame->linesize[0], rgb_frame->linesize[0]);
////				memcpy(outBuffer.bits,out_buffer,pCodecCtx->width*pCodecCtx->height*4);
//                }
                memcpy(outBuffer.bits,out_buffer,pCodecCtx->width*pCodecCtx->height*4);
                LOGE("锁定成功");
                ANativeWindow_unlockAndPost(nativeWindow);

//                //获取帧率tbr fbs
//                float fram_rate =pFormatCtx->streams[v_stream_idx]->avg_frame_rate.num/pFormatCtx->streams[v_stream_idx]->avg_frame_rate.den;
//

                //线程休眠防止过快奔溃
//                usleep(1);


            }
        }
        av_free_packet(packet);
    }




    //关闭文件
    //	fclose(fp_yuv);
    ANativeWindow_release(nativeWindow);
//	//关闭资源
    av_frame_free(&pFrame);
//	//	av_frame_free(&pFrameYUV);
//	av_frame_free(&rgb_frame);
//	//关闭编码器上下文
    avcodec_close(pCodecCtx);
//	//关闭输入流
    avformat_close_input(&pFormatCtx);
//	//关闭封装格式
    avformat_free_context(pFormatCtx);


}






