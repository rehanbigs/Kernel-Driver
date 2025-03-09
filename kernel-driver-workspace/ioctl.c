#include <stdio.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h> // Add this line to include the close function

int main() {
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("Failed to set video format");
        close(fd); // Now it will work
        return -1;
    }

    printf("Video format set successfully: %dx%d\n", format.fmt.pix.width, format.fmt.pix.height);
    close(fd); // Close the device properly
    return 0;
}

