#ifndef __REF_RENDERER_H__
#define __REF_RENDERER_H__

#include "circleRenderer.h"


class RefRenderer : public CircleRenderer {

private:

    Image* image;
    SceneName sceneName;

    int numCircles;
    float* position;
    float* velocity;
    float* color;
    float* radius;

public:

    RefRenderer();
    virtual ~RefRenderer();

    const Image* getImage();

    void setup();

    void loadScene(SceneName name);

    void allocOutputImage(int width, int height);   //分配图片缓冲区

    void clearImage();

    void advanceAnimation();    //每帧调用一次。它更新圆圈的位置和速度

    void render();

    void dumpParticles(const char* filename);

    /*
    circleIndex：当前圆形的索引，用于查找该圆的属性。
	•	pixelCenterX, pixelCenterY：像素中心的 x 和 y 坐标。
	•	px, py, pz：圆心的 x、y 坐标和深度 z 值。
	•	pixelData：指向像素数据的指针，用于存储像素的颜色和透明度。
    */
    void shadePixel(
        int circleIndex,
        float pixelCenterX, float pixelCenterY,
        float px, float py, float pz,
        float* pixelData);  //计算圆对当前像素的颜色和透明度贡献，不在圆内的像素不会产生颜色改变。
};


#endif
