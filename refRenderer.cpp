#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <vector>

#include "refRenderer.h"
#include "image.h"
#include "noise.h"
#include "sceneLoader.h"
#include "util.h"

RefRenderer::RefRenderer() {
    image = NULL;

    numCircles = 0;
    position = NULL;
    velocity = NULL;
    color = NULL;
    radius = NULL;
}

RefRenderer::~RefRenderer() {

    if (image) {
        delete image;
    }

    if (position) {
        delete [] position;
        delete [] velocity;
        delete [] color;
        delete [] radius;
    }
}

const Image*
RefRenderer::getImage() {
    return image;
}

void
RefRenderer::setup() {
    // nothing to do here
}

// allocOutputImage --
//
// Allocate buffer the renderer will render into.  Check status of
// image first to avoid memory leak.
void
RefRenderer::allocOutputImage(int width, int height) {

    if (image)
        delete image;
    image = new Image(width, height);
}

// clearImage --
//
// Clear's the renderer's target image.  The state of the image after
// the clear depends on the scene being rendered.
void
RefRenderer::clearImage() {

    // clear image to white unless this is the snowflake scene.  For
    // the snowflake clear the image to a more pleasing color ramp

    if (sceneName == SNOWFLAKES || sceneName == SNOWFLAKES_SINGLE_FRAME) {

        for (int j=0; j<image->height; j++) {
            float* ptr = image->data + (4 * j * image->width);
            float shade = .4f + .45f * static_cast<float>(image->height-j) / image->height;
            for (int i=0; i<image->width; i++) {
                ptr[0] = ptr[1] = ptr[2] = shade;
                ptr[3] = 1.f;
                ptr += 4;
            }
        }
    } else {
        image->clear(1.f, 1.f, 1.f, 1.f);
    }
}

void
RefRenderer::loadScene(SceneName scene) {
    sceneName = scene;
    loadCircleScene(sceneName, numCircles, position, velocity, color, radius);
}

// advanceAnimation --
//
// Advance the simulation one time step.  Updates all circle positions
// and velocities
void
RefRenderer::advanceAnimation() {

    // only the snowflake scene has animation

    if (sceneName == SNOWFLAKES) {

        const float dt = 1.f / 60.f;
        const float kGravity = -1.8f; // sorry Newton
        const float kDragCoeff = 2.f;

        for (int i=0; i<numCircles; i++) {

            int index3 = 3 * i;

            // hack to make farther circles move more slowly, giving the
            // illusion of parallax
            float forceScaling = CLAMP(1.f - position[index3+2], .1f, 1.f);

            // add some noise to the motion to make the snow flutter
            float noiseInput[3];
            float noiseForce[2];
            noiseInput[0] = 10.f * position[index3];
            noiseInput[1] = 10.f * position[index3+1];
            noiseInput[2] = 255.f * position[index3+2];
            vec2CellNoise(noiseInput, noiseForce, i);
            noiseForce[0] *= 7.5f;
            noiseForce[1] *= 5.f;

            // drag
            float dragForce[3];
            dragForce[0] = -1.f * kDragCoeff * velocity[index3];
            dragForce[1] = -1.f * kDragCoeff * velocity[index3+1];

            // update positions
            position[index3]   += velocity[index3] * dt;
            position[index3+1] += velocity[index3+1] * dt;
            position[index3+2] += velocity[index3+2] * dt;

            // update forces
            velocity[index3]   += forceScaling * (noiseForce[0] + dragForce[0]) * dt;
            velocity[index3+1] += forceScaling * (kGravity + noiseForce[1] + dragForce[1]) * dt;

            // if the snowflake has moved off the left, right or bottom of
            // the screen, place it back at the top and give it a
            // pseudorandom x position and velocity.
            if ( (position[index3+1] + radius[i] < 0.f) ||
                 (position[index3]+radius[i]) < -0.f ||
                 (position[index3]-radius[i]) > 1.f)
            {
                noiseInput[0] = 255.f * position[index3];
                noiseInput[1] = 255.f * position[index3+1];
                noiseInput[2] = 255.f * position[index3+2];
                vec2CellNoise(noiseInput, noiseForce, i);

                position[index3] = .5f + .5f * noiseForce[0];
                position[index3+1] = 1.35f + radius[i];

                // restart from 0 vertical velocity.  Choose a
                // pseudo-random horizontal velocity.
                velocity[index3] = 2.f * noiseForce[1];
                velocity[index3+1] = 0.f;
            }
        }
    } else if (sceneName == BOUNCING_BALLS) {
        const float dt = 1.f / 60.f;
        const float kGravity = -2.8f; // sorry Newton
        const float kDragCoeff = -0.8f; 
        const float epsilon = 0.001f; 

        for (int i=0; i<numCircles; i++) {
            int index3 = 3 * i;

            // reverse velocity if center position < 0
            float oldVelocity = velocity[index3+1]; 
            float oldPosition = position[index3+1]; 

            if (oldVelocity == 0.f && oldPosition == 0.f) { // stop-condition 
                continue; 
            }

            if (position[index3+1] < 0 && oldVelocity < 0.f) { // bounce ball 
                velocity[index3+1] *= kDragCoeff; 
            }

            // update velocity: v = u + at (only along y-axis)
            velocity[index3+1] += kGravity * dt; 

            // update positions (only along y-axis)
            position[index3+1] += velocity[index3+1] * dt;

            if (fabsf(velocity[index3+1] - oldVelocity) < epsilon 
                    && oldPosition < 0.0f 
                    && fabsf(position[index3+1]-oldPosition) < epsilon) { // stop ball 
                velocity[index3+1] = 0.f; 
                position[index3+1] = 0.f; 
            } 
        }
    } else if (sceneName == HYPNOSIS) { 
        float cutOff = 0.5f;  
        for (int i = 0; i < numCircles; i++) { // update radius 
            // place circle back in center after reaching threshold radisus 
            if (radius[i] > cutOff) { 
                radius[i] = 0.02f; 
            } else { 
                radius[i] += 0.01f; 
            }
        }
    } else if (sceneName == FIREWORKS) {
        const float dt = 1.f / 60.f;
        const float pi = 3.14159;
        const float maxDist = 0.25f; 

        for (int i = 0; i < NUM_FIREWORKS; i++) { 
            int index3i = 3 * i;
            // fire-work center
            float cx = position[index3i]; 
            float cy = position[index3i+1]; 
            for (int j = 0; j < NUM_SPARKS; j++) { 
                int sIdx = NUM_FIREWORKS + i * NUM_SPARKS + j;
                int index3j = 3 * sIdx;
                
                // update position
                position[index3j] += velocity[index3j] * dt;  
                position[index3j+1] += velocity[index3j+1] * dt; 

                // fire-work sparks
                float sx = position[index3j]; 
                float sy = position[index3j+1];

                // compute vector from firework-spark
                float cxsx = sx - cx; 
                float cysy = sy - cy;
    
                // compute distance from fire-work 
                float dist = sqrt(cxsx * cxsx + cysy * cysy);
                if (dist > maxDist) { // restore to starting position 
                    // random starting position on fire-work's rim
                    float angle = (j * 2 * pi)/NUM_SPARKS;
                    float sinA = sin(angle); 
                    float cosA = cos(angle); 
                    float x = cosA * radius[i]; 
                    float y = sinA * radius[i]; 

                    position[index3j] = position[index3i] + x;  
                    position[index3j+1] = position[index3i+1] + y;  
                    position[index3j+2] = 0.0f; 

                    // travel scaled unit length 
                    velocity[index3j] = cosA/5.0;  
                    velocity[index3j+1] = sinA/5.0; 
                    velocity[index3j+2] = 0.0f;  
                } 
            }
        } 

    } 
}

static inline void
lookupColor(float coord, float& r, float& g, float& b) {

    const int N = 5;

    float lookupTable[N][3] = {
        {1.f, 1.f, 1.f},
        {1.f, 1.f, 1.f},
        {.8f, .9f, 1.f},
        {.8f, .9f, 1.f},
        {.8f, 0.8f, 1.f},
    };

    float scaledCoord = coord * (N-1);

    int base = std::min(static_cast<int>(scaledCoord), N-1);

    // linearly interpolate between values in the table based on the
    // value of coord
    float weight = scaledCoord - static_cast<float>(base);
    float oneMinusWeight = 1.f - weight;

    r = (oneMinusWeight * lookupTable[base][0]) + (weight * lookupTable[base+1][0]);
    g = (oneMinusWeight * lookupTable[base][1]) + (weight * lookupTable[base+1][1]);
    b = (oneMinusWeight * lookupTable[base][2]) + (weight * lookupTable[base+1][2]);
}

// shadePixel --
//
// Computes the contribution of the specified circle to the
// given pixel.  All values are provided in normalized space, where
// the screen spans [0,1]^2.  The color/opacity of the circle is
// computed at the pixel center.
void
RefRenderer::shadePixel(
    int circleIndex,
    float pixelCenterX, float pixelCenterY,
    float px, float py, float pz,
    float* pixelData)
{
    float diffX = px - pixelCenterX;
    float diffY = py - pixelCenterY;
    float pixelDist = diffX * diffX + diffY * diffY;

    float rad = radius[circleIndex];
    float maxDist = rad * rad;

    // 当前像素不在圆形范围内，circle does not contribute to the image
    if (pixelDist > maxDist)
        return;

    float colR, colG, colB;
    float alpha;

    // there is a non-zero contribution.  Now compute the shading
    if (sceneName == SNOWFLAKES || sceneName == SNOWFLAKES_SINGLE_FRAME) {

        //  SNOWFLAKES 或 SNOWFLAKES_SINGLE_FRAME，则使用一个渐变效果。
        //Snowflake opacity falls off with distance from center.
        // Snowflake color is determined by distance from center and
        // radially symmetric.  The color value f(dist) is looked up
        // from a table.

        const float kCircleMaxAlpha = .5f;
        const float falloffScale = 4.f;

        float normPixelDist = sqrt(pixelDist) / rad;
        lookupColor(normPixelDist, colR, colG, colB);

        float maxAlpha = kCircleMaxAlpha * CLAMP(.6f + .4f * (1.f-pz), 0.f, 1.f);
        alpha = maxAlpha * exp(-1.f * falloffScale * normPixelDist * normPixelDist);

    } else {

        // simple: each circle has an assigned color
        int index3 = 3 * circleIndex;
        colR = color[index3];
        colG = color[index3+1];
        colB = color[index3+2];
        alpha = .5f;
    }

    // 混合操作是 “读-改-写” 操作，即读取像素当前状态、修改颜色和透明度，然后写回图像数据。应当是原子操作，
    // 确保多线程环境中多个圆形对同一像素的写入顺序正确，否则透明度混合会出现错误。如果在序列处理中，圆1先处理像素P，然后是圆2，那么在并行处理中也必须保证这相同的顺序。
    // The following code is *very important*: it blends the
    // contribution of the circle primitive with the current state
    // of the output image pixel.  This is a read-modify-write
    // operation on the image, and it needs to be atomic.  Moreover,
    // (and even more challenging) all writes to this pixel must be
    // performed in same order as when the circles are processed
    // serially.
    //
    // That is, if circle 1 and circle 2 both write to pixel P.
    // circle 1's contribution *must* be blended in first, then
    // circle 2's.  If this invariant is not preserved, the
    // rendering of transparent circles will not be correct.

    //圆的颜色按透明度比例叠加在原像素颜色上。cuda的实现必须是原子性的，并且保证次序
    float oneMinusAlpha = 1.f - alpha;
    pixelData[0] = alpha * colR + oneMinusAlpha * pixelData[0];
    pixelData[1] = alpha * colG + oneMinusAlpha * pixelData[1];
    pixelData[2] = alpha * colB + oneMinusAlpha * pixelData[2];
    pixelData[3] += alpha;
}

void
RefRenderer::render() {

    // render all circles
    for (int circleIndex=0; circleIndex<numCircles; circleIndex++) {

        int index3 = 3 * circleIndex;   //3维空间，所以有x y z三个坐标抽
        //圆中心点位置
        float px = position[index3];
        float py = position[index3+1];
        float pz = position[index3+2];
        float rad = radius[circleIndex];    //半径

        // 每个圆的二维的bounding box范围，所有计算都映射到一个标准化的二维坐标空间 [0,1]^2 内进行
        //compute the bounding box of the circle.  This bounding box
        // is in normalized coordinates
        float minX = px - rad;
        float maxX = px + rad;
        float minY = py - rad;
        float maxY = py + rad;

        // convert normalized coordinate bounds to integer screen
        // pixel bounds.  Clamp to the edges of the screen.
        //计算圆在 x 方向和Y方向的边界范围
        int screenMinX = CLAMP(static_cast<int>(minX * image->width), 0, image->width);
        int screenMaxX = CLAMP(static_cast<int>(maxX * image->width)+1, 0, image->width);
        int screenMinY = CLAMP(static_cast<int>(minY * image->height), 0, image->height);
        int screenMaxY = CLAMP(static_cast<int>(maxY * image->height)+1, 0, image->height);

        float invWidth = 1.f / image->width;
        float invHeight = 1.f / image->height;

        // for each pixel in the bounding box, determine the circle's
        // contribution to the pixel.  The contribution is computed in
        // the function shadePixel.  Since the circle does not fill
        // the bounding box entirely, not every pixel in the box will
        // receive contribution.
        for (int pixelY=screenMinY; pixelY<screenMaxY; pixelY++) {

            // pointer to pixel data
            float* imgPtr = &image->data[4 * (pixelY * image->width + screenMinX)];//每个像素包含4个值（R、G、B、Alpha通道），因此每行偏移量计算为4

            for (int pixelX=screenMinX; pixelX<screenMaxX; pixelX++) {

                // When "shading" the pixel ("shading" = computing the
                // circle's color and opacity at the pixel), we treat
                // the pixel as a point at the center of the pixel.
                // We'll compute the color of the circle at this
                // point.  Note that shading math will occur in the
                // normalized [0,1]^2 coordinate space, so we convert
                // the pixel center into this coordinate space prior
                // to calling shadePixel.
                /*
                计算像素中心的标准化坐标：将每个像素视作一个中心点。这种处理方式把整个像素的颜色和透明度计算集中在像素的中心位置，而不是在像素的每个子点上进行复杂计算
                •	pixelCenterNormX和pixelCenterNormY是将像素的中心位置映射到标准化坐标系 [0,1]^2 的结果。
                •	invWidth和invHeight是图像宽度和高度的倒数，用来将像素坐标归一化。
                •	pixelX + 0.5f 和 pixelY + 0.5f 则是将像素中心对齐到标准化坐标。
                */
                float pixelCenterNormX = invWidth * (static_cast<float>(pixelX) + 0.5f);
                float pixelCenterNormY = invHeight * (static_cast<float>(pixelY) + 0.5f);
                //shadePixel函数用来计算圆对当前像素的颜色和透明度贡献
                shadePixel(circleIndex, pixelCenterNormX, pixelCenterNormY, px, py, pz, imgPtr);
                imgPtr += 4;
            }
        }
    }
}

void RefRenderer::dumpParticles(const char* filename) {

    FILE* output = fopen(filename, "w");

    fprintf(output, "%d\n", numCircles);
    for (int i=0; i<numCircles; i++) {
        fprintf(output, "%f %f %f   %f %f %f   %f\n",
                position[3*i+0], position[3*i+1], position[3*i+2],
                velocity[3*i+0], velocity[3*i+1], velocity[3*i+2],
                radius[i]);
    }
    fclose(output);

}
