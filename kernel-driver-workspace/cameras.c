#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/videodev2.h>
#include <linux/file.h>
#include <linux/fdtable.h>

#define DEVICE_NAME "camera_capture"
#define INTERVAL_MS 1000 // Capture interval in milliseconds
#define WIDTH 640
#define HEIGHT 480

static struct class *camera_class = NULL;
static struct device *camera_device = NULL;
static int major_number;
static struct timer_list capture_timer;

static void *frame_buffer = NULL;
static size_t frame_size = 0;

static struct file *camera_filp = NULL;

static void capture_frame(struct work_struct *work);
DECLARE_WORK(capture_work, capture_frame);

// Camera setup (open and configure device)
static int setup_camera(void) {
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    // Open camera device
    camera_filp = filp_open("/dev/video0", O_RDWR, 0);
    if (IS_ERR(camera_filp)) {
        pr_err("Failed to open /dev/video0\n");
        return PTR_ERR(camera_filp);
    }

    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        pr_err("Device does not support video capture\n");
        return -EINVAL;
    }

    // Set video format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // Adjust as needed
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (vfs_ioctl(camera_filp, VIDIOC_S_FMT, (unsigned long)&fmt) < 0) {
        pr_err("Failed to set video format\n");
        return -EINVAL;
    }

    pr_info("Camera setup complete\n");
    return 0;
}

// Capture a frame
static void capture_frame(struct work_struct *work) {
    struct v4l2_buffer buf;

    // Dequeue buffer
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (vfs_ioctl(camera_filp, VIDIOC_DQBUF, (unsigned long)&buf) < 0) {
        pr_err("Failed to dequeue buffer\n");
        return;
    }

    // Allocate frame buffer
    if (frame_buffer) {
        vfree(frame_buffer);
    }
    frame_buffer = vmalloc(buf.bytesused);
    if (!frame_buffer) {
        pr_err("Failed to allocate frame buffer\n");
        return;
    }
    memcpy(frame_buffer, (void *)buf.m.userptr, buf.bytesused); // Correct buffer pointer
    frame_size = buf.bytesused;

    pr_info("Frame captured: %zu bytes\n", frame_size);

    // Requeue buffer
    if (vfs_ioctl(camera_filp, VIDIOC_QBUF, (unsigned long)&buf) < 0) {
        pr_err("Failed to requeue buffer\n");
    }

    // Reschedule timer
    mod_timer(&capture_timer, jiffies + msecs_to_jiffies(INTERVAL_MS));
}

// Timer callback
static void capture_timer_callback(struct timer_list *t) {
    schedule_work(&capture_work);
}

// Character device read operation
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t len, loff_t *offset) {
    if (*offset >= frame_size) {
        return 0; // EOF
    }

    if (len > frame_size - *offset) {
        len = frame_size - *offset;
    }

    if (copy_to_user(user_buffer, frame_buffer + *offset, len)) {
        return -EFAULT;
    }

    *offset += len;
    return len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
};

// Module initialization
static int __init camera_capture_init(void) {
    int ret;

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_err("Failed to register character device\n");
        return major_number;
    }

    camera_class = class_create(DEVICE_NAME); // Corrected
    if (IS_ERR(camera_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(camera_class);
    }

    camera_device = device_create(camera_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(camera_device)) {
        class_destroy(camera_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(camera_device);
    }

    // Setup camera
    ret = setup_camera();
    if (ret < 0) {
        return ret;
    }

    // Initialize and start timer
    timer_setup(&capture_timer, capture_timer_callback, 0);
    mod_timer(&capture_timer, jiffies + msecs_to_jiffies(INTERVAL_MS));

    pr_info("Camera capture module initialized\n");
    return 0;
}

// Module cleanup
static void __exit camera_capture_exit(void) {
    del_timer_sync(&capture_timer);
    cancel_work_sync(&capture_work);

    if (frame_buffer) {
        vfree(frame_buffer);
    }

    if (camera_filp) {
        filp_close(camera_filp, NULL);
    }

    device_destroy(camera_class, MKDEV(major_number, 0));
    class_destroy(camera_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("Camera capture module exited\n");
}

module_init(camera_capture_init);
module_exit(camera_capture_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module for periodic frame capture");
MODULE_VERSION("1.0");

