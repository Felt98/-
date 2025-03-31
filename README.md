# 基于CUDA实现的彩色圆形渲染器

本项目根据 CMU15418-Assignment2，基于CUDA实现了一个简单的彩色圆形渲染器，能够加速彩色圆在空白图像上的渲染。

## 使用方法

使用make编译程序后，执行可执行程序`render`的参数选项如下：

````
有效场景名称：rgb、rgby、rand10k、rand100k、biglittle、littlebig、pattern、bouncingballs、fireworks、hypnosis、snow、snowsingle

程序选项：
-r --renderer <cpuref/cuda>   选择渲染器：ref 或 cuda（默认=cuda）
-s --size <INT>               使渲染的图像大小为 <INT>x<INT> 像素（默认=1024）
-b --bench START:END          运行帧范围 [START,END)（默认 [0,1)）
-f --file <FILENAME>          输出文件名（FILENAME_xxxx.ppm）
-c --check                    检查 CUDA 输出是否正确，与 CPU 参考结果进行比对
-i --interactive              将渲染输出到交互式显示
````

